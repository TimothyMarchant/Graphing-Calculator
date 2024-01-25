#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
//store shift register data in a byte array.
//it's size 4 since we have 4 shift registers.
byte data[4];
//pins to use and read shift register.
const byte clockpin = 3;
const byte latchpin = 7;
const byte datapin = 4;
//most buttons are mapped to the shift registers, but a few are left over, so it is best to map a few to the arduino pins we weren't using (espically to a few special ones for higher priority);
const byte Enter = 2;
const byte Clear = 5;
const byte Zero = 6;
const byte One = 8;
//this variable exists to prevent constant button presses (that is holding down the button doesn't repeatly write the same thing until you release all buttons you're holding down);
bool isPressed = false;
//we need stack for conversion of expressions to postfix and for operations of functions.
class CharStack {
  private:
  char stack[40];
  private:
  byte size;
private:
  int toppointer;
public:
  CharStack() {
    size = 40;
    toppointer = -1;
  }
  //add to the stack.
public:
  void Push(char chartoadd) {
    toppointer++;
    if (toppointer == size) {
      return;
    }
    stack[toppointer] = chartoadd;
  }
  //Notice that we don't actually remove the variable rather just move the pointer.
  //when we go and add something to that same spot the original character is overwritten and gone, so effectively we do remove things.
public:
  char Pop() {
    if (toppointer < 0) {
      return '?';
    }
    //Serial.println("RAN");
    //Serial.println(stack[0]);
    toppointer--;
    //Serial.println(toppointer+1);
    return stack[toppointer + 1];
  }
public:
  char Peak() {
    if (toppointer >= 0) {
      return stack[toppointer];
    }
    return '?';
  }
public:
  byte getsize() {
    return toppointer + 1;
  }
};
//need for final steps of graphing (Might change stack?)
class intStack {
private:
  int stack[40];
private:
  byte size;
private:
  byte toppointer;
public:
  intStack() {
    size = 40;
    toppointer = -1;
  }
  //add to the stack.
public:
  void Push(int inttoadd) {
    toppointer++;
    if (toppointer == size) {
      return;
    }
    stack[toppointer] = inttoadd;
  }
public:
  int Pop() {
    if (toppointer < 0) {
      return '?';
    }
    return stack[toppointer];
    toppointer--;
  }
public:
  int Peak() {
    if (toppointer >= 0) {
      return stack[toppointer];
    }
    return -1;
  }
public:
  byte getsize() {
    return toppointer + 1;
  }
};
void PopOffOperators(byte* currentpos, char conversion[], CharStack stack) {
  for (byte j = 0; j < stack.getsize(); j++) {
    if (stack.Peak() == '(') {
      return;
    }
    conversion[*currentpos] = stack.Pop();
    (*currentpos)++;
    //Serial.println(*currentpos);
  }
}
void PostfixConversion(char entries[], char conversion[]) {
  byte size = sizeof(entries) / sizeof(entries[0]);
  CharStack stack;
  //char conversion[size];
  byte* currentpos = new byte(0);
  for (byte i = 0; i < 8; i++) {
    //referring to ascii values.
    //40=(;41=);42=*;43=+;45=-;47=/;94=^
    if (entries[i] == '(') {
      stack.Push(entries[i]);
    } else if (entries[i] == ')') {
      PopOffOperators(*currentpos, conversion, stack);
    } else if (entries[i] == '+' || entries[i] == '-') {
      Serial.println(entries[i]);
      if (stack.Peak() != '(' || stack.Peak() != '?' || stack.getsize() > 0) {
        PopOffOperators(*currentpos, conversion, stack);
      }
      //else {
      Serial.println("TL");
      stack.Push(entries[i]);
      //}
    } else if (entries[i] == '*' || entries[i] == '/') {
      if (stack.Peak() == '*' || stack.Peak() == '/' || stack.Peak() == '^') {
        PopOffOperators(*currentpos, conversion, stack);
      } else {
        stack.Push(entries[i]);
      }
    } else if (entries[i] == '^') {
      if (stack.Peak() == '^') {
        PopOffOperators(*currentpos, conversion, stack);
      } else {
        stack.Push(entries[i]);
      }
    } else {
      conversion[*currentpos] = entries[i];
      (*currentpos)++;
    }
    Serial.println(*currentpos);
  }
  while (stack.getsize() > 0) {
    //Serial.println(stack.Peak());
    delay(200);
    char temp = stack.Pop();
    conversion[*currentpos] = temp;
    Serial.println(conversion[3]);
    (*currentpos)++;
  }
  Serial.println("Hello");
  delete currentpos;
}

