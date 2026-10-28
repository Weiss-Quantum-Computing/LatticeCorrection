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

const char changeToRemote[]  =  "MR\r\n";            // change to remote mode
const char setForwardAmp[]   =  "1SU+50\r\n";        // set the forward step amplitude
const char setBackwardAmp[]  =  "1SU-50\r\n";        // set the backward step amplitude
const string moveCmd         =  "1PR";               // relative move, always on axis 1 of the selected channel
const string changeChannel   =  "CC";                // change channel
const string endLine         =  "\r\n";              // CRLF, command terminator

/* Paths, COM port and setpoints: defaults, each can be overridden in parameters.txt (see readParameter) */
const string paraFile        =  "parameters.txt";    // looked for in the current folder
string herald                =  "C:/Users/Yang/Documents/Data/CameraControl/herald.txt"; // written by CameraGUI after each run
string logF                  =  "C:/Users/Yang/Documents/Data/CameraControl/LClog.txt";  // last measurement (running average)
string flogDir               =  "C:/Users/Yang/Documents/Data/latticelog";               // folder for the flog-*.txt drift logs
string stepsFile             =  "LCsteps.txt";       // accumulated steps per axis, kept across restarts
string comPort               =  "COM6";
double setpoint[3]           =  {0.0, 0.0, 0.0};     // x, y in px; z in um, compared with deltaZ + Objective Position

const double alpha[]         =  {0.3, 0.3, 0.3}; //{0.3,0.3,0.5}; //proportional gain
const double gammaI[]        =  {0.6, 0.6, 0.5}; //{0.6,0.6,0.4}; //integral gain
const double decayT[]        =  {150, 150, 150}; //{ 150, 150, 150}; //decay time const in unit of "s"
const double zWindow         =   2.0; // z is used only within this distance (um) of the z setpoint
const int threshold          =   1000; //boundary of the piezo movement
const int maxsteps[]         =  {4000, 1300, 2000}; // Numbers of steps between boundaries in each direction (minus some buffer)
const double conv[]          =  {33.59, 16.72, 25}; // pixels to steps (x, y); microns to steps (z, was 38.00 before 20120903)
const int limit[]            =  {75, 40, 45}; // maximum number of steps allowed per cycle
const int manualLimit        =   5000; // largest manual move accepted from the console

#endif /* CONSTANT_H_ */
