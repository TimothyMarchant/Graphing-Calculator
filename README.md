The purpose of this project is to create a graphing calculator by using an Arduino Mega 2560, SN74HC165N shift registers, and the SSD1306 OLED screen (and lots of wires).
The calculator can graph most expressions you type in.  This includes polynomials and trig functions.
It can also evaluate numerical expressions.
The display I am using is the SSD1306 OLED screen (the size for reference is 0.96 inches).  It is 128x64.
Graphing is accomplished by drawing lines (via the graphing library made by adafruit) on the OLED over a sequence of points.  
Polynomials of integer powers (both positive and negative) can be graphed along with the sine function, although if your function grows too slowly it may not be able to finished drawing it (it graphs only 250 points).
Also if you type in functions with slow growth they may also appear to not be increasing and look like a straight line.  Lastly, if you type in a discontinous function it may try to draw a line off the screen this I will fix at some point.
Here are some pictures 
Cubic polynomial x^3-ax^2 for some real constant a (don't recall what constant I used in this one).
![IMG_20240225_011038629](https://github.com/TimothyMarchant/Graphing-Calculator/assets/124601612/c59e066d-73be-47cb-a7a5-e3f5e30cd1e4)

Rational function 1/x
![IMG_20240225_211708719](https://github.com/TimothyMarchant/Graphing-Calculator/assets/124601612/ac478c01-59ec-44bd-997e-0376c636ed1f)

Sine curve (sin(x))
![thumbnail_1000002498](https://github.com/TimothyMarchant/Graphing-Calculator/assets/124601612/1ef7f77f-a9a0-42ca-b33f-b0e9dd211887)

Sin(1/x)
![thumbnail_1000002499](https://github.com/TimothyMarchant/Graphing-Calculator/assets/124601612/dec3e816-026d-4608-91bf-1baeb0bf68af)

Csc(x) (1/sin(x)).
![thumbnail_1000002505](https://github.com/TimothyMarchant/Graphing-Calculator/assets/124601612/07f42294-ebb8-4028-b9db-ee9b300cf22d)
tan(x)
![image](https://github.com/TimothyMarchant/Graphing-Calculator/assets/124601612/3cb6dfd1-2603-4e21-9010-5f44316a87c4)

Normal calculation (yes 1+1 does in fact  equal 2).
![IMG_20240218_003935751](https://github.com/TimothyMarchant/Graphing-Calculator/assets/124601612/3a4dca75-90c8-4467-a8f7-c9bf6855e1fc)

Function menu
![Polynomial](https://github.com/TimothyMarchant/Graphing-Calculator/assets/124601612/a24f2c52-7bfe-43a3-9fcf-13a27010f596)
Result
![Fourth power polynomial](https://github.com/TimothyMarchant/Graphing-Calculator/assets/124601612/5ebcaa57-69c0-4db9-8a73-64457bb91013)

The breadboards I used to actually wire the arduino
![BreadBoardDesign](https://github.com/TimothyMarchant/Graphing-Calculator/assets/124601612/2d941e90-329a-437b-9cfb-a9bb0e16c4bb)

Also here are the FRAM chips I was talking about in the comments of the code (the picture is misleading these things are very small).
![IMG_20240221_124639701](https://github.com/TimothyMarchant/Graphing-Calculator/assets/124601612/514858a1-34c5-459e-acd9-0fe5812cb489)
These (when I can solder them to a breakout board) will be used to save typed in functions.

Hopefully I can make a video showing everything off in full and also an actual schematic.  Thanks for viewing my project.
