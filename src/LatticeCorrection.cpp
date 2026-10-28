//============================================================================
// Name        : LatticeCorrection.cpp
// Author      : Yang
// Version     : 1.0
// Copyright   :
// Description : 2012/2, for lattice drift correction.
// Log:
// 2012/3/12 Math checked
// 2026/10 Console commands for manual plate moves; step counters kept across restarts

//============================================================================

#include <windows.h>
#include <conio.h>
#include <iostream>
#include <string>
#include "constant.h"
#include "function.h"
#include <stdio.h>
#include <time.h>
using namespace std;

int main(int, char*[]);
void printHelp();
bool runCommand(string, int*, double*, double*, double*, string); // false means quit


int main(int argc, char* argv[])

    {
    double xc[2],yc[2],zc[2];    // current data+uncertainty (x,y in px, z in um)
    double xl[2],yl[2],zl[2];    // running average+uncertainty
    double c[3];                    // correction
    int cc[3];                      // correction in whole steps, as sent
    struct tm logTime;           // time of last measurement
    double tao;                  // time elapsed since last measurement
    int steps[3]; // total number of steps taken in each direction since the counters were last zeroed
    char timestring[40];
    const string timefmt = "%Y-%m-%d-%H:%M:%S";
    char fname[40];
    string flogname;
    string input;                // console command being typed
    int incomplete=0;            // polls in a row that found an unreadable herald file
    bool running=true;

    readParameter();
    loadSteps(steps);
    openLog(&logTime, xl,yl,zl); //open and load from last measurement
    time_t now=time(NULL);
    strftime(fname,40,"flog-%Y%m%d-%H%M%S.txt",localtime(&now)); // named by start time, so a restart never overwrites a log
    flogname=flogDir+"/"+fname;
    ofstream feedbacklog;
    feedbacklog.open(flogname.c_str());
    if(!feedbacklog.is_open()) {cerr<<"Cannot create drift log "<<flogname<<endl;}
    feedbacklog << "Lattice drift log.  x and y in pixels, z in um." << endl;
    feedbacklog << "#Time\txc\txcs\tyc\tycs\tzc\tzcs\txl\txls\tyl\tyls\tzl\tzls\tcx\tcy\tcz\tsx\tsy\tsz" << endl;
    feedbacklog.close();
    cout << "Accumulated steps: " << steps[0] << ", " << steps[1] << ", " << steps[2] << endl;
    warnRange(steps);
    printHelp();
    while(running)
	{
	HeraldState state=checkHerald(&logTime, xc,yc,zc, &tao);
	if(state==HERALD_INCOMPLETE)
	    {
	    if(++incomplete>=5) // still unreadable after 5 s: not a write in progress
		{
		cout<<"herald file unreadable, skipping it"<<endl;
		if(remove(herald.c_str())!=0){cout<<"error deleting file"<<endl;}
		incomplete=0;
		}
	    }
	else
	    {incomplete=0;}
	if(state==HERALD_OK) //if herald file exists, update and proceed; if not, wait until it appears
	    {
	    if(!input.empty()) {cout<<endl;}
	    strftime(timestring,40,timefmt.c_str(),&logTime);
	    if(tao>24*3600)  //neglecting the data from a day ago
		{
		xl[0]=0;xl[1]=1e5;
		yl[0]=0;yl[1]=1e5;
		zl[0]=0;zl[1]=1e5;
		}
	    /* Write current position and uncertainty in position to log: */
	    feedbacklog.open(flogname.c_str(),ios::app);
	    feedbacklog << timestring << "\t" << xc[0] <<"\t" << xc[1] << "\t" << yc[0] << "\t"
		    << yc[1] << "\t" << zc[0] << "\t" << zc[1] << "\t";
	    /*apply algorithm, and update the running average*/
	    PID(xl,yl,zl,xc,yc,zc,tao,c);
	    for (short i=0;i<3;i++){cc[i]=(int)rint(c[i]);}
	    /* Write running average and uncertainty to log: */
	    feedbacklog << xl[0] <<"\t" << xl[1] << "\t" << yl[0] << "\t"
		    << yl[1] << "\t" << zl[0] << "\t" << zl[1] << "\t";
	    if(checkRange(cc))
		{
		cout<<"correction out of range!\a\a"<<endl;
		cc[0]=0;cc[1]=0;cc[2]=0;
		}
	    else
		{
		/*open and set COM port*/
		HANDLE hPort=openCom();
		if(hPort==INVALID_HANDLE_VALUE)
		    {
		    cout<<"------------CHECK COM PORT: no correction applied------------\a\a"<<endl;
		    cc[0]=0;cc[1]=0;cc[2]=0;
		    }
		else
		    {
		    if(SetCom(hPort,cc)!=0) {cout<<"Not every command was written in full, check the controller"<<endl;}
		    CloseHandle(hPort);
		    // update step count:
		    for (short i=0;i<3;i++){steps[i]+=cc[i];}
		    saveSteps(steps);
		    }
		}
	    /* Write correction and cumulative correction to log: */
	    feedbacklog << cc[0] << "\t"<< cc[1] << "\t" << cc[2] << "\t"
		    << steps[0] << "\t" << steps[1] << "\t" << steps[2] << endl;
	    feedbacklog.close();

	    cout << "Accumulated steps: " << steps[0] << ", " << steps[1] << ", " << steps[2] << endl;
	    warnRange(steps);

	    /*remove herald file, and save log*/
	    if(remove(herald.c_str())!=0){cout<<"error deleting file"<<endl;}
	    saveLog(&logTime, xl,yl,zl);
	    if(!input.empty()) {cout<<"> "<<input<<flush;} // show again what was being typed
	    }

	/* Wait 1 s for the next herald check, reading console commands meanwhile */
	for(short k=0;k<10&&running;k++)
	    {
	    while(running&&_kbhit())
		{
		int ch=_getch();
		if(ch==0||ch==224) {_getch(); continue;} // arrow and function keys: ignore
		if(ch=='\r')
		    {
		    cout<<endl;
		    running=runCommand(input,steps,xl,yl,zl,flogname);
		    if(running) {saveLog(&logTime, xl,yl,zl);}
		    input.clear();
		    }
		else if(ch=='\b')
		    {
		    if(!input.empty()) {input.erase(input.size()-1); cout<<"\b \b"<<flush;}
		    }
		else if(ch>=32&&ch<127)
		    {
		    if(input.empty()) {cout<<"> ";}
		    input+=(char)ch;
		    cout<<(char)ch<<flush;
		    }
		}
	    Sleep(100); //sleep time in ms
	    }
	}

    return 0;
    }

