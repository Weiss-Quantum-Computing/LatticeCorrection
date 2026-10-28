/*
 * constant.h
 *
 *  Created on: Feb 17, 2012
 *      Author: Yang
 */

#ifndef CONSTANT_H_
#define CONSTANT_H_
#include <stdint.h>
#include <math.h>
#include <string>
using namespace std;

const char version[]         =  "VE\r\n";            // firmware version
const char ampPlus[]         =  "1SU50\r\n";         // set the forward step amplitude
const char changeToRemote[]  =  "MR\r\n";            // change to remote mode
const char tellError[]       =  "TE\r\n";            // catch any error
const string move            =  "1PR"; 		     // relative move, always on one of the axes (default: 1)
const string changeChannel   =  "CC";		     // change channel
const char setForwardAmp[]	     =  "1SU+50\r\n";                // set the step amplitude
const char setBackwardAmp[]	     =  "1SU-50\r\n";                // set the step amplitude
const string endLine         =  "\r\n";              //CRLF, command terminator
const string herald	     =  "C:/Users/Yang/Documents/Data/CameraControl/herald.txt"; //herald of the measurement done
const string logF	     =  "C:/Users/Yang/Documents/Data/CameraControl/LClog.txt"; //herald of the measurement done
const string para	     =  "C:/Users/Yang/Documents/Eclipse/LatticeCorrection/Release/parameters.txt"; //herald of the measurement done
const string syslog	     =  "C:/Users/Yang/Documents/Eclipse/LatticeCorrection/Release/log.txt"; //herald of the measurement done

const double alpha[]           =  {0.3, 0.3, 0.3}; //{0.3,0.3,0.5}; //proportional gain
const double gamma[]           =  {0.6, 0.6, 0.5}; //{0.6,0.6,0.4}; //integral gain
const double decay[]           =  {150, 150, 150}; //{ 150, 150, 150}; //decay time const in unit of "s"
const short  amp	     =   10; //piezo step amplitude
const double xS	             =   0.0; //set position
const double yS	             =   0.0; //set position
const double zS	             =   0.0; //set position
const int threshold          =   1000; //boundary of the piezo movement
const int maxsteps[]          = {4000, 1300, 2000}; // Numbers of steps between boundaries in each direction (minus some buffer)
const double xconv		= 33.59; // conversion from pixels to steps
const double yconv		= 16.72;
const double zconv		= 25;//38.00; // conversion from microns to steps //modified on 20120903
const int xlimit                = 75; // maximum number of steps allowed per cycle
const int ylimit                = 40;
const int zlimit                = 45;

#endif /* CONSTANT_H_ */