void setup() {
  Serial.begin(57600);
  while (!Serial)
    ;

  //delay(5000);
  /*if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
    }
    display.display();
    delay(2000);
    display.clearDisplay();*/
  pinMode(latchpin, OUTPUT);
  pinMode(clockpin, OUTPUT);
  pinMode(datapin, INPUT);
  char entries[8];
  entries[0] = '1';
  entries[1] = '3';
  entries[2] = '+';
  entries[3] = '5';
  entries[4] = '*';
  entries[5] = '9';
  entries[6] = '^';
  entries[7] = '2';
  Serial.println("12+5");
  char conversion[8];
  PostfixConversion(entries, conversion);
  //Serial.println(conversion);
  for (byte i = 0; i < 8; i++) {
    Serial.print(conversion[i]);
  }
}
byte Shift_In(byte DataPin, byte ClockPin, byte index) {
  for (int i = 0; i < 8; i++) {
    int bit = digitalRead(DataPin);
    if (bit == HIGH) {
      bitWrite(data[index], i, 1);
    } else {
      //Serial.print("0");
    }
    digitalWrite(ClockPin, HIGH);  // Shift out the next bit
    digitalWrite(ClockPin, LOW);
  }
}
//enter button
void enter() {
}
//clear button
void clear() {
}
//Shift 1 corresponds to all number inputs excluding 0 and 1.
void Shift1(byte data) {

  switch (data) {
    case 0b00000000:
      return;
    case 0b00000001:
      return;
    case 0b00000010:
      return;
    case 0b00000100:
      return;
    case 0b00001000:
      return;
    case 0b00010000:
      return;
    case 0b00100000:
      return;
    case 0b01000000:
      return;
    case 0b10000000:
      return;
  }
}
void Shift2(byte data) {
  switch (data) {
    case 0b00000000:
      return;
    case 0b00000001:
      return;
    case 0b00000010:
      return;
    case 0b00000100:
      return;
    case 0b00001000:
      return;
    case 0b00010000:
      return;
    case 0b00100000:
      return;
    case 0b01000000:
      return;
    case 0b10000000:
      return;
  }
}
void Shift3(byte data) {
  switch (data) {
    case 0b00000000:
      return;
    case 0b00000001:
      return;
    case 0b00000010:
      return;
    case 0b00000100:
      return;
    case 0b00001000:
      return;
    case 0b00010000:
      return;
    case 0b00100000:
      return;
    case 0b01000000:
      return;
    case 0b10000000:
      return;
  }
}
void Shift4(byte data) {
  switch (data) {
    case 0b00000000:
      return;
    case 0b00000001:
      return;
    case 0b00000010:
      return;
    case 0b00000100:
      return;
    case 0b00001000:
      return;
    case 0b00010000:
      return;
    case 0b00100000:
      return;
    case 0b01000000:
      return;
    case 0b10000000:
      return;
  }
}
void whichshift(byte data, byte ShiftRegisterNumber) {
  switch (ShiftRegisterNumber) {
    case 0:
      Shift1(data);
      break;
    case 1:
      Shift2(data);
      break;
    case 2:
      Shift3(data);
      break;
    case 3:
      Shift4(data);
      break;
  }
}
bool areallLow() {
  if (digitalRead(Enter) == 1 || digitalRead(Clear) == 1 || digitalRead(Zero) == 1 || digitalRead(One) == 1) {
    return false;
  }
  for (byte j = 0; j < 2; j++) {
    data[j] = Shift_In(datapin, clockpin, j);
    if (data[j] != 0) {
      return false;
    }
  }
  return true;
}
void loop() {
  digitalWrite(latchpin, LOW);
  delayMicroseconds(20);
  digitalWrite(latchpin, HIGH);
  if (!isPressed) {
    //enter and clear have the highest priority in terms of being pressed first
    if (digitalRead(Enter) == 1) {

    } else if (digitalRead(Clear) == 1) {
    }
    if (digitalRead(Zero) == 1) {

    } else if (digitalRead(One) == 1) {

    } else {
      data[0] = 0;
      data[1] = 0;
      //Serial.print("Bits: ");
      for (byte j = 0; j < 4; j++) {
        data[j] = Shift_In(datapin, clockpin, j);
        if (data[j] != 0) {
          whichshift(data[j], j);
          break;
        }
      }
    }
  } else {
    isPressed = !areallLow();
  }
}