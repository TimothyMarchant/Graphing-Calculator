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
//made charindex global since that makes certain things easier.
byte charindex = 0;
const byte MaxCharIndex = 63;
//this variable exists to prevent constant button presses
//that is holding down the button doesn't repeatly write the same thing until you release all buttons you're holding down
bool isPressed = false;
//needed for enter function.  Meant to help with printing the display again.
bool EvaulatedOnce = false;
//bool is here for when we add the graphing mode.  We need to have seperate menus so this is here for when the user presses the button that is assigned to "GRAPH";  See Shift4;
bool isSaved = false;
bool DrawingGraph = false;
bool GraphMode = false;
bool ScalingMode = false;
bool PressedEnter = false;
//default value
float ScalingFactor = 10;
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
    for (byte i = 0; i < size; i++) {
      stack[i] = 0;
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
I've capped the character array at 63 originally for memory concern and problems with the display, but later I may increase this to something much larger.
but with a unsigned int array this number is somewhere in the 100 thousands, but that's not needed.
Operators are '+'=249,'*'=250,'/'=251,'^'=252,'('=253,')'=254;  Noice that '-' is excluded.  In the parser dealing with '-' is a slight annoyance, so instead of subtracting we add by the opposite of whatever the number is.
For example, 5-2 is the same as 5+-2.  This is one less thing to worry about when doing operations.
for other special functions/operators I will use some number >=200 for recognizing them.
I probably will at some point make this a library to make this code file nicer to read.
*/
//charexpression was what the user typed, expression is the conversion into something easier to read for a computer, and values stores the actual values that were typed into the char array.
byte CharParser(char charExpression[], byte expression[], float values[]) {
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
  //we have these starting if statements for weird cases that start on them, how the parser is made prevents them from being read on the first iteration.
  if (charExpression[0] == '(') {
    expression[Eindex] = 253;
    Eindex++;
  } else if (charExpression[0] == 'x') {
    expression[Eindex] = 200;
    Eindex++;
  } else if (charExpression[0] == '-' && (charExpression[1] == 'x' || charExpression[1] == '(')) {
    values[Findex] = -1;
    expression[Eindex] = Findex;
    expression[Eindex + 1] = 250;
    Findex++;
    Eindex += 2;
  }
  //run until we reach the end of the chararray or where we last entered on the array, which ever is closer.
  //NOTE if you see "return 255", 255 is for detecting that something went wrong, 255 is never used for anything so I made it for telling the enter method to stop and print to Serial monitor.
  //will make better error handling later on.
  for (byte i = 0; i < charindex; i++) {
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
    //check for decimal point.
    if (charExpression[i] == '.') {
      if (decimalpoint == 255) {
        decimalpoint = i;
      } else {
        Serial.println("INVALID EXPRESSION");
        return 255;
      }
    }
    //terminate the loop, and place the final number (if present) into the byte and float arrays.
    if ((i + 1 == charindex || charExpression[i + 1] == 0) && charExpression[i] != 'x') {
      convertnumber(temp, i + 1, decimalpoint, Findex, isNegative, charExpression, values);
      expression[Eindex] = Findex;
      Eindex++;
      break;
    } 
    //since we've already played 'x' from the switch from before we don't need to do anything on the last pass.
    else if (i + 1 == charindex && charExpression[i] == 'x') {
      Eindex++;
      break;
    }
    //This switch check for the next character to see if it's an operator.
    //if so we have several cases to check for what we need to do to handle it.
    switch (charExpression[i + 1]) {
      //setexpression adds the number we've been parsing and then places an operator after it in expression[];
      //we manually update the values Eindex and Findex
      case '+':
        if (charExpression[i] != 'x') {
          Eindex = setexpression(249, temp, i + 1, decimalpoint, Findex, isNegative, Eindex, charExpression, expression, values);
          Findex++;
        } else {
          expression[Eindex] = 249;
          Eindex++;
        }
        //reset firstdigit to be true and start searching again for the first digit
        firstdigit = true;
        //value for decimal point for when we start searching for a new number
        decimalpoint = 255;

        break;
        //subtracting is the same as by adding the negative of whatever you were going to subtract.
        //since we look for the minus sign on the next pass we get the correct value.
      case '-':
        if (!firstdigit) {
          if (charExpression[i] != 'x') {
            Eindex = setexpression(249, temp, i + 1, decimalpoint, Findex, isNegative, Eindex, charExpression, expression, values);
            Findex++;
            Serial.println("MINUS");
          } else {
            expression[Eindex] = 249;
            Eindex++;
          }
          firstdigit = true;
          decimalpoint = 255;
          //if the next character after '-' then we add an extra value, -1 and add operator * (250) and then put down the left parthese.
          if (i + 2 < charindex && (charExpression[i + 2] == '(' || charExpression[i + 2] == 'x')) {
            expression[Eindex] = Findex;
            values[Findex] = -1;
            Findex++;
            expression[Eindex + 1] = 250;
            if (charExpression[i + 2] == '(') {
              expression[Eindex + 2] = 253;
            }
            //That is charExpression[i+2]='x'.
            else {
              expression[Eindex + 2] = 200;
            }
            Eindex += 3;
            i++;
          }
        }
        //exists for same reason as before.  I will look for a better way to make this look nicer.  and yes this needs to be in both places
        //I'm not exactly sure why me just putting this as an if and removing the top one breaks the code, but I'm trying to figure it out.
        else if (i + 2 < charindex && (charExpression[i + 2] == '(' || charExpression[i + 2] == 'x')) {
          expression[Eindex] = 249;
          Eindex++;
          expression[Eindex] = Findex;
          values[Findex] = -1;
          Findex++;
          expression[Eindex + 1] = 250;
          if (charExpression[i + 2] == '(') {
            expression[Eindex + 2] = 253;
          }
          //That is charExpression[i+2]='x'.
          else {
            expression[Eindex + 2] = 200;
          }
          Eindex += 3;
          i++;
        }
        if (charExpression[i] == 'x') {
          expression[Eindex] = 249;
          Eindex++;
        }
        break;
        //identical to '+' case but * has value 250
      case '*':
        if (charExpression[i] != 'x') {
          Eindex = setexpression(250, temp, i + 1, decimalpoint, Findex, isNegative, Eindex, charExpression, expression, values);
          Findex++;
        } else {
          expression[Eindex] = 250;
          Eindex++;
        }
        firstdigit = true;
        decimalpoint = 255;
        break;
        //identical to '+' case but / has value 251
      case '/':
        if (charExpression[i] != 'x') {
          Eindex = setexpression(251, temp, i + 1, decimalpoint, Findex, isNegative, Eindex, charExpression, expression, values);
          Findex++;
        } else {
          expression[Eindex] = 251;
          Eindex++;
        }
        firstdigit = true;
        decimalpoint = 255;
        break;
        //identical to '+' case but has value 252
      case '^':
        if (charExpression[i] != 'x') {
          Eindex = setexpression(252, temp, i + 1, decimalpoint, Findex, isNegative, Eindex, charExpression, expression, values);
          Findex++;
        } else {
          expression[Eindex] = 252;
          Eindex++;
        }
        firstdigit = true;
        decimalpoint = 255;
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
          } else if (charExpression[temp] == 'x') {
            expression[Eindex] = 250;
            Eindex++;
          }
          expression[Eindex] = 253;
          Eindex++;
        } else {
          if (charExpression[i] != 'x') {
            Eindex = setexpression(253, temp, i + 1, decimalpoint, Findex, isNegative, Eindex, charExpression, expression, values);
            Findex++;
          } else {
            expression[Eindex] = 253;
            Eindex++;
          }
        }
        firstdigit = true;
        decimalpoint = 255;
        break;
      case ')':
        //this is when you have weird results like ")1" or ")2", other calculators I tested don't accept these as valid (or just ignore it);
        if (i + 1 != 63 && (charExpression[i + 1] >= 48 && charExpression[i + 1] <= 57)) {
          return;
        } else {
          if (charExpression[i] != 'x') {
            Eindex = setexpression(254, temp, i + 1, decimalpoint, Findex, isNegative, Eindex, charExpression, expression, values);
            Findex++;
          } else {
            expression[Eindex] = 254;
            Eindex++;
          }
          //this skips ')' in the char array and goes to the next spot.
          //which may or may not be an operator. if not, then we end the method and throw out it's invalid.
          //otherwise start at the new operator and work from there.
          //for example in (3+2)+5; this part of the switch starts at 2, we move i one to avoid the ) next to 2
          //and on the next pass we start at +.
          i++;
          if (charExpression[i + 1] != ')') {
            Eindex = whichoperator(charExpression[i + 1], Eindex, expression);
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
      //As it turns out the 'x' case is very similiar to the ')' case.  Which means that most likely things like Sine, e, and other such things may not take that long to add.
      case 'x':
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
          expression[Eindex] = 200;
          Eindex++;
        } else {
          Eindex = setexpression(200, temp, i + 1, decimalpoint, Findex, isNegative, Eindex, charExpression, expression, values);
          Findex++;
        }
        firstdigit = true;
        decimalpoint = 255;
        break;
    }
  }
  //left in here for debugging purposes, this is useful for when I need to know when this method is messed up somewhere.
  Serial.print("Expression:");
  for (byte i = 0; i < Eindex; i++) {
    if (expression[i]==200){
      Serial.print("x");
    }
    else {
      switch (expression[i]){
        case 249:
        Serial.print("+");
        break;
        case 250:
        Serial.print("*");
        break;
        case 251:
        Serial.print("/");
        break;
        case 252:
        Serial.print("^");
        break;
        case 253:
        Serial.print("(");
        break;
        case 254:
        Serial.print(")");
        break;
        default:
        Serial.print(expression[i]);
        break;
      }
    
    }
    Serial.print(" ");
  }
  Serial.println();
  //needed to know how long to run postfixconversion method.
  return Eindex;
}
//this method actual converts each number in the char array into it's actual value into a floating point number (e.g if you have 21+32, this method will convert 21 and 32 correctly and put it into a float value).
void convertnumber(byte first, byte last, byte decimalpoint, byte floatindex, bool isNegative, char charExpression[], float values[]) {
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
//checking for invalid expressions will probably be moved into a larger error/exception handing method
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
int whichoperator(char OP, byte index, byte expression[]) {
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
    case 'x':
      expression[index] = 250;
      index++;
      expression[index] = 200;
      break;
  }
  //need to update index
  index++;
  return index;
}
//This is just a method that sets up the byte expression and float values and fills it with certain values that are given in the method.
byte setexpression(byte OP, byte temp, byte last, byte decimal, byte Findex, bool isNegative, byte Eindex, char charExpression[], byte expression[], float values[]) {
  //call this method to set the value of a number and put it into the values array.
  convertnumber(temp, last, decimal, Findex, isNegative, charExpression, values);
  expression[Eindex] = Findex;
  Findex++;
  Eindex++;
  //multiply if the operator is "(" or "x"
  if (OP == 253 || OP == 200) {
    expression[Eindex] = 250;
    Eindex++;
  }
  //finally set the original operator here.
  expression[Eindex] = OP;
  Eindex++;
  return Eindex;
}
/*POSTFIX OPERATION
  This section of the code does the postfix conversion and evaulation of expressions.
  The alogrithms that are used we're taught in my datastructures class (although we only went over how the alogrithm worked and how to do it by hand, not how to code it).
*/
//249=+,250=*,251=/,252=^,253=(,254=),255=STOP;
//this method pops off the operators that are in the stack in the postfix conversion method.
byte PopOPS(byte index, ByteStack *stack, byte conversion[], bool endofpar) {
  //Stop at '(' we don't have it in postfix notation.
  while ((!stack->isEmpty()) && stack->Peek() != 253) {
    conversion[index] = stack->Pop();
    Serial.println(conversion[index]);
    index++;
  }
  //actually remove the left partheses.
  if (endofpar) {
    Serial.println(stack->Pop());
  }
  Serial.print("POPOPS");
  Serial.println();
  return index;
}
//converts regular infix expression into postfix-notation.
byte postfixconversion(byte conversion[], byte expression[], byte ELength) {
  //stack class needed for postfix conversion.
  ByteStack *stack = new ByteStack(30);
  //new index for determing where to put stuff in the new converted array.
  byte index = 0;
  //needed for checking when the end of the array really appears (of useful information).
  bool seenzero = false;
  for (byte i = 0; i < ELength; i++) {
    //if operator do something based on what's in stack->
    if (expression[i] >= 249) {
      if (stack->isEmpty()) {
        stack->Push(expression[i]);
      }
      //preserve order of operations, pop off the operators if something that is of lower precedence 
      else if (expression[i] > (stack->Peek()) && expression[i] != 254) {
        if (expression[i] == 251 && stack->Peek() == 250) {
          index = PopOPS(index, stack, conversion, false);
        }
        stack->Push(expression[i]);
        Serial.println(expression[i]);
        Serial.println("TOP");
      }
      //always push '(' they won't be in the final conversion. Also make sure it's not a right parenthese.
      else if (stack->Peek() == 253&&expression[i]!=254) {
        stack->Push(expression[i]);
        Serial.println(expression[i]);
        Serial.println("MIDDLE");
      }
      //The end of ')' implies that we pop off all operators contained in the parentheses. 
      else if (expression[i] != 253) {
        if (expression[i] == 254) {
          index = PopOPS(index, stack, conversion, true);
          Serial.println("POP");
        } else {

          index = PopOPS(index, stack, conversion, false);
          Serial.println(expression[i]);
          Serial.println("BOTTOM");
          stack->Push(expression[i]);
        }
      }
    } 
    //otherwise just put into the conversion array.
    else {
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
  Serial.print("Conversion: ");
  for (byte i=0;i<index;i++){
    Serial.print(conversion[i]);
    Serial.print(" ");
  }
  Serial.println();
  return index;
}
//This evaulates the two values that given from postfixevaulation();  We put the new value on the Left value;
//once postfixevaulation reaches the end the 0th index will have the final result.  You can think of this as the final result sinking closer to the zeroth index.
byte operation(byte Left, byte OP, byte Right, float tempvalues[]) {
  switch (OP) {
    case 249:
      tempvalues[Left] += tempvalues[Right];
      break;
    case 250:
      tempvalues[Left] *= tempvalues[Right];
      break;
    case 251:
      //I just put this huge value as a temp value until I find a better way to throw some sort of error.
      if (tempvalues[Right] == 0) {
        tempvalues[Left] = 10000000;
        return 255;
      }
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
float postfixevaulation(byte length, byte conversion[], float values[]) {
  //for when you just enter one thing no need to do anything else.
  if (length == 1) {
    return values[0];
  }
  ByteStack *stack = new ByteStack(length);
  float tempvalues[length];
  byte FirstIndex = conversion[0];
  //fill a tempvalues array to preserve the original values array.
  for (byte i = 0; i < length; i++) {
    tempvalues[i] = values[i];
  }
  //run until we reach the end of the conversion array and return that result.
  for (byte i = 0; i < length; i++) {
    if (conversion[i] < 249) {
      stack->Push(conversion[i]);
    } else {
      byte Right = stack->Pop();
      delay(5);
      byte Left = stack->Pop();
      stack->Push(operation(Left, conversion[i], Right, tempvalues));
      if (stack->Peek() == 255) {
        delete stack;
        stack = nullptr;
        return tempvalues[Left];
      }
    }
  }
  delete stack;
  stack = nullptr;
  return tempvalues[FirstIndex];
}
//Shift Registers and other physical components (that is everything here is relevant to the actual wiring and inputs of the buttons and microcontroller etc);

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
/*The numbers given in the shift register are in binary so each case is a binary number with only 1 index that is a 1.
//E.G. you have numbers like 00000001, 00000010, etc these correspond to a button being pressed, values like 00111111 this is just multiple buttons being pressed which we just reject as they have no purpose for this project.
//if a line is commented out then it doesn't function yet and is planned on being worked on at some point.*/

//Shift 1 corresponds to all number inputs excluding 8 and 9. It is the closest to microcontroller.
void Shift1(byte data, char charExpression[]) {
  if (DrawingGraph && !ScalingMode) {
    return;
  } else if (ScalingMode && charindex >= 7) {
    return;
  }
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
//prints 8 and 9, then . and all normal operators.  2nd Shift register in the daisy chain
void Shift2(byte data, char charExpression[]) {
  if (DrawingGraph && !ScalingMode) {
    return;
  } else if (ScalingMode && data > 3 && charindex >= 7) {
    return;
  }
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
//the special functions are planned for later.
//2nd Shift register in the daisy chain
void Shift3(byte data, char charExpression[]) {
  if (DrawingGraph || ScalingMode) {
    return;
  }
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

//last register, farthest from microcontroller.
//buttons for things that have special operations. These will call specialized functions and most are placeholders for now.
void Shift4(byte data, char charExpression[]) {
  switch (data) {
    //enter; evaluate or enter a scaling factor
    case 0b00000001:
      if (ScalingMode) {
        PressedEnter = true;
        ScalingMode = false;
        return;
      }
      if (GraphMode || DrawingGraph) {
        return;
      }
      Enter(charExpression);
      display.println();
      break;
    //clear; clear actual expression
    case 0b00000010:
      if (ScalingMode) {
        clear(charExpression);
        reprintScalingMode(charExpression);
        return;
      }
      if (DrawingGraph) {
        return;
      }
      clear(charExpression);
      if (GraphMode) {
        display.print("Y=");
      }
      break;
    //Delete current character
    case 0b00000100:
      if (charindex > 0) {
        if (DrawingGraph) {
          return;
        }
        charExpression[charindex - 1] = 0;
        charindex--;
        display.setCursor(0, 0);
        display.clearDisplay();
        if (ScalingMode) {
          reprintScalingMode(charExpression);
          return;
        }
        if (GraphMode) {
          display.print("Y=");
        }
        printmainmenu(charExpression);
      }
      break;
      //button brings up scaling menu.  Called from here in case you don't want to exit graphing mode.
    case 0b00001000:
      if (ScalingMode) {
        return;
      }
      ScalingMode = true;
      DrawingGraph = false;
      SetScalingFactor();
      display.clearDisplay();
      display.setCursor(0, 0);
      if (GraphingMode){
          display.print("Y=");
      }
      printmainmenu(charExpression);
      display.display();
      break;
      //This button starts the drawing process. It's intentional that it's called from here rather than in void loop().  Based on a limitation of how I stored the charExpressions.
    case 0b00010000:
      if (GraphMode && charindex > 0 && !DrawingGraph) {
        DrawingGraph = true;
        DrawGraph(charExpression);
        if (GraphMode) {
          display.clearDisplay();
          display.setCursor(0, 0);
          display.print("Y=");
          printmainmenu(charExpression);
          display.display();
        }
      }
      break;
      //exit current mode that is not the normal calculation mode.
    case 0b00100000:
      GraphMode = false;
      DrawingGraph = false;
      ScalingMode = false;
      display.clearDisplay();
      display.display();
      break;
      //button for adding 'x' for an expression
    case 0b01000000:
      if (charindex == 63 || DrawingGraph || ScalingMode) {
        return;
      }
      display.print("x");
      charExpression[charindex] = 'x';
      charindex++;
      break;
    case 0b10000000:
      //Y=, lets you type in a function.
      GraphMode = true;
      ScalingMode = false;
      DrawingGraph = false;
      break;
  }
}
//reprints the scaling menu.
void reprintScalingMode(char charExpression[]) {
  display.setCursor(0, 0);
  display.clearDisplay();
  display.print("Current Scaling Factor: ");
  display.println(ScalingFactor);
  display.print("Enter new Scaling Factor: ");
  printmainmenu(charExpression);
  display.display();
}
//ScalingMode.  Let's the user type in a scaling factor for scaling the graph drawing.
void SetScalingFactor() {
  char charExpression[63];
  byte tempcharindex = charindex;
  charindex = 0;
  ClearcharExpression(charExpression);
  reprintScalingMode(charExpression);
  //keep looping menu until enter, exit, graph, or draw is pressed.
  while (ScalingMode) {
    checkforinputs(charExpression);
  }
  delay(5);
  display.println();
  if (PressedEnter) {
    //the arrays are dummy variables for charparser, since it parses a single number.  
    //Otherwise we would have to keep track of where decimals are when it's typed into this method.
    byte temp[1] = { 0 };
    float temp2[1] = { 0 };
    CharParser(charExpression, temp, temp2);
    ScalingFactor = temp2[0];
  }
  //set to false so that it's reset to normal state.
  PressedEnter = false;
  Serial.println("Finished");
  display.clearDisplay();
  display.display();
  display.setCursor(0, 0);
  if (GraphMode) {
    display.print("Y=");
  }
  charindex = tempcharindex;
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
      Shift1(data, charExpression);
      break;
    case 1:
      Shift2(data, charExpression);
      break;
    case 2:
      Shift3(data, charExpression);
      break;
    case 3:
      Shift4(data, charExpression);
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
//reprint whatever the expression was.
void printmainmenu(char charExpression[]) {
  for (byte i = 0; i < charindex; i++) {
    display.print(charExpression[i]);
  }
}
//Calls charParser and PostFixConversion returns the postfix conversion of expression.
byte SetbyteExpression(char charexp[], float values[], byte conversion[]) {
  //we declare expression in here, for when we're done with PostFixConversion, it is no longer needed and we can save some memory.
  byte expression[80];
  byte Elength = 0;
  setarrays(expression, values, conversion);
  Elength = CharParser(charexp, expression, values);
  if (Elength == 255) {
    Serial.println("FAIL");
    clear(charexp);
    return Elength;
  }
  //variable for postfixevaulation. This is the amount of values in the conversion array that are used.
  byte length = 0;
  length = postfixconversion(conversion, expression, Elength);
  return length;
}
//sets up arrays.  In C++ (according to the internet), not all values in a array are just set to 0, so you apparently have to do it manually, unlike C#/Java.
void setarrays(byte expression[], float values[], byte conversion[]) {
  for (byte i = 0; i < 80; i++) {
    conversion[i] = 0;
    expression[i] = 0;
  }
  for (byte i = 0; i < 50; i++) {
    values[i] = 0;
  }
}
//enter method this is where we evaluate expressions.
void Enter(char charExpression[]) {
  float values[50];
  byte conversion[80];
  byte length = SetbyteExpression(charExpression, values, conversion);
  delay(50);
  byte LargestIndices[2];
  convertXsToIndices(conversion, LargestIndices);
  Serial.println();
  //just give x values a default value for when you press enter.  For testing, also in case someone types it into main menu.
  PlaceXValues(values, LargestIndices, 2);
  if (EvaulatedOnce) {
    display.clearDisplay();
    display.setCursor(0, 0);
    printmainmenu(charExpression);
    display.display();
  }
  display.println();
  display.display();
  EvaulatedOnce = true;
  float temp = postfixevaulation(length, conversion, values);
  display.print(temp,5);
  delay(30);
  ClearcharExpression(charExpression);
}
//Clear method.  Clears out expressions.
void clear(char charExpression[]) {
  display.clearDisplay();
  display.display();
  display.setCursor(0, 0);
  ClearcharExpression(charExpression);
  EvaulatedOnce = false;
}
//clear out charExpression, that is set every entry to 0.
void ClearcharExpression(char charExpression[]) {
  for (byte i = 0; i < MaxCharIndex; i++) {
    charExpression[i] = 0;
  }
  charindex = 0;
}
//the default screen mode. For doing arithmetic operations alone
void MainMode() {
  display.setCursor(0, 0);
  display.display();
  char charExpression[63];
  ClearcharExpression(charExpression);
  while (!GraphMode) {
    checkforinputs(charExpression);
  }
}
//Meant for user to type in functions
void GraphingMode() {
  //the only limitations of this method is that it supports only one function and doesn't save after you press exit (although it saves after pressing draw, and scale).
  //I planned on adding some FRAM chips to save typed in character expressions.  Since you can change expressions a lot I didn't think using EEPROM would be a good idea
  display.clearDisplay();
  char charExpression[63];
  ClearcharExpression(charExpression);
  display.setCursor(0, 0);
  display.print("Y=");
  display.display();
  while (GraphMode) {
    checkforinputs(charExpression);
  }
}
//This method looks for the largest index used in values, and from that fills all the X's with float indexes after that index
void convertXsToIndices(byte conversion[], byte indices[]) {
  byte largestindex = 0;
  for (byte i = 0; i < 80; i++) {
    if (conversion[i] < 200 && largestindex < conversion[i]) {
      largestindex = conversion[i];
    }
  }
  //need to start at the next index.
  largestindex++;
  byte tempindex = largestindex;
  //fill in the converted char array
  for (byte i = 0; i < 80; i++) {
    if (conversion[i] == 200) {
      conversion[i] = tempindex;
      tempindex++;
    }
  }
  indices[0] = largestindex;
  indices[1] = tempindex;
}
//Places X values into the float array values.  Indices is the starting point for where the x positions are and the ending point.
void PlaceXValues(float values[], byte indices[], float XValue) {
  for (byte i = indices[0]; i < indices[1]; i++) {
    values[i] = XValue;
  }
}
//Get's the values for graphing.  Save X values and results is the result of the function evaulated at a specific x value.
float GetResults(float results[], float XValues[], char charExpression[], byte RunThisManyTimes, float scale) {
  float values[50];
  byte conversion[80];
  byte length = 0;
  length = SetbyteExpression(charExpression, values, conversion);
  /*for debugging will use later for when I implement trig functions
  Serial.print("Conversion: ");
  for (byte i = 0; i < length; i++) {
    if (conversion[i] == 200) {
      Serial.print("x");
    } else {
      Serial.print(conversion[i]);
    }
    Serial.print(" ");
  }
  Serial.println();*/
  byte LargestIndices[2];
  float SpecificXValue = 0;
  convertXsToIndices(conversion, LargestIndices);
  for (byte i = 0; i < RunThisManyTimes / 2; i++) {
    PlaceXValues(values, LargestIndices, SpecificXValue);
    results[i] = postfixevaulation(length, conversion, values);
    XValues[i] = SpecificXValue;
    SpecificXValue -= 1 / scale;
    delay(15);
  }
  SpecificXValue = 0;
  for (byte i = RunThisManyTimes / 2; i < RunThisManyTimes; i++) {
    PlaceXValues(values, LargestIndices, SpecificXValue);
    results[i] = postfixevaulation(length, conversion, values);
    XValues[i] = SpecificXValue;
    SpecificXValue += 1 / scale;
    delay(15);
  }
}
//checking to make sure we're not outside the screen's dimensions.
bool isExceedingScreenDimensions(float results[], float XValues[], byte i) {
  //seperated the if statements to make it easier to read.
  if (results[i] > 64 || results[i] < 0) {
    return true;
  } else if (XValues[i] < 0 || XValues[i] > 128) {
    return true;
  }
  return false;
}
//Method that actually draws the graph
void DrawGraph(char charExpression[]) {
  //parameter for how many floats we want to generate.  For 50 floats it took for a medium sized (about 21-24 characters) about 3(?) seconds to calculate and draw.  
  //100 floats takes about 5 or so seconds.
  const byte runthismanytimes = 100;
  //might make these larger later.
  float results[runthismanytimes];
  float XValues[runthismanytimes];
  const float scale = ScalingFactor;
  display.clearDisplay();
  display.drawFastHLine(0, 32, 128, WHITE);
  display.drawFastVLine(64, 0, 64, WHITE);
  display.display();
  GetResults(results, XValues, charExpression, runthismanytimes, scale);
  Serial.println("HERE");
  //we need to setup where to draw things, the center of the screen is (64,32) since it's 128X64.
  for (byte i = 0; i < runthismanytimes; i++) {
    //Serial.print(i);
    //Serial.print(" Result: ");
    //Serial.println(results[i]);
    XValues[i] *= scale;
    //multiply by -1 since the screen's origin starts at the top left of the screen, to make it look like it's going up, we multiply by -1.
    results[i] *= scale * -1;
    results[i] += 32;
    XValues[i] += 64;
  }
  /*
    all we're doing when we do that is scaling the standard matrix of R^2 (R is the real numbers) and scaling the pivot positions of the identity matrix by some constant c
    and then multiplying each pair of points by the scaling factor.  Think of a pair of points as a vector in R^2
    We can also do shifting by adding by some arbitary constant to get a certain view of the screen.  
    You can think of the screen as the coordinate plane, but we're only restricted to positive integers (and zero) that we can map to. 
    since the screen can only draw at a positive integer and not a floating point number
    Example: consider the vector X= [5] and the matrix A= [10 0] and consider the transformation T(X) where AX=T(X) and all entries are rounded up to the nearest integer.  Then the corresponding vector becomes [50]
                                    [2]                   [0 10]                                                                                                                                                  [20]    
  */
  //It's -1 since the midpoint of the array starts at the origin (or at least at x=0) and goes in the other direction after that point.
  //this draws for x<=0
  for (byte i = 0; i < ((runthismanytimes) / 2) - 1; i++) {
    //conditional is here to prevent the functions from drawing off screen which has undesirable effects on the main screen.
    //this mostly applies to functions that grow very fast.
    if (!isExceedingScreenDimensions(results, XValues, i)) {
      display.drawLine((int)XValues[i], (int)(results[i] - 0.5f), (int)XValues[i + 1], (int)(results[i + 1] - 0.5f), WHITE);
      display.display();
    }
  }
  //draw for x>=0
  for (byte i = (runthismanytimes) / 2; i < runthismanytimes - 1; i++) {

    if (!isExceedingScreenDimensions(results, XValues, i)) {
      display.drawLine((int)XValues[i], (int)(results[i] - 0.5f), (int)XValues[i + 1], (int)(results[i + 1] - 0.5f), WHITE);
      display.display();
    }
  }
  //we don't want to add any actual characters as of right now.
  char temp[1] = { 0 };
  display.display();
  while (DrawingGraph) {
    checkforinputs(temp);
  }
  display.setCursor(0, 0);
}
//setup
void setup() {
  Serial.begin(115200);
  //wait for serial monitor
  while (!Serial)
    ;
  //setup screen
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  display.display();
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  //setup pins.
  pinMode(latchpin, OUTPUT);
  pinMode(clockpin, OUTPUT);
  pinMode(datapin, INPUT);
  digitalWrite(clockpin, LOW);
  display.clearDisplay();
  //debugging purposes only, tells me when the program actually begins.  Mainly when I have to restart the microcontroller and I want to compare outputs from previous uploads.
  Serial.println("Start");
}
//loop cycle between MainMode and GraphingMode.  MainMode is for calculations
//while GraphingMode brings up the function menu.
void loop() {
  if (GraphMode) {
    GraphingMode();
  }
  MainMode();
  delay(25);
}
