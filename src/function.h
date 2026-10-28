/*
 * function.h
 *
 *  Created on: Feb 17, 2012
 *      Author: Yang
 */

#ifndef FUNCTION_H_
#define FUNCTION_H_
#include <windows.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <stdint.h>
#include <math.h>
#include "constant.h"
#include <fstream>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
using namespace std;

enum HeraldState {HERALD_NONE, HERALD_OK, HERALD_INCOMPLETE};

HANDLE openCom(); //open and initialize the COM port, INVALID_HANDLE_VALUE on failure
int InitCom(HANDLE);
int SetCom(HANDLE,int *);   //set values to each channels
int moveChannel(HANDLE,int,int); //relative move of one channel
void PID(double*,double*,double*, double*,double*,double*,double, double *); //apply algorithm
bool parseTime(string, struct tm *); //parse string to formated time
void openLog(struct tm*, double*, double*, double*); //open log file and load most recent measurement
void saveLog(struct tm*, double*, double*, double*); //save the running average as the most recent measurement
HeraldState checkHerald(struct tm* ,double*,double*,double*, double*); //check herald file and updates data
bool checkRange (int *);  //check whether the pizeo step is within range
void readParameter(); //override paths, COM port and setpoints from parameters.txt
void loadSteps(int *); //accumulated steps per axis, kept across restarts
void saveSteps(int *);
void warnRange(int *); //warn about axes that have moved further than maxsteps

/*------------------------------------------------------------------------------------------------------*/
void openLog(struct tm* ts, double* x, double* y, double* z)
    {
    ifstream oLog;
    string timeStamp;
    double temp[6];
    bool ok=false;
    oLog.open(logF.c_str());
    if(oLog.is_open())
	{
	getline(oLog,timeStamp);
	ok=!(oLog >> temp[0] >> temp[1] >> temp[2] >> temp[3] >> temp[4] >> temp[5]).fail();
	oLog.close();
	}
    time_t now=time(NULL);
    double t=1e10;
    if(ok&&parseTime(timeStamp,ts))
	{
	cout<<"last measurement: "<<timeStamp<<endl;
	t= difftime(now,mktime(ts));
	cout<<"time elapsed: "<<t<<endl;
	}
    else
	{
	ok=false;
	cout<<"No readable last measurement in "<<logF<<endl;
	*ts=*localtime(&now);
	}
    // Now check the values to see if they're sane
    if(ok&&fabs(temp[0])<100&&fabs(temp[2])<100&&temp[1]>1e-4&&temp[1]<10&&temp[3]>1e-4&&temp[3]<10&&t<24*3600)
	{
	// it's safe to use these values
	x[0] = temp[0];
	x[1] = temp[1];
	y[0] = temp[2];
	y[1] = temp[3];
	z[0] = temp[4];
	z[1] = temp[5];
	}
    else
	{
	cout << "Bad or old values in log file, starting without a running average" << endl;
	// overwrite values with some safe defaults
	x[0] = 0; x[1] = 1e10;
	y[0] = 0; y[1] = 1e10;
	z[0] = 0; z[1] = 1e10;
	}


    }
/*------------------------------------------------------------------------------------------------------*/
void saveLog(struct tm* ts, double* x, double* y, double* z)
    {
    ofstream oLog;
    oLog.open(logF.c_str());
    char buf[80];
    strftime(buf, sizeof(buf), "%Y-%m-%d-%H:%M:%S", ts);
    string timeStamp=buf;
    oLog << timeStamp <<endl;
    oLog << x[0] << "\t" << x[1] << "\t" << y[0] << "\t" << y[1] << "\t" << z[0] << "\t" << z[1] <<endl;
    oLog.close();

    }
/*------------------------------------------------------------------------------------------------------*/
HANDLE openCom()
    {
    HANDLE hPort=CreateFileA(comPort.c_str(), GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hPort==INVALID_HANDLE_VALUE)	{cerr<<"Fail to open "<<comPort<<endl; return hPort;}
    if(InitCom(hPort)!=0)
	{
	CloseHandle(hPort);
	return INVALID_HANDLE_VALUE;
	}
    return hPort;
    }
