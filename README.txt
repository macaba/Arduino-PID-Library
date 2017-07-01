Arduino PID library modified for Teensy 3.6 hardware float support and real time loop rates.
Warning: the Compute() method no longer checks the elapsed time, so it's absolutely crucial to call this at a consistent fixed rate, and to tell the library what this rate is with SetSampleTime() in order for Ki and Kd to be correct.

***************************************************************
* Arduino PID Library - Version 1.2.1
* by Brett Beauregard <br3ttb@gmail.com> brettbeauregard.com
*
* This Library is licensed under the MIT License
***************************************************************

 - For an ultra-detailed explanation of why the code is the way it is, please visit: 
   http://brettbeauregard.com/blog/2011/04/improving-the-beginners-pid-introduction/

 - For function documentation see:  http://playground.arduino.cc/Code/PIDLibrary
