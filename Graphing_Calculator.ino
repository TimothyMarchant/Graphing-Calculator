/*
The purpose of this project was to make a graphing calculator (granted not to the same standard as a TI-84 etc)
The actual disign of the circuit is just 4 shift registers, 32 buttons, and an OLED screen, I plan on adding a FRAM chip to save character arrays.
*/
//libarys to include
//may use eeprom to save tables of data(?);  Like perfect squares maybe(?);
#include <EEPROM.h>
//graphics library and setup
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/*the screen can print 21 characters per line.  The print method will automatically goto the next line
  upon running out of space.*/
/*
  store shift register data in a byte array.
  it's size 4 since we have 4 shift registers.
  The shift registers are read from closest to microcontroller to furthest.*/
byte ShiftData[4];
//pins to use and read shift register.
const byte latchpin = 3;
const byte clockpin = 5;
const byte datapin = 7;
/*
  We store the actual expression someone types into a char array called charExpression
  We then convert it for evaulation purposes into a byte array called expression.  The actual values of each number in the char array is saved into a float array called values
  Operators in the byte array are 249=+, 250=*, 251=/, 252=^ 253=( 254=);
  Variables willed be stored as 200 (in particular 200='x');
  we have a max expression length in concern for memory.
*/
//these arrays are global variables, I will try to at some point make them local.
//Originally it made some sense to make them global but when you throw in a graphing mode it becomes kinda clunky, and less generalized.
//expression holds the converted char array for evaluation purposes.  It's larger than the actual charExpression because of how we parse charExpression.
//byte expression[80];
//holds actual values for the expression.
//float values[50];
//the number corresponds to how many characters can fit onto certain amount of lines.
//char charExpression[63];
byte charindex = 0;
byte MaxCharIndex = 63;
//this variable exists to prevent constant button presses
//that is holding down the button doesn't repeatly write the same thing until you release all buttons you're holding down
bool isPressed = false;
//needed for enter function.  Meant to help with printing the display again.
bool EvaulatedOnce = false;
//bool is here for when we add the graphing mode.  We need to have seperate menus so this is here for when the user presses the button that is assigned to "GRAPH";  See Shift4;
bool inGraphingMode=false;
//stack class, needed for postfix evaulation and conversion.
class ByteStack {
public:
  //default constructor
  ByteStack() {
    toppointer = -1;
    size = 20;
    stack[20];
  }
  //constructor with a specific stack size.
  ByteStack(byte givensize) {
    toppointer = -1;
    size = givensize;
    stack[size];
    for (byte i=0;i<size;i++){
      stack[i]=0;
    }
  }
  //Push value onto stack
  void Push(byte num) {
    if (size - 1 == toppointer) {
      return;
    }
    toppointer++;
    stack[toppointer] = num;
  }
  //return top pointing value and "remove".
  byte Pop() {
    if (toppointer < 0) {
      return;
    }
    byte temp = stack[toppointer];
    stack[toppointer] = 0;
    toppointer--;
    return temp;
  }
  //Return top pointing value without removing
  byte Peek() {
    if (toppointer == -1) {
      return 255;
    }
    return stack[toppointer];
  }
  //get toppoitner
  byte GetToppointer() {
    return toppointer;
  }
  //check to see if empty
  bool isEmpty() {
    if (toppointer >= 0) {
      return false;
    }
    return true;
  }
  //attributes
private:
  int toppointer;
  byte size;
  byte stack[];
};
//method raises floating point number to a integer power (both positive and negative).
float power(float number, int Power) {
  float newvalue = number;
  if (Power > 0) {
    for (int i = 1; i < Power; i++) {
      newvalue = newvalue * number;
    }
  } else if (Power == 0) {
    return 1;
  } else {
    Power *= -1;
    for (int i = -1; i < Power; i++) {
      newvalue = newvalue / number;
    }
  }
  return newvalue;
}
//CHAR PARSER
/*
This section of the code parses what the user typed.  We convert it into something that the computer can more easily read, and the result will be used in the postfix conversion stage.
I used a byte array to hold certain values, if it's a number less than 200 it's an index to a float array, anything higher is some sort of operator or something special.
This idea can be generalized to a int array if the need for more indices is needed, you could probably have a byte array for a 300 character string (assuming on average there are two numbers per 3 characters)
but with a unsigned int array this number is somewhere in the 100 thousands, but that's not needed.
Operators are '+'=249,'*'=250,'/'=251,'^'=252,'('=253,')'=254;  Noice that '-' is excluded.  In the parser dealing with '-' is a slight annoyance, so instead of subtracting we add by the opposite of whatever the number is.
For example, 5-2 is the same as 5+-2.  This is one less thing to worry about when doing operations.
for other special functions/operators I will use some number >=200 for recongizing them.
I probably will at some point make this a library to make this code file nicer to read.
*/
//charexpression was what the user typed, expression is the conversion into something easier to read for a computer, and values stores the actual values that were typed into the char array.
byte CharParser(char charExpression[],byte expression[],float values[]) {
  //index for values[].  This is so we know where to put values in values[]
  byte Findex = 0;
  //index for expression[].  This is so we know where to put the indices of the float array values[] and where operators are suppose to be.
  byte Eindex = 0;
  //used for checking if we're at our first digit.
  bool firstdigit = true;
  byte temp = 0;
  //255 is the default value for decimalpoint.  If we encounter it anywhere it means there is no decimal point.
  //I chose 255 since, the longest a float can be I believe doesn't exceed more than 31 digit places(?);
  byte decimalpoint = 255;
  //bool is here to check if a value is negative when parsing.
  bool isNegative = false;
  //start usually will be zero, but if we start with something like "(((..." it checks for those and adds them accordingly in the while loop below
  byte start = 0;
  //byte Charindex=GetUsedLength();
  if (charExpression[0] == 40) {
    while (charExpression[start] == 40 && start < charindex) {
      expression[Eindex] = 253;
      start++;
      Eindex++;
    }
  }
  //run until we reach the end of the chararray or where we last entered on the array, which ever is closer.
  //NOTE if you see "return 255", 255 is for detecting that something went wrong, 255 is never used for anything so I made it for telling the enter method to stop and print to Serial monitor.
  //will make better error handling later on.
  for (byte i = start; i < charindex; i++) {
    //delay(1);
    //Serial.print("EIndex: ");
    //Serial.println(Eindex);
    //this conditional exists to check if we're at our first digit or not for a number.
    if (firstdigit) {
      //if we encounter a number (assci values between 48 and 57) set firstdigit=false;
      if (charExpression[i] >= 48 && charExpression[i] <= 57) {
        firstdigit = false;
        //temp is the position of the first digit.
        temp = i;
      } else {
        if (charExpression[i] == '-') {
          isNegative = true;
        } else {
          isNegative = false;
        }
        temp = i;
        //this prevents weird things like 0.- or 0.+  These don't make sense to write out.
        if (decimalpoint != 255 && validness(charExpression[i])) {
          Serial.println("OPERATOR AFTER DECIMAL POINT INVALID");
          return 255;
        }
      }
    }
    if (charExpression[i] == '.') {
      if (decimalpoint == 255) {
        decimalpoint = i;
      } else {
        Serial.println("INVALID EXPRESSION");
        return 255;
      }
    }
    if (i + 1 == charindex || charExpression[i + 1] == 0) {
      convertnumber(temp, i + 1, decimalpoint, Findex, isNegative,charExpression,values);
      expression[Eindex] = Findex;
      break;
    }
    //This switch check for the next character to see if it's an operator.
    //if so we have several cases to check for what we need to do to handle it.
    switch (charExpression[i + 1]) {
      //setexpression adds the number we've been parsing and then places an operator after it in expression[];
      //we manually update the values Eindex and Findex
      case '+':
        Eindex = setexpression(249, temp, i + 1, decimalpoint, Findex, isNegative, Eindex,charExpression,expression,values);
        //Eindex+=2;
        Findex++;
        //reset firstdigit to be true and start searching again for the first digit
        firstdigit = true;
        //value for decimal point for when we start searching for a new number
        decimalpoint = 255;
        break;
        //subtracting is the same as by adding the negative of whatever you were going to subtract.
        //since we look for the minus sign on the next pass we get the correct value.
      case '-':
        if (!firstdigit){
        Eindex = setexpression(249, temp, i + 1, decimalpoint, Findex, isNegative, Eindex,charExpression,expression,values);
        firstdigit = true;
        decimalpoint = 255;
        //Eindex+=2;
        Findex++;
        //if the next character after '-' then we add an extra value, -1 and add operator * (250) and then put down the left parthese.
        if (i + 2 < charindex && charExpression[i + 2] == '(') {
          expression[Eindex] = Findex;
          values[Findex] = -1;
          Findex++;
          expression[Eindex + 1] = 250;
          expression[Eindex + 2] = 253;
          Eindex += 3;
          i++;
        }
        }
        else if (i + 2 < charindex &&charExpression[i+2]=='('){
          expression[Eindex] = Findex;
          values[Findex] = -1;
          Findex++;
          expression[Eindex + 1] = 250;
          expression[Eindex + 2] = 253;
          Eindex += 3;
          i++;
        }
        break;
        //identical to '+' case but * has value 250
      case '*':
        Eindex = setexpression(250, temp, i + 1, decimalpoint, Findex, isNegative, Eindex,charExpression,expression,values);
        firstdigit = true;
        decimalpoint = 255;
        //Eindex+=2;
        Findex++;
        break;
        //identical to '+' case but / has value 251
      case '/':
        Eindex = setexpression(251, temp, i + 1, decimalpoint, Findex, isNegative, Eindex,charExpression,expression,values);
        firstdigit = true;
        decimalpoint = 255;
        //Eindex+=2;
        Findex++;
        break;
        //identical to '+' case but has value 252
      case '^':
        Eindex = setexpression(252, temp, i + 1, decimalpoint, Findex, isNegative, Eindex,charExpression,expression,values);
        firstdigit = true;
        decimalpoint = 255;
        //Eindex+=2;
        Findex++;
        break;
        //'(' is unfortunately a slight annoyance when parsing, so this code looks less nice.  I didn't make a bunch of new methods since it's mainly special cases related to '('.
      case '(':
        //that is we're right next to our starting point
        if (temp + 1 == i + 1) {
          //if the distance is one, then the previous index was either a single digit number or operator.
          //we do nothing for an operator since the previous cases place them down already.
          if (charExpression[temp] <= 57 && charExpression[temp] >= 48) {
            expression[Eindex] = Findex;
            values[Findex] = (float)(charExpression[temp] - 48);
            if (isNegative) {
              values[Findex] *= -1;
            }
            Findex++;
            expression[Eindex + 1] = 250;
            Eindex += 2;
          }
          expression[Eindex] = 253;
          Eindex++;
        } else {
          Eindex = setexpression(253, temp, i + 1, decimalpoint, Findex, isNegative, Eindex,charExpression,expression,values);
          //Eindex+=3;
          Findex++;
        }
        firstdigit = true;
        decimalpoint = 255;
        break;
      case ')':
        //this is when you have weird results like ")1" or ")2", other calculators I tested don't accept these as valid (or just ignore it);
        if (i + 1 != 19 && (charExpression[i + 1] >= 48 && charExpression[i + 1] <= 57)) {
          return;
        } else {
          Eindex = setexpression(254, temp, i + 1, decimalpoint, Findex, isNegative, Eindex,charExpression,expression,values);
          Findex++;
          //Eindex+=2;
          //this skips ')' in the char array and goes to the next spot.
          //which may or may not be an operator. if not, then we end the method and throw out it's invalid.
          //otherwise start at the new operator and work from there.
          //for example in (3+2)+5; this part of the switch starts at 2, we move i one to avoid the ) next to 2
          //and on the next pass we start at +.
          i++;
          if (charExpression[i + 1] != ')') {
            Eindex = whichoperator(charExpression[i + 1], Eindex,expression);
          } else {
            while (i + 1 != charindex && charExpression[i + 1] == ')') {
              expression[Eindex] = 254;
              Eindex++;
              i++;
            }
          }
        }
        firstdigit = true;
        decimalpoint = 255;
        break;
    }
  }
  return Eindex + 1;
}
//this method actual converts each number in the char array into it's actual value into a floating point number (e.g if you have 21+32, this method will convert 21 and 32 correctly and put it into a float value).
void convertnumber(byte first, byte last, byte decimalpoint, byte floatindex, bool isNegative,char charExpression[],float values[]) {
  //if there is no decimal simply set last=decimalpoint
  if (decimalpoint == 255) {
    decimalpoint = last;
  }
  int Power = decimalpoint - first - 1;
  Serial.println(decimalpoint);
  //run until we reach the decimal point
  for (byte i = first; i < decimalpoint; i++) {
    //the temp variables is found by subtracting the value 48 from the assci value in charExpression[i].  the 48 is there since the assci value of 0 is 48, so to get the actual value we have to subtract 48.
    byte temp = charExpression[i] - 48;
    values[floatindex] += (float)temp * power((float)10, Power);
    Power--;
  }
  //run until we reach the last digit place.  We start after decimalpoint since charExpression[decimalpoint]='.' which we don't want that.
  for (byte j = decimalpoint + 1; j < last; j++) {
    byte temp = charExpression[j] - 48;
    values[floatindex] += (float)temp * power((float)10, Power);
    Power--;
  }
  //if negative multiply by -1
  if (isNegative) {
    values[floatindex] *= -1;
  }
}
//checking for invalid expressions
bool validness(char tempchar) {
  bool isvalid = true;
  switch (tempchar) {
    case '+':
      isvalid = false;
      break;
    case '-':
      isvalid = false;
      break;
    case '*':
      isvalid = false;
      break;
    case '/':
      isvalid = false;
      break;
    case '^':
      isvalid = false;
      break;
    case '(':
      isvalid = false;
      break;
    case ')':
      isvalid = false;
      break;
  }
  return isvalid;
}
//check for an operator to put it in expression[index];  Exists for special cases of ')'.
int whichoperator(char OP, byte index,byte expression[]) {
  switch (OP) {
    case '+':
      expression[index] = 249;
      break;
    case '-':
      expression[index] = 249;
      break;
    case '*':
      expression[index] = 250;
      break;
    case '/':
      expression[index] = 251;
      break;
    case '^':
      expression[index] = 252;
      break;
      //this case is special since if you have "...)(2+..." you have to multiply both of these (really you could just distribute but that's more time consuming to parse than just evaluating directly).
    case '(':
      expression[index] = 250;
      index++;
      expression[index] = 253;
      break;
  }
  //need to update index
  index++;
  return index;
}
//This is just a method that
byte setexpression(byte OP, byte temp, byte last, byte decimal, byte Findex, bool isNegative, byte Eindex, char charExpression[],byte expression[],float values[]) {
  convertnumber(temp, last, decimal, Findex, isNegative,charExpression,values);

  expression[Eindex] = Findex;
  if (OP != 249) {
  }
  Findex++;
  Eindex++;
  if (OP == 253) {
    expression[Eindex] = 250;
    Eindex++;
  }
  expression[Eindex] = OP;
  Eindex++;
  return Eindex;
}
//POSTFIX OPERATION
//249=+,250=*,251=/,252=^,253=(,254=),255=STOP;
//this method pops off the operators that are in the stack.
byte PopOPS(byte index, ByteStack *stack, byte conversion[], bool endofpar) {
  //Stop at '(' we don't have it in postfix notation.
  //Serial.println("LOOP");
  while ((!stack->isEmpty()) && stack->Peek() != 253) {
    conversion[index] = stack->Pop();
    Serial.println(conversion[index]);
    index++;
  }
  //actually remove the left partheses.
  if (endofpar) {
    stack->Pop();
  }
  return index;
}
//this follows the standard alogirthm for converting an expression into post fix notation.  Or at least the one I learned in my data structures class (we never actually went on to how you code it though!).
byte postfixconversion(byte conversion[], byte expression[],byte ELength) {
  ByteStack *stack = new ByteStack(30);
  byte index = 0;
  bool seenzero = false;
  for (byte i = 0; i < ELength; i++) {
    //if operator do something based on what's in stack->
    if (expression[i] >= 249) {
      if (stack->isEmpty()) {
        stack->Push(expression[i]);
      } else if (expression[i] > (stack->Peek()) && expression[i] != 254) {
        if (expression[i] == 251 && stack->Peek() == 250) {
          index = PopOPS(index, stack, conversion, false);
        }
        stack->Push(expression[i]);
      } else if (stack->Peek() == 253) {
        stack->Push(expression[i]);
      } else if (expression[i] != 253) {
        if (expression[i] == 254) {
          index = PopOPS(index, stack, conversion, true);
        } else {

          index = PopOPS(index, stack, conversion, false);
          stack->Push(expression[i]);
        }
      }
    } else {
      if (seenzero && expression[i] == 0) {
        break;
      }
      if (expression[i] == 0) {
        seenzero = true;
      }
      conversion[index] = expression[i];
      index++;
    }
    delay(1);
  }
  if (!stack->isEmpty()) {
    index = PopOPS(index, stack, conversion, false);
  }
  delete stack;
  stack = nullptr;
  /*for (byte i = 0; i < 20; i++) {
    Serial.print(conversion[i]);
    Serial.print(" ");
  }*/
  Serial.println();
  return index;
}
//This evaulates the two values that given from postfixevaulation();  We put the new value on the Left value;
//once postfixevaulation reaches the end the 0th index will have the final result.  You can think of this as the final result sinking closer to the zeroth index.
byte operation(byte Left, byte OP, byte Right, float tempvalues[]) {
  //Serial.println(values[Left]);
  //Serial.println(values[Right]);
  switch (OP) {
    case 249:
      tempvalues[Left] += tempvalues[Right];
      break;
    case 250:
      tempvalues[Left] *= tempvalues[Right];
      break;
    case 251:
      tempvalues[Left] /= tempvalues[Right];
      break;
    case 252:
      tempvalues[Left] = power(tempvalues[Left], (int)tempvalues[Right]);
      break;
  }
  delay(1);
  return Left;
}
//Postfix evaulation method.  This is how we evaluate things in postfix correctly.
float postfixevaulation(byte length, byte conversion[],float values[]) {
  ByteStack* stack=new ByteStack(length);
  float tempvalues[length];
  for (byte i = 0; i < length; i++) {
    tempvalues[i] = values[i];
    //Serial.println(tempvalues[i]);
  }
  //Serial.print("Length: ");
  //Serial.println(length);
  for (byte i = 0; i < length; i++) {
    if (conversion[i] < 249) {
      stack->Push(conversion[i]);
      /*Serial.print("Conversion: ");
      Serial.println(conversion[i]);
      Serial.print("ITH: ");
      Serial.println(i);*/
    } else {
      byte Right = stack->Pop();
      //Serial.println(Right);
      delay(5);
      byte Left = stack->Pop();

      //Serial.println(Left);
      delay(50);
      stack->Push(operation(Left, conversion[i], Right, tempvalues));
      Serial.print("POP:");
      Serial.println(i);
    }
  }
  delete stack;
  stack = nullptr;
  //float temp = tempvalues[0];
  return tempvalues[0];
}
//Shift Registers and other physical components (that is everything here is relevant to the actual wiring and inputs of the buttons and microcontroller etc);
bool isSaved = false;
bool DrawingGraph = false;
bool GraphMode = false;
//this method actually gets data from shift register
void Shift_In(byte DataPin, byte ClockPin, byte index) {
  /*the shift register gives us 8 bits, if we read HIGH on a bit then write 1 for that specific index of the byte otherwise write 0 if LOW. This writes the binary value.
  E.G. if the shift register reads LOW HIGH LOW LOW LOW LOW LOW LOW, this is the same as 01000000

  */
  for (int i = 0; i < 8; i++) {
    int bit = digitalRead(DataPin);
    if (bit == HIGH) {
      bitWrite(ShiftData[index], i, 1);
    } else {
      bitWrite(ShiftData[index], i, 0);
    }
    digitalWrite(ClockPin, HIGH);  // Shift out the next bit
    digitalWrite(ClockPin, LOW);
  }
}
//The numbers given in the shift register are in binary so each case is a binary number with only 1 index that is a 1.
//E.G. you have numbers like 00000001, 00000010, etc these correspond to a button being pressed, values like 00111111 this is just multiple buttons being pressed which we just reject as they have no purpose to acount for.
//if a line is commented out then it doesn't function yet and is planned on being worked on at some point.
//Shift 1 corresponds to all number inputs excluding 8 and 9.
//closest to microcontroller.
void Shift1(byte data,char charExpression[]) {
  switch (data) {
    case 0b00000001:
      display.print('0');
      charExpression[charindex] = '0';
      break;
    case 0b00000010:
      display.print('1');
      charExpression[charindex] = '1';
      break;
    case 0b00000100:
      display.print('2');
      charExpression[charindex] = '2';
      break;
    case 0b00001000:
      display.print('3');
      charExpression[charindex] = '3';
      break;
    case 0b00010000:
      display.print('4');
      charExpression[charindex] = '4';
      break;
    case 0b00100000:
      display.print('5');
      charExpression[charindex] = '5';
      break;
    case 0b01000000:
      display.print('6');
      charExpression[charindex] = '6';
      break;
    case 0b10000000:
      display.print('7');
      charExpression[charindex] = '7';
      break;
  }
  charindex++;
}
//prints 8 and 9, then . and all normal operators.
void Shift2(byte data,char charExpression[]) {
  switch (data) {
    case 0b00000001:
      display.print('8');
      charExpression[charindex] = '8';
      break;
    case 0b00000010:
      display.print('9');
      charExpression[charindex] = '9';
      break;
    case 0b00000100:
      display.print('.');
      charExpression[charindex] = '.';
      break;
    case 0b00001000:
      display.print('+');
      charExpression[charindex] = '+';
      break;
    case 0b00010000:
      display.print('-');
      charExpression[charindex] = '-';
      break;
    case 0b00100000:
      display.print('*');
      charExpression[charindex] = '*';
      break;
    case 0b01000000:
      display.print('/');
      charExpression[charindex] = '/';
      break;
    case 0b10000000:
      display.print('^');
      charExpression[charindex] = '^';
      break;
  }
  charindex++;
}
//mostly special functions like e or sin and (, )
void Shift3(byte data,char charExpression[]) {
  switch (data) {
    case 0b00000001:
      display.print('(');
      charExpression[charindex] = '(';
      break;
    case 0b00000010:
      display.print(')');
      charExpression[charindex] = ')';
      break;
    case 0b00000100:
      //display.print("sin(");
      break;
    case 0b00001000:
      //display.print("cos(");
      break;
    case 0b00010000:
      //display.print("tan(");
      break;
    case 0b00100000:
      //display.print('e');
      //charExpression[charindex]='e';
      break;
    case 0b01000000:
      //display.print("ln(");
      break;
    case 0b10000000:
      //display.print("^2");
      break;
  }
  charindex++;
}
//reprint whatever the expression was.
void printmainmenu(char charExpression[]) {
  for (byte i = 0; i < charindex; i++) {
    display.print(charExpression[i]);
  }
}
byte SetbyteExpression(char charexp[],byte expression[],float values[], byte conversion[]){
  byte Elength = 0;
  Elength = CharParser(charexp,expression,values);
  if (Elength == 255) {
    Serial.println("FAIL");
    clear(charexp);
    return Elength;
  }

  //setup conversion and set all entries=0

  //variable for postfixevaulation
  byte length = 0;
  length = postfixconversion(conversion, expression,Elength);
  return length;
}
//enter method this is where we evaluate expressions.
void Enter(char charExpression[]) {
  byte expression[80];
  float values[50];
  byte conversion[80];
    for (byte i = 0; i < 80; i++) {
    conversion[i] = 0;
    expression[i]=0;
  }
  for (byte i=0;i<50;i++){
    values[i]=0;
  }
  byte length=SetbyteExpression(charExpression,expression,values,conversion);
  delay(50);

  if (EvaulatedOnce) {
    display.clearDisplay();
    display.setCursor(0, 0);
    printmainmenu(charExpression);
    display.display();
  }
  display.println();
  display.display();
  EvaulatedOnce = true;
  float temp = postfixevaulation(length, conversion,values);
  display.print(temp);
  delay(30);
  ClearArrays(charExpression);
}
//Clear out the arrays.
void clear(char charExpression[]) {
  display.clearDisplay();
  display.display();
  display.setCursor(0, 0);
  ClearArrays(charExpression);
  EvaulatedOnce = false;
}
void ClearArrays(char charExpression[]) {
  for (byte i = 0; i < MaxCharIndex; i++) {
    charExpression[i] = 0;
  }
  charindex = 0;
}
//last register, farthest from microcontroller.
//buttons for things that have special operations. These will call specialized functions and are placeholders for now.
void Shift4(byte data,char charExpression[]) {
  switch (data) {
    //enter
    case 0b00000001:
      Enter(charExpression);
      display.println();
      break;
    //clear
    case 0b00000010:
      clear(charExpression);
      break;
    //Delete current character
    case 0b00000100:
      if (charindex > 0) {
        charExpression[charindex - 1] = 0;
        charindex--;
        display.setCursor(0, 0);
        display.clearDisplay();
        if (GraphingMode){
          display.print("Y=");
        }
        printmainmenu(charExpression);
      }
      break;
    case 0b00001000:
      //display.print("TRACE");
      break;
    case 0b00010000:
      //display.print("GRAPH");
      if (GraphMode){
        DrawingGraph=true;
        DrawGraph();
      }
      break;
    case 0b00100000:
      //display.print("EXIT");
      GraphMode = false;
      DrawingGraph=false;
      break;
    case 0b01000000:
      if (charindex == 63) {
        return;
      }
      //display.print("X");
      break;
    case 0b10000000:
    //Y=, lets you type in a function.
      //display.print("POWER");
      GraphMode = true;
      DrawingGraph=false;
      break;
  }
}
//check for which shift register read an input. That way we know what to display.
void whichshift(byte data, byte ShiftRegisterNumber, char charExpression[]) {
  //make sure we're not exceeding max char length.  If it's shift 4 though only special things like enter/clear are in it, so we just run it.
  if (charindex >= MaxCharIndex && ShiftRegisterNumber != 4) {

    return;
  }
  //this switch makes it easier to check what's being inputted and to abstract what shift is being read.
  switch (ShiftRegisterNumber) {
    case 0:
      Shift1(data,charExpression);
      break;
    case 1:
      Shift2(data,charExpression);
      break;
    case 2:
      Shift3(data,charExpression);
      break;
    case 3:
      Shift4(data,charExpression);
      break;
  }
  //we need to only display after the user types something.  If they don't type anything this isn't ran. (that way we're not running it in void loop()).
  display.display();
}
//check for if all buttons are not being pressed.
bool areallLow() {
  for (byte j = 0; j < 4; j++) {
    Shift_In(datapin, clockpin, j);
    if (ShiftData[j] != 0) {
      //Serial.println(ShiftData[j]);
      return false;
    }
  }
  delay(50);
  return true;
}
//check for inputs;
void checkforinputs(char charExpression[]) {
  digitalWrite(latchpin, LOW);
  delayMicroseconds(20);
  digitalWrite(latchpin, HIGH);
  //don't run if the person is still holding down the button.
  if (!isPressed) {
    //we loop 4 times since there are 4 shift registers.  If you add another one just increase this number by one. j is what shift register we're on.
    //this makes reading inputs easier since we can break them into their own methods and label what they do inside the methods, etc.
    for (byte j = 0; j < 4; j++) {

      Shift_In(datapin, clockpin, j);
      //if the shiftdata is not zero then we have a button press and we run whichshift to find out what to do.
      if (ShiftData[j] != 0) {
        isPressed = true;
        whichshift(ShiftData[j], j, charExpression);

        break;
      }
    }

  }
  //otherwise check to see if all the buttons are low
  else {
    //take the not value of arealllow.  Since if the button is pressed=true, then arealllow=false, which is logically equivalent to if arealllow=true, then ispressed=false;
    isPressed = !areallLow();
  }
}
//the default screen mode. For doing arithmetic operations alone
void MainMode() {
  display.display();
  char charExpression[63];
  for (byte i=0;i<MaxCharIndex;i++){
    charExpression[i]=0;
  }
  while (!inGraphingMode){
    checkforinputs(charExpression);
  }
}
//Placeholder:  Meant for user to type in functions.
void GraphingMode() {
  char charExpression[63];
  for (byte i=0;i<MaxCharIndex;i++){
    charExpression[i]=0;
  }
  display.setCursor(0, 0);
  display.print("Y=");
  //this one block is for when I can get the FRAM chips to work.  They will save the char arrays and we print them out from reading the chips.
  //the chip I will be using is FM24C04B, which holds 512 bytes per chip (or 4kBits);
  //I'll probably will just write to the chips directly rather than have charExpression and print from them.  But for now we only have one function.
  /*
  if (charExpression[0] != 0) {
    for (byte i = 0; i < 63; i++) {
      if (charExpression[i] != 0) {
        display.print(charExpression[i]);
      }
    }
  }*/
  while (GraphMode) {
    checkforinputs(charExpression);
  }
}
//placeholder: Will draw graph.
void DrawGraph() {
  //while ()
}
//setup and loop
void setup() {
  Serial.begin(115200);
  //wait for serial monitor
  while (!Serial);
  //setup screen
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);  // Don't proceed, loop forever
  }
  display.display();
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  //delay(2000);
  pinMode(latchpin, OUTPUT);
  pinMode(clockpin, OUTPUT);
  pinMode(datapin, INPUT);
  digitalWrite(clockpin, LOW);
  //we call this to make sure every value in all three arrays is actually zero.  This is so we don't have random gabarage data in the arrays when they're made
  //I learned in C++ (and C) that they don't do this for you already unlike Java and C#.
  display.clearDisplay();
  //debugging purposes only, tells me when the program actually begins.
  Serial.println("Start");
}

void loop() {
  if (!inGraphingMode){
    GraphingMode();
  }
  MainMode();
  delay(25);
}