/*------------------------------------------------------------------------------------------------------*/
int InitCom(HANDLE hPort)
    {
	int err = 0; //error flag
	DCB dcb;
	memset(&dcb,0,sizeof(dcb));
	dcb.DCBlength=sizeof(dcb);
	if(!GetCommState(hPort, &dcb)){cerr<<"Fail to read COM port settings"<<endl; return 1;}
	DWORD written=0;
	dcb.BaudRate=921600;
	dcb.ByteSize=8;
	dcb.Parity=0;
	dcb.StopBits=0;
	dcb.fDtrControl=0x00;  // flow control: none
	dcb.fRtsControl=0x00; //flow control:none
	if(!SetCommState(hPort, &dcb)){cerr<<"Fail to set COM port settings"<<endl; return 1;}
	WriteFile(hPort, changeToRemote, sizeof(changeToRemote)-1, &written, NULL); //change to remote mode
	if(written!=sizeof(changeToRemote)-1){cerr<<"Fail to initialize COM port"<<endl; err += 2;}
	WriteFile(hPort, setForwardAmp, sizeof(setForwardAmp)-1, &written, NULL); //set forward amplitude
	if(written!=sizeof(setForwardAmp)-1){cerr<<"Fail to set forward amplitude"<<endl; err += 4;}
	WriteFile(hPort, setBackwardAmp, sizeof(setBackwardAmp)-1, &written, NULL); //set backward amplitude
	if(written!=sizeof(setBackwardAmp)-1){cerr<<"Fail to set backward amplitude"<<endl; err += 8;}
	return err;
    }
/*------------------------------------------------------------------------------------------------------*/
bool parseTime(string timeStamp, struct tm *ts)
    {
	int year, month, day, hour, min, sec;
	if(sscanf(timeStamp.c_str(),"%d-%d-%d-%d:%d:%d", &year,&month,&day,&hour,&min,&sec)!=6) return false;

	memset(ts,0,sizeof(*ts));
	(*ts).tm_year = year-1900;
	(*ts).tm_mon  = month-1;
	(*ts).tm_mday = day;
	(*ts).tm_hour = hour;
	(*ts).tm_min  = min;
	(*ts).tm_sec  = sec;
	(*ts).tm_isdst= -1; //ignore daylight savings issues
	return true;
    }
/*------------------------------------------------------------------------------------------------------*/
HeraldState checkHerald(struct tm* tOld, double* x, double* y, double* z, double *t)
    {

    ifstream hOpen;
    hOpen.open(herald.c_str());
    if(!hOpen.is_open()) {return HERALD_NONE;}
    stringstream buf;
    buf << hOpen.rdbuf();
    hOpen.close();
    string content=buf.str();

    // CameraGUI writes a time stamp line and then one line of six numbers. Until that second line
    // ends in a newline the file may still be being written: leave it for the next poll.
    if(content.empty()||content[content.size()-1]!='\n') {return HERALD_INCOMPLETE;}
    istringstream hRead(content);
    string timeStamp;
    double temp[6];
    struct tm tNow;
    getline(hRead,timeStamp);
    if((hRead >> temp[0] >> temp[1] >> temp[2] >> temp[3] >> temp[4] >> temp[5]).fail()||!parseTime(timeStamp,&tNow))
	{return HERALD_INCOMPLETE;}

    // Now check the values to see if they're sane
    if(fabs(temp[0])<100&&fabs(temp[2])<100&&temp[1]>1e-4&&temp[1]<10&&temp[3]>1e-4&&temp[3]<10)
	{
	// it's safe to use these values
	x[0] = temp[0];
	x[1] = temp[1];
	y[0] = temp[2];
	y[1] = temp[3];
	z[0] = temp[4];
	z[1] = temp[5];
	}
    else
	{
	cout << "Bad values in herald file!" << endl;
	// overwrite values with some safe defaults
	x[0] = 0; x[1] = 1e10;
	y[0] = 0; y[1] = 1e10;
	z[0] = 0; z[1] = 1e10;
	}
    cout<<"current measurement: "<<timeStamp<<endl;

    *t= difftime(mktime(&tNow),mktime(tOld));
    cout<<"time elapsed: "<<*t<<endl;
    *tOld=tNow;

    return HERALD_OK;
    }
/*------------------------------------------------------------------------------------------------------*/