/*------------------------------------------------------------------------------------------------------*/
void printHelp()
    {
    cout<<"Commands (type while it runs and press Enter; corrections continue while you type):"<<endl;
    cout<<"  m <axis> <steps>  move a plate by hand, e.g. \"m x -1500\" (axis x, y or z, at most "<<manualLimit<<" steps)"<<endl;
    cout<<"  s                 show the step counters and the running average"<<endl;
    cout<<"  z <axis|all>      zero step counter(s), e.g. once the plates are back near the middle"<<endl;
    cout<<"  h                 show this help"<<endl;
    cout<<"  q                 quit"<<endl;
    }

bool runCommand(string line, int* steps, double* xl, double* yl, double* zl, string flogname)
    {
    const string axisName="xyz";
    istringstream in(line);
    string cmd, axis;
    if(!(in>>cmd)) {return true;}
    in>>axis;
    size_t a=(axis.size()==1)?axisName.find(tolower(axis[0])):string::npos;

    if(cmd=="q")
	{
	return false;
	}
    else if(cmd=="s")
	{
	cout<<"step counters: x "<<steps[0]<<" (limit "<<maxsteps[0]<<"), y "<<steps[1]<<" (limit "<<maxsteps[1]
	    <<"), z "<<steps[2]<<" (limit "<<maxsteps[2]<<")"<<endl;
	cout<<"running average: "<<xl[0]<<"+/-"<<xl[1]<<" "<<yl[0]<<"+/-"<<yl[1]<<" "<<zl[0]<<"+/-"<<zl[1]<<endl;
	}
    else if(cmd=="z")
	{
	if(axis=="all") {steps[0]=0;steps[1]=0;steps[2]=0;}
	else if(a!=string::npos) {steps[a]=0;}
	else {cout<<"usage: z x   (or y, z, all)"<<endl; return true;}
	saveSteps(steps);
	cout<<"step counters now: "<<steps[0]<<", "<<steps[1]<<", "<<steps[2]<<endl;
	}
    else if(cmd=="m")
	{
	int n=0;
	if(a==string::npos||(in>>n).fail()||n==0||n>manualLimit||n<-manualLimit)
	    {
	    cout<<"usage: m x -1500   (axis x, y or z; at most "<<manualLimit<<" steps either way)"<<endl;
	    return true;
	    }
	HANDLE hPort=openCom();
	if(hPort==INVALID_HANDLE_VALUE) {cout<<"Move not done"<<endl; return true;}
	if(moveChannel(hPort,a+1,n)!=0) {cout<<"Not every command was written in full, check the controller"<<endl;}
	CloseHandle(hPort);
	steps[a]+=n;
	saveSteps(steps);
	// the move shifts the lattice: start this axis's running average afresh from the next measurement
	double *l=(a==0)?xl:((a==1)?yl:zl);
	l[0]=0; l[1]=1e10;
	char timestring[40];
	time_t now=time(NULL);
	strftime(timestring,40,"%Y-%m-%d-%H:%M:%S",localtime(&now));
	ofstream feedbacklog(flogname.c_str(),ios::app);
	feedbacklog<<"#"<<timestring<<"\tmanual move "<<axisName[a]<<" "<<n<<" steps\tcounters "
	    <<steps[0]<<"\t"<<steps[1]<<"\t"<<steps[2]<<endl;
	cout<<"Accumulated steps: "<<steps[0]<<", "<<steps[1]<<", "<<steps[2]<<endl;
	warnRange(steps);
	}
    else
	{
	printHelp();
	}
    return true;
    }
