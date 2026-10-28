//============================================================================
// Name        : LatticeCorrection.cpp
// Author      : Yang
// Version     : 1.0
// Copyright   : 
// Description : 2012/2, for lattice drift correction.
// Log:
// 2012/3/12 Math checked

//============================================================================

#include <windows.h>
#include <iostream>
#include <string>
#include "constant.h"
#include "function.h"
#include <stdio.h>
#include <time.h>
using namespace std;

int main(int, char*[]);


//TODO: error handling, set different step size, set accumulated log


int main(int argc, char* argv[])

    {
    double xc[2],yc[2],zc[2];    // current data+uncertainty (x,y in px, z in um)
    double xl[2],yl[2],zl[2];    // last data+uncertainty
    double c[3];                    // correction
    struct tm logTime;           // time of last measurement
    double tao;                  // time elapsed since last measurement
    int steps[3]={0,0,0}; // total number of steps taken in each direction
    int rangeflag[3]={0,0,0}; // Flag for axis out of range = +1 for pos dir., -1 for neg dir.
    bool anyrangeflag = false;
    char timestring[40];
    const string timefmt = "%Y-%m-%d-%H:%M:%S";
    char flogname[80];

    openLog(&logTime, xl,yl,zl); //open and load from last measurement
    strftime(flogname,80,"C:/Users/Yang/Documents/Data/latticelog/flog-%Y%m%d-%H%M%S.txt",&logTime);
    ofstream feedbacklog;
    feedbacklog.open(flogname);
    feedbacklog << "Lattice drift log.  x and y in pixels, z in um." << endl;
    feedbacklog << "#Time\txc\txcs\tyc\tycs\tzc\tzcs\txl\txls\tyl\tyls\tzl\tzls\tcx\tcy\tcz\tsx\tsy\tsz" << endl;
    feedbacklog.close();
    while(1)
	{
	if(checkHerald(&logTime, xc,yc,zc, &tao)) //if herald file exists, update and proceed; if not, wait until it appears
	    {
	    strftime(timestring,40,timefmt.c_str(),&logTime);
	    if(tao>24*3600)  //neglecting the data from a day ago
		{
		xl[0]=0;xl[1]=1e5;
		yl[0]=0;yl[1]=1e5;
		zl[0]=0;zl[1]=1e5;
		}
	    /* Write current position and uncertainty in position to log: */
	    feedbacklog.open(flogname,ios::app);
	    feedbacklog << timestring << "\t" << xc[0] <<"\t" << xc[1] << "\t" << yc[0] << "\t"
		    << yc[1] << "\t" << zc[0] << "\t" << zc[1] << "\t";
	    /*apply algorithm, and update old data with current one*/
	    PID(xl,yl,zl,xc,yc,zc,tao,c);
	    int cc[3]; for (short i=0;i<3;i++){cc[i]=(int)rint(c[i]);}
	    /* Write running average and uncertainty to log: */
	    feedbacklog << xc[0] <<"\t" << xc[1] << "\t" << yc[0] << "\t"
		    << yc[1] << "\t" << zc[0] << "\t" << zc[1] << "\t";
	    if(checkRange(cc))
		{
		cout<<"correction out of range!\a\a"<<endl;
		feedbacklog << 0 << "\t"<< 0 << "\t" << 0<< "\t"
			<< steps[0] << "\t" << steps[1] << "\t" << steps[2] << endl;
		}
	    else
		{
		// update step count:
		steps[0]+=c[0];
		steps[1]+=c[1];
		steps[2]+=c[2];
		/* Write correction and cumulative correction to log: */
		feedbacklog << cc[0] << "\t"<< cc[1] << "\t" << cc[2] << "\t"
			<< steps[0] << "\t" << steps[1] << "\t" << steps[2] << endl;

		cout << "Accumulated steps: " << steps[0] << ", " << steps[1] << ", " << steps[2] << endl;

		for(int j = 0; j<3;j++)
		    {
		    if(steps[j]>maxsteps[j]&&rangeflag[j]==0)
			{
			rangeflag[j]=1;
			anyrangeflag = true;
			// recenter(j);
			}
		    else if(steps[j]<-maxsteps[j]&&rangeflag[j]==0)
			{
			rangeflag[j]=-1;
			anyrangeflag = true;
			}
		    }

		if (anyrangeflag)
		    {
		    cout << "WARNING: Etalon out of range!!\a\a\a" << endl;
		    }

		/*open and set COM port*/
		HANDLE hPort=CreateFile("COM6", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hPort==INVALID_HANDLE_VALUE)	{cerr<<"Fail to open COM port"<<endl;break;}
		InitCom(hPort);
		SetCom(hPort,c);
		CloseHandle(hPort);

		/*remove herald file, and save log*/
		if(remove(herald.c_str())!=0){cout<<"error deleting file";}
		saveLog(&logTime, xc,yc,zc);
		feedbacklog.close();
		}
	    }
	else
	    { Sleep(1000);} //sleep time in ms
	}

    cout<<"------------------------------------------------"<<endl;
    cout<<"---------------CHECK COM PORT-------------------"<<endl;
    cout<<"------------------------------------------------"<<endl;

    return 0;
    }

