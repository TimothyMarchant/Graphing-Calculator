The purpose of this project is to create a graphing calculator by using an Arduino Mega 2560 (originally for Uno/mini, but there isn't enough ram on them for this kind of application)
The calculator can evaulate most expressions you type in.  This includes only polynomials and numbers.
The display I am using is the SSD1306 OLED screen (the size for reference is 0.96 inches).  It is 128x64.
Graphing will be accomplished by drawing lines (via the graphing library made by adafruit) on the OLED over a sequence of points.  
Polynomials of integer powers (both positive and negative) can be graphed, although if your function grows too slowly it may not be able to finished drawing it (it graphs only 100 points).
Also if you type in functions with slow growth they may also appear to not be increasing and look like a straight line.
