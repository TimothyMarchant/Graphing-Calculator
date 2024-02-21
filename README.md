The purpose of this project is to create a graphing calculator by using an Arduino Mega 2560 (originally for Uno/mini, but there isn't enough ram on them for this kind of application)
The calculator can evaulate most expressions you type in.  It only includes arithmetic operators with only real numbers that can be typed.
Currently you can't do things like sin, exponents of noninteger powers, logs, etc.  These require taylor series to approximate. This will be added later.
The next planned feature is letting the calculator hold in a function that can repeatly evaluate an expression with variables (single-variable not a multi-variable function), and form tables from the function.  Polynomials are of main concern right now.
The display I am using is the SSD1306 OLED screen (the size for reference is 0.96 inches).
Graphing will be accomplished by drawing lines (via the graphing library made by adafruit) on the OLED over a sequence of points  and also (probably) based on some sort of zoom factor.
Hopefully by the end polynomials (of integer powers, maybe non integer powers), square roots (maybe cube roots too), trig functions and Exponetial functions are able to be graphed.