void readParameter()
    {
    ifstream read(paraFile.c_str());
    if(!read.is_open())
	{
	cout<<"No "<<paraFile<<" in the current folder, using the built-in defaults"<<endl;
	}
    string line;
    while(read.is_open()&&getline(read,line))
	{
	istringstream in(line);
	string key, value;
	if(!(in>>key)||key[0]=='#') {continue;}
	getline(in>>ws,value); // the rest of the line, so paths may contain spaces
	while(!value.empty()&&isspace((unsigned char)value[value.size()-1])) {value.erase(value.size()-1);}
	if(key=="herald")        {herald=value;}
	else if(key=="log")      {logF=value;}
	else if(key=="flogdir")  {flogDir=value;}
	else if(key=="steps")    {stepsFile=value;}
	else if(key=="comport")  {comPort=value;}
	else if(key=="xS"||key=="yS"||key=="zS")
	    {
	    double v;
	    if((istringstream(value)>>v).fail()) {cout<<"Bad number for "<<key<<" in "<<paraFile<<": "<<value<<endl;}
	    else {setpoint[key[0]-'x']=v;}
	    }
	else {cout<<"Unknown key in "<<paraFile<<": "<<key<<endl;}
	}
    cout<<"herald file:  "<<herald<<endl;
    cout<<"last-value log: "<<logF<<endl;
    cout<<"drift logs:   "<<flogDir<<endl;
    cout<<"step counters: "<<stepsFile<<endl;
    cout<<"COM port:     "<<comPort<<endl;
    cout<<"setpoints:    x "<<setpoint[0]<<" px, y "<<setpoint[1]<<" px, z "<<setpoint[2]
	<<" um (z is used within "<<zWindow<<" um of its setpoint)"<<endl;
    }

/*------------------------------------------------------------------------------------------------------*/
void loadSteps(int *steps)
    {
    ifstream read(stepsFile.c_str());
    int temp[3];
    if(read.is_open()&&!(read>>temp[0]>>temp[1]>>temp[2]).fail())
	{
	for(short i=0;i<3;i++) {steps[i]=temp[i];}
	}
    else
	{
	cout<<"No step counters in "<<stepsFile<<", starting from 0"<<endl;
	for(short i=0;i<3;i++) {steps[i]=0;}
	}
    }

void saveSteps(int *steps)
    {
    ofstream write(stepsFile.c_str());
    write<<steps[0]<<"\t"<<steps[1]<<"\t"<<steps[2]<<endl;
    }

void warnRange(int *steps)
    {
    const char axisName[]="xyz";
    for(short i=0;i<3;i++)
	{
	if(steps[i]>maxsteps[i]||steps[i]<-maxsteps[i])
	    {
	    cout<<"WARNING: "<<axisName[i]<<" has moved "<<steps[i]<<" steps since the counters were zeroed (limit "
		<<maxsteps[i]<<"), it may be near the rail. Type \"m "<<axisName[i]<<" <steps>\" to move it back.\a"<<endl;
	    }
	}
    }

/*------------------------------------------------------------------------------------------------------*/
double clampSteps(double c, int lim, bool *coerced)
    {
    if(c>lim)  {*coerced=true; return lim;}
    if(c<-lim) {*coerced=true; return -lim;}
    return c;
    }

