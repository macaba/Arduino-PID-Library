/**********************************************************************************************
 * Arduino PID Library - Version 1.2.1
 * by Brett Beauregard <br3ttb@gmail.com> brettbeauregard.com
 *
 * This Library is licensed under the MIT License
 **********************************************************************************************/

#if ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include <FastFloatPID.h>

/*Constructor (...)*********************************************************
 *    The parameters specified here are those for for which we can't set up
 *    reliable defaults, so we need to have the user set them.
 ***************************************************************************/
FastFloatPID::FastFloatPID(float* Input, float* Output, float* Setpoint,
        float Kp, float Ki, float Kd, int POn, int ControllerDirection)
{
    myOutput = Output;
    myInput = Input;
    mySetpoint = Setpoint;
    inAuto = false;

    FastFloatPID::SetOutputLimits(0, 255);				//default output limit corresponds to
												//the arduino pwm limits

    SampleTimeInSec = 0.1;							//default Controller Sample Time is 0.1 seconds

    FastFloatPID::SetControllerDirection(ControllerDirection);
    FastFloatPID::SetTunings(Kp, Ki, Kd, POn);
}

/*Constructor (...)*********************************************************
 *    To allow backwards compatability for v1.1, or for people that just want
 *    to use Proportional on Error without explicitly saying so
 ***************************************************************************/

FastFloatPID::FastFloatPID(float* Input, float* Output, float* Setpoint,
        float Kp, float Ki, float Kd, int ControllerDirection)
    :FastFloatPID::FastFloatPID(Input, Output, Setpoint, Kp, Ki, Kd, P_ON_E, ControllerDirection)
{

}


/* Compute() **********************************************************************
 *     This, as they say, is where the magic happens.  this function should be called
 *   every time "void loop()" executes.  the function will decide for itself whether a new
 *   pid Output needs to be computed.  returns true when the output is computed,
 *   false when nothing has been done.
 **********************************************************************************/
bool FastFloatPID::Compute()
{
	if(!inAuto) return false;

	/*Compute all the working error variables*/
	float input = *myInput;
	float error = *mySetpoint - input;
	float dInput = (input - lastInput);
	outputSum+= (ki * error);

	/*Add Proportional on Measurement, if P_ON_M is specified*/
	if(!pOnE) outputSum-= kp * dInput;

	if(outputSum > outMax) outputSum= outMax;
	else if(outputSum < outMin) outputSum= outMin;

	/*Add Proportional on Error, if P_ON_E is specified*/
	float output;
	if(pOnE) output = kp * error;
	else output = 0;

	/*Compute Rest of PID Output*/
	output += outputSum - kd * dInput;

	if(output > outMax) output = outMax;
	else if(output < outMin) output = outMin;
	*myOutput = output;

	/*Remember some variables for next time*/
	lastInput = input;
	return true;
}

/* SetTunings(...)*************************************************************
 * This function allows the controller's dynamic performance to be adjusted.
 * it's called automatically from the constructor, but tunings can also
 * be adjusted on the fly during normal operation
 ******************************************************************************/
void FastFloatPID::SetTunings(float Kp, float Ki, float Kd, int POn)
{
   if (Kp<0 || Ki<0 || Kd<0) return;

   pOn = POn;
   pOnE = POn == P_ON_E;

   dispKp = Kp; dispKi = Ki; dispKd = Kd;

   kp = Kp;
   ki = Ki * SampleTimeInSec;
   kd = Kd / SampleTimeInSec;

  if(controllerDirection ==REVERSE)
   {
      kp = (0 - kp);
      ki = (0 - ki);
      kd = (0 - kd);
   }
}

/* SetTunings(...)*************************************************************
 * Set Tunings using the last-rembered POn setting
 ******************************************************************************/
void FastFloatPID::SetTunings(float Kp, float Ki, float Kd){
    SetTunings(Kp, Ki, Kd, pOn); 
}

/* SetSampleTime(...) *********************************************************
 * sets the period, in seconds, at which the calculation is performed
 ******************************************************************************/
void FastFloatPID::SetSampleTime(float newSampleTimeInSec)
{
   if (newSampleTimeInSec > 0)
   {
      float ratio  = (float)newSampleTimeInSec
                      / (float)SampleTimeInSec;
      ki *= ratio;
      kd /= ratio;
      SampleTimeInSec = newSampleTimeInSec;
   }
}

/* SetOutputLimits(...)****************************************************
 *     This function will be used far more often than SetInputLimits.  while
 *  the input to the controller will generally be in the 0-1023 range (which is
 *  the default already,)  the output will be a little different.  maybe they'll
 *  be doing a time window and will need 0-8000 or something.  or maybe they'll
 *  want to clamp it from 0-125.  who knows.  at any rate, that can all be done
 *  here.
 **************************************************************************/
void FastFloatPID::SetOutputLimits(float Min, float Max)
{
   if(Min >= Max) return;
   outMin = Min;
   outMax = Max;

   if(inAuto)
   {
	   if(*myOutput > outMax) *myOutput = outMax;
	   else if(*myOutput < outMin) *myOutput = outMin;

	   if(outputSum > outMax) outputSum= outMax;
	   else if(outputSum < outMin) outputSum= outMin;
   }
}

/* SetMode(...)****************************************************************
 * Allows the controller Mode to be set to manual (0) or Automatic (non-zero)
 * when the transition from manual to auto occurs, the controller is
 * automatically initialized
 ******************************************************************************/
void FastFloatPID::SetMode(int Mode)
{
    bool newAuto = (Mode == AUTOMATIC);
    if(newAuto && !inAuto)
    {  /*we just went from manual to auto*/
        FastFloatPID::Initialize();
    }
    inAuto = newAuto;
}

/* Initialize()****************************************************************
 *	does all the things that need to happen to ensure a bumpless transfer
 *  from manual to automatic mode.
 ******************************************************************************/
void FastFloatPID::Initialize()
{
   outputSum = *myOutput;
   lastInput = *myInput;
   if(outputSum > outMax) outputSum = outMax;
   else if(outputSum < outMin) outputSum = outMin;
}

/* SetControllerDirection(...)*************************************************
 * The PID will either be connected to a DIRECT acting process (+Output leads
 * to +Input) or a REVERSE acting process(+Output leads to -Input.)  we need to
 * know which one, because otherwise we may increase the output when we should
 * be decreasing.  This is called from the constructor.
 ******************************************************************************/
void FastFloatPID::SetControllerDirection(int Direction)
{
   if(inAuto && Direction !=controllerDirection)
   {
	    kp = (0 - kp);
      ki = (0 - ki);
      kd = (0 - kd);
   }
   controllerDirection = Direction;
}

/* Status Funcions*************************************************************
 * Just because you set the Kp=-1 doesn't mean it actually happened.  these
 * functions query the internal state of the PID.  they're here for display
 * purposes.  this are the functions the PID Front-end uses for example
 ******************************************************************************/
float FastFloatPID::GetKp(){ return  dispKp; }
float FastFloatPID::GetKi(){ return  dispKi;}
float FastFloatPID::GetKd(){ return  dispKd;}
int FastFloatPID::GetMode(){ return  inAuto ? AUTOMATIC : MANUAL;}
int FastFloatPID::GetDirection(){ return controllerDirection;}

