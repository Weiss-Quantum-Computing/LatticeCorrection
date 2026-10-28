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
#include <string>
#include <stdint.h>
#include <math.h>
#include "constant.h"
#include <fstream>
#include <stdio.h>
#include <time.h>
using namespace std;

int InitCom(HANDLE);

void PID(double*,double*,double*, double*,double*,double*,double, double *); //apply algorithm
void SetCom(HANDLE,double *);   //set values to each channels
void parseTime(string, struct tm *); //parse string to formated time
void openLog(struct tm*, double*, double*, double*); //open log file and load most recent measurement
void saveLog(struct tm*, double*, double*, double*); //open log file and load most recent measurement
bool checkHerald(struct tm* ,double*,double*,double*, double*); //check herald file and updates data
bool checkRange (int *);  //check whether the pizeo step is within range
void readParameter(double*);
int recenter(int); // recenter the etalon on one axis

/*------------------------------------------------------------------------------------------------------*/
void openLog(struct tm* ts, double* x, double* y, double* z)
    {
    ifstream oLog;
    string timeStamp;
    char comments[255];
    double temp[6];
    oLog.open(logF.c_str());
    oLog.getline(comments,253,'\n');
    timeStamp=comments;
    oLog >> temp[0] >> temp[1] >> temp[2] >> temp[3] >> temp[4] >> temp[5];
    oLog.close();
    // Now check the values to see if they're sane
    parseTime(timeStamp,ts);
    cout<<"last measurement: "<<timeStamp<<endl;
    struct tm tNow;
    parseTime(timeStamp,&tNow);
    double t= difftime(mktime(&tNow),mktime(ts));
    cout<<"time elapsed: "<<t<<endl;
    if(temp[0]<100&&temp[2]<100&&temp[1]>1e-4&&temp[1]<10&&temp[3]>1e-4&&temp[3]<10&&t<24*3600)
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
	cout << "Bad values in log file!" << endl;
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
int InitCom(HANDLE hPort)
    {
	int err = 0; //error flag
	DCB dcb;
	GetCommState(hPort, &dcb);
	DWORD written=0;
	dcb.BaudRate=921600;
	dcb.ByteSize=8;
	dcb.Parity=0;
	dcb.StopBits=0;
	dcb.fDtrControl=0x00;  // flow control: none
	dcb.fRtsControl=0x00; //flow control:none
	SetCommState(hPort, &dcb);
	WriteFile(hPort, &changeToRemote, 4, &written, NULL); //change to remote mode
	if(written!=4){cerr<<"Fail to initialize COM port"; err =+ 1;}
	WriteFile(hPort, &setForwardAmp, 8, &written, NULL); //change to remote mode
	if(written!=8){cerr<<"Fail to set forward amplitude"; err =+ 2;}
	WriteFile(hPort, &setBackwardAmp, 8, &written, NULL); //change to remote mode
	if(written!=8){cerr<<"Fail to set backward amplitude"; err =+ 4;}
	return err;
    }
/*------------------------------------------------------------------------------------------------------*/
void parseTime(string timeStamp, struct tm *ts)
    {
	int year, month, day, hour, min, sec;
	sscanf(timeStamp.c_str(),"%d-%d-%d-%d:%d:%d", &year,&month,&day,&hour,&min,&sec);

	(*ts).tm_year = year-1900;
	(*ts).tm_mon  = month-1;
	(*ts).tm_mday = day;
	(*ts).tm_hour = hour;
	(*ts).tm_min  = min;
	(*ts).tm_sec  = sec;
	(*ts).tm_isdst= -1; //ignore daylight savings issues
	//mktime(ts);

    }
/*------------------------------------------------------------------------------------------------------*/
bool checkHerald(struct tm* tOld, double* x, double* y, double* z, double *t)
    {

    ifstream hOpen;
    string timeStamp;
    char comments[255];
    hOpen.open(herald.c_str());
    if(hOpen.is_open()){
	double temp[6];
	hOpen.getline(comments,255,'\n');
	timeStamp=comments;
	    hOpen >> temp[0] >> temp[1] >> temp[2] >> temp[3] >> temp[4] >> temp[5];
	    hOpen.close();
	    // Now check the values to see if they're sane
	    if(temp[0]<100&&temp[2]<100&&temp[1]>1e-4&&temp[1]<10&&temp[3]>1e-4&&temp[3]<10)
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
	cout<<"current measurement: "<<timeStamp<<endl;	//for (short i=0;i<2;i++) {cout<<x[i]<<"\t"<<y[i]<<"\t"<<z[i]<<endl;}

	struct tm tNow;
	parseTime(timeStamp,&tNow);
	*t= difftime(mktime(&tNow),mktime(tOld));
	cout<<"time elapsed: "<<*t<<endl;
	*tOld=tNow;



	return true;
	}
    else
	{
	return false;
	}
    }
/*------------------------------------------------------------------------------------------------------*/

void readParameter(double * para)
    {
    ifstream read;


    }

/*------------------------------------------------------------------------------------------------------*/
void PID(double* xl,double* yl,double* zl, double* xc,double* yc,double* zc, double t, double *c)
    {
    bool coerceflag = false; // tells if the step size exceeds limits

    cout<<"position measurement: ";
    cout<< fixed << setprecision (3)<<xc[0]<<"+/-"<<xc[1]<<" "<<yc[0]<<"+/-"<<yc[1]<<" "<<zc[0]<<"+/-"<<zc[1]<<endl;

    bool errorFlag=true;
    if(abs(xc[0])<5&&abs(yc[0])<5&&xc[1]>1e-4&&xc[1]<10&&yc[1]>1e-4&&yc[1]<10) //Sanity check on input values
	{
	errorFlag=false;
	}
   /* else
	{
	cout << "Bad position data on X and Y!" << endl;
	}
   */

    double beta[3];
	for (short i=0;i<3;i++)
	    {beta[i]=exp(-t/decay[i]);}


	/*Z correction*/
	if(t!=0&&abs(zc[0]-zS)<2&&zc[1]>1e-4&&zc[1]<5)  //recall the absolute position for z contains objective position, which lies within 0~10
	    {
	    double temp;
	    //proportional gain
	    c[2] = -alpha[2]*(zc[0]-zS);
	    //calculate running average for integral gain
	    //temp=1/((1-beta[2])/zc[1]/zc[1]+beta[2]/zl[1]/zl[1]);                       //uncertainty
	    //zc[0]=temp*((1-beta[2])/zc[1]/zc[1]*zc[0]+beta[2]/zl[1]/zl[1]*zl[0]); //update value
	    //zc[1]=sqrt(temp);							//update uncertainty

	    // New algorithm 20120420, fixed an algebra mistake in derivation
	    // that would cause errors when t << decay
	    temp = 1/(1+zc[1]*zc[1]/zl[1]/zl[1]*beta[2]); // weights
	    zc[0] = temp*zc[0]+(1-temp)*zl[0]; // average
	    zc[1] = sqrt(temp*temp*zc[1]*zc[1]+(1-temp)*(1-temp)*zl[1]*zl[1]); // uncertainty

	    //integral gain
	    c[2] += -gamma[2]*(zc[0]-zS);
	    //change to step
	    c[2] *= zconv;//38;

	    //coerce
	    if (c[2]>zlimit)
		{
		c[2] = zlimit;
		coerceflag = true;
		}
	    if (c[2]< -zlimit)
		{
		c[2] = zlimit;
		coerceflag = true;
		}

	    //update running average
	    zl[0]=zc[0];zl[1]=zc[1];

	    }
	else
	    {
	    cout << "Bad position data on Z OR no update." << endl;
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
	c[0] = -alpha[0]*(xc[0]-xS);
	c[1] = -alpha[1]*(yc[0]-yS);


	//calculate running average for integral gain
	//temp=1/((1-beta[0])/xc[1]/xc[1]+beta[0]/xl[1]/xl[1]);                       //uncertainty
	//xc[0]=temp*((1-beta[0])/xc[1]/xc[1]*xc[0]+beta[0]/xl[1]/xl[1]*xl[0]); //update value
	//xc[1]=sqrt(temp);                                         //update uncertainty
	//temp=1/((1-beta[1])/yc[1]/yc[1]+beta[1]/yl[1]/yl[1]);                       //uncertainty
	//yc[0]=temp*((1-beta[1])/yc[1]/yc[1]*yc[0]+beta[1]/yl[1]/yl[1]*yl[0]); //update value
	//yc[1]=sqrt(temp);                                         //update uncertainty

	// New algorithm 20120420, fixed an algebra mistake in derivation
        // that would cause errors when t << decay
	temp = 1/(1+xc[1]*xc[1]/xl[1]/xl[1]*beta[0]); // weights
	xc[0] = temp*xc[0]+(1-temp)*xl[0]; // average
	xc[1] = sqrt(temp*temp*xc[1]*xc[1]+(1-temp)*(1-temp)*xl[1]*xl[1]); // uncertainty

	temp = 1/(1+yc[1]*yc[1]/yl[1]/yl[1]*beta[1]); // weights
	yc[0] = temp*yc[0]+(1-temp)*yl[0]; // average
	yc[1] = sqrt(temp*temp*yc[1]*yc[1]+(1-temp)*(1-temp)*yl[1]*yl[1]); // uncertainty


	//integral gain
	c[0] += -gamma[0]*(xc[0]-xS);
	c[1] += -gamma[1]*(yc[0]-yS);


	//change to step
	c[0] *= xconv;//310/9.23;
	c[1] *= yconv;//160/9.57;

	    //coerce
	    if (c[0]>xlimit)
		{
		c[0] = xlimit;
		coerceflag = true;
		}
	    if (c[0]< -xlimit)
		{
		c[0] = xlimit;
		coerceflag = true;
		}
	    if (c[1]>ylimit)
		{
		c[1] = ylimit;
		coerceflag = true;
		}
	    if (c[1]< -ylimit)
		{
		c[1] = ylimit;
		coerceflag = true;
		}



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
	for(short i=0;i<2;i++)
	    {
	    c[i]=0; // corrections equal to zero for fail
	    // don't replace the running average with the measurement,
	    // but do "age" it by increasing the uncertainty
	    xl[1] = xl[1]/beta[0];
	    yl[1] = yl[1]/beta[1];

	    }
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
void SetCom(HANDLE hPort,double * cRaw)   //set values to each channels
    {
    int c[3]; for (short i=0;i<3;i++){c[i]=(int)rint(cRaw[i]);}

    cout<<"correction: ";
    cout << c[0] << " "<< c[1] << " "<< c[2] << endl;

    //c[0]=1;c[1]=20;c[2]=10;


/*testing
       string DATA=move;
       char step[5];
       itoa(10,step,10);
       DATA.append(step);
       DWORD written=0;
    	DATA.append(endLine);
    	char data[DATA.size()];
    	DATA.copy(data,DATA.size(),0);
    	  WriteFile(hPort, &data, DATA.size(), &written, NULL);
    	  cout<<"size is "<<DATA.size()<<endl;
    	  cout<<data<<endl;
*/


	{
    DWORD written=0;
    char channelIndex[1];
    char step[2];
//    char amp[2];
//    char errorCode[3];
    for(short i=0;i<3;i++)
	{
	/*change channel*/
	itoa(i+1,channelIndex,10);//int to string, base 10
	string channel=changeChannel;
	channel.append(channelIndex);
	channel.append(endLine);
	char dataC[channel.size()];
	channel.copy(dataC,channel.size());
	WriteFile(hPort, &dataC, channel.size(), &written, NULL);if(written!=channel.size()){cerr<<"error in changing channel!"<<endl;}
	cout<<"channel "<< i+1 << " moves " << c[i] << " steps"<<endl;



	/*set amplitudes
	itoa(stepSize[i],amp,10);//int to string, base 10
	string setAmp = amplitude;
	setAmp.append(amp);
	setAmp.append(endLine);
	char dataA[setAmp.size()];
	setAmp.copy(dataA,setAmp.size());
	WriteFile(hPort,&dataA,setAmp.size(),&written,NULL);if(written!=setAmp.size()){cerr<<"error in setting amplitude in "<<i<<" axis"<<endl;}
	cout<<" with amplitude "<< dataA  << endl;
*/
//	WriteFile(hPort, &tellError, 4, &written, NULL);
//	ReadFile(hPort, &errorCode, 3, &written, NULL); cout<<"error code is "<<errorCode<<endl;

	/*write to the 1 axis (default) of each channel*/
	itoa(c[i],step,10);//int to string, base 10
	string motion=move;
	motion.append(step);
	motion.append(endLine);
	char dataM[motion.size()];
	motion.copy(dataM,motion.size());
	WriteFile(hPort,&dataM,motion.size(),&written,NULL);if(written!=motion.size()){cerr<<"error in moving "<<i<<" axis"<<endl;}


	Sleep(100);
	}
    }

    }

int recenter(int axis)
    {
    /* Function to recenter the etalons,
     * called when
     * the etalon goes out of bounds.
     */

    char channelIndex[1]; // name of channel
    char step[6]; // text number of steps
    int err=0; // error flag
    DWORD written=0;
    /* The goal is to move the etalon to one edge and then approximately half way back. */
    HANDLE hPort=CreateFile("COM6", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hPort==INVALID_HANDLE_VALUE)	{cerr<<"Fail to open COM port"<<endl;}
	        err =+ InitCom(hPort);

	        /*change channel*/
	        itoa(axis+1,channelIndex,10);//int to string, base 10
		string channel=changeChannel;
		channel.append(channelIndex);
		channel.append(endLine);
		char dataC[channel.size()];
		channel.copy(dataC,channel.size());
		WriteFile(hPort, &dataC, channel.size(), &written, NULL);if(written!=channel.size()){cerr<<"error in changing channel!"<<endl;err =+ 8;}
		cout<<"Recentering channel "<< axis+1 << " moves " << maxsteps << " steps"<<endl;

		// move to end
		itoa(-3*maxsteps[axis],step,10);//int to string, base 10
		string motion=move;
		motion.append(step);
		motion.append(endLine);
		char dataM[motion.size()];
		motion.copy(dataM,motion.size());
		WriteFile(hPort,&dataM,motion.size(),&written,NULL);if(written!=motion.size()){cerr<<"error in moving "<<axis<<" axis"<<endl; err =+ 16;}

		// move back to center
		itoa(maxsteps[axis],step,10);//int to string, base 10
		motion=move;
		motion.append(step);
		motion.append(endLine);
		 //dataM[motion.size()];
		motion.copy(dataM,motion.size());
		WriteFile(hPort,&dataM,motion.size(),&written,NULL);if(written!=motion.size()){cerr<<"error in moving "<<axis<<" axis"<<endl; err =+ 32;}

	   CloseHandle(hPort);

    return err;
    }


#endif /* FUNCTION_H_ */