void PID(double* xl,double* yl,double* zl, double* xc,double* yc,double* zc, double t, double *c)
    {
    bool coerceflag = false; // tells if the step size exceeds limits

    cout<<"position measurement: ";
    cout<< fixed << setprecision (3)<<xc[0]<<"+/-"<<xc[1]<<" "<<yc[0]<<"+/-"<<yc[1]<<" "<<zc[0]<<"+/-"<<zc[1]<<endl;

    bool errorFlag=true;
    if(fabs(xc[0])<5&&fabs(yc[0])<5&&xc[1]>1e-4&&xc[1]<10&&yc[1]>1e-4&&yc[1]<10) //Sanity check on input values
	{
	errorFlag=false;
	}

    double beta[3];
	for (short i=0;i<3;i++)
	    {beta[i]=exp(-t/decayT[i]);}


	/*Z correction*/
	if(t!=0&&fabs(zc[0]-setpoint[2])<zWindow&&zc[1]>1e-4&&zc[1]<5)  //recall the absolute position for z contains objective position, which lies within 0~10
	    {
	    double temp;
	    //proportional gain
	    c[2] = -alpha[2]*(zc[0]-setpoint[2]);

	    // New algorithm 20120420, fixed an algebra mistake in derivation
	    // that would cause errors when t << decay
	    temp = 1/(1+zc[1]*zc[1]/zl[1]/zl[1]*beta[2]); // weights
	    zc[0] = temp*zc[0]+(1-temp)*zl[0]; // average
	    zc[1] = sqrt(temp*temp*zc[1]*zc[1]+(1-temp)*(1-temp)*zl[1]*zl[1]); // uncertainty

	    //integral gain
	    c[2] += -gammaI[2]*(zc[0]-setpoint[2]);
	    //change to step
	    c[2] *= conv[2];

	    //coerce
	    c[2] = clampSteps(c[2],limit[2],&coerceflag);

	    //update running average
	    zl[0]=zc[0];zl[1]=zc[1];

	    }
	else
	    {
	    cout << "Bad position data on Z OR no update." << endl;
	    if(t!=0&&zc[1]>1e-4&&zc[1]<5)
		{
		cout << "  z = " << zc[0] << " is more than " << zWindow << " um from the z setpoint " << setpoint[2]
		     << "; zS in " << paraFile << " should match the camera's Objective Position." << endl;
		}
	    c[2]=0;
	    zl[1] = zl[1]/beta[2];
	    if(isnan(zl[1])||isinf(zl[1])!=0) // Check for fault conditions
		{
		zl[0]=0;
		zl[1] = 1e10;
		}
	    }


	/*X & Y correction*/
    if (t!=0&&(!errorFlag))
	{

	double temp;

	//proportional gain
	c[0] = -alpha[0]*(xc[0]-setpoint[0]);
	c[1] = -alpha[1]*(yc[0]-setpoint[1]);

	// New algorithm 20120420, fixed an algebra mistake in derivation
        // that would cause errors when t << decay
	temp = 1/(1+xc[1]*xc[1]/xl[1]/xl[1]*beta[0]); // weights
	xc[0] = temp*xc[0]+(1-temp)*xl[0]; // average
	xc[1] = sqrt(temp*temp*xc[1]*xc[1]+(1-temp)*(1-temp)*xl[1]*xl[1]); // uncertainty

	temp = 1/(1+yc[1]*yc[1]/yl[1]/yl[1]*beta[1]); // weights
	yc[0] = temp*yc[0]+(1-temp)*yl[0]; // average
	yc[1] = sqrt(temp*temp*yc[1]*yc[1]+(1-temp)*(1-temp)*yl[1]*yl[1]); // uncertainty


	//integral gain
	c[0] += -gammaI[0]*(xc[0]-setpoint[0]);
	c[1] += -gammaI[1]*(yc[0]-setpoint[1]);


	//change to step
	c[0] *= conv[0];
	c[1] *= conv[1];

	//coerce
	c[0] = clampSteps(c[0],limit[0],&coerceflag);
	c[1] = clampSteps(c[1],limit[1],&coerceflag);

	//update running average
	for (short i=0;i<2;i++)
	{
	xl[i]=xc[i];
	yl[i]=yc[i];
	}

	}
    else
	{
	cout<< "Bad position data on X and Y OR no updates." << endl;
	c[0]=0; c[1]=0; // corrections equal to zero for fail
	// don't replace the running average with the measurement,
	// but do "age" it by increasing the uncertainty
	xl[1] = xl[1]/beta[0];
	yl[1] = yl[1]/beta[1];
        }
	if (coerceflag)
	    {
	    cout << "Something exceeds step limit.\a"<< endl;
	    coerceflag=false;
	    }
	cout << "running average: ";
	cout<<fixed<<setprecision(3)<<xl[0]<<"+/-"<<xl[1]<<" "<<yl[0]<<"+/-"<<yl[1]<<" "<<zl[0]<<"+/-"<<zl[1]<<endl;

    }
/*------------------------------------------------------------------------------------------------------*/
bool checkRange(int *c)
    {
    for(short i=0; i<3;i++)
	{

        if(c[i]>threshold||c[i]<-1*threshold) // abs doesn't work if c is at INT_MIN (e.g integer version of nan)
            {
            return true;
            }
	}
    return false;
    }
/*------------------------------------------------------------------------------------------------------*/
int moveChannel(HANDLE hPort, int channel, int steps)
    {
    int err=0;
    DWORD written=0;
    char cmd[32];

    /*change channel*/
    int n=snprintf(cmd,sizeof(cmd),"%s%d%s",changeChannel.c_str(),channel,endLine.c_str());
    if(!WriteFile(hPort, cmd, n, &written, NULL)||written!=(DWORD)n){cerr<<"error in changing to channel "<<channel<<endl; err += 1;}

    /*write to the 1 axis (default) of the channel*/
    n=snprintf(cmd,sizeof(cmd),"%s%d%s",moveCmd.c_str(),steps,endLine.c_str());
    if(!WriteFile(hPort, cmd, n, &written, NULL)||written!=(DWORD)n){cerr<<"error in moving channel "<<channel<<endl; err += 2;}
    cout<<"channel "<< channel << " moves " << steps << " steps"<<endl;

    Sleep(100);
    return err;
    }

int SetCom(HANDLE hPort,int * c)   //set values to each channels
    {
    int err=0;
    cout<<"correction: ";
    cout << c[0] << " "<< c[1] << " "<< c[2] << endl;
    for(short i=0;i<3;i++)
	{
	err += moveChannel(hPort,i+1,c[i]);
	}
    return err;
    }


#endif /* FUNCTION_H_ */
