/*
*MyServer
*Copyright (C) 2002 The MyServer team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "../stdafx.h"
#include "../include/cserver.h"
#include "../include/stringutils.h"
extern "C" {
#ifdef WIN32
#include <direct.h>
#endif
#ifdef __linux__
#include <string.h>
#include <unistd.h>
#include <signal.h>
#endif
}


/*
*External libraries to be included in the project.
*/
#ifdef WIN32
#pragma comment(lib,"winmm.lib")
#endif


void console_service (int, char **);

#ifdef WIN32
void __stdcall myServerCtrlHandler(u_long fdwControl);
void __stdcall myServerMain (u_long argc, LPTSTR *argv); 
void runService();
#endif

static char path[MAX_PATH];

int cmdShow;
/*
*Change this for every new version of this software.
*/
const char *versionOfSoftware="0.4.3";
cserver server;


#ifdef __linux__
void Sig_Quit(int signal)
{
	printf("Exiting...\n");
	sync();
	server.stop();
}
#endif

int main (int argn, char **argc)
{
#ifndef WIN32
	struct sigaction sig1, sig2;
	sig1.sa_handler=SIG_IGN;
	sig2.sa_handler=Sig_Quit;
	sigaction(SIGPIPE,&sig1,NULL); // catch broken pipes
	sigaction(SIGTERM,&sig2,NULL); // catch ctrl-c
#endif
	/*
	*By default use the console mode.
	*/
	if(argn==1)
		argc[1]="CONSOLE";
	lstrcpy(path,argc[0]);
	int len=strlen(path);
	while((path[len]!='\\')&&(path[len]!='/'))
		len--;
	path[len]='\0';
#ifdef WIN32
	_chdir(path);
#endif
#ifdef __linux__
	chdir(path);
#endif
	
	cmdShow=0;
	char* cmdLine=argc[1];
	int i;

	for(i=0;i<strlen(cmdLine);i++)
	{
		if(!lstrcmpi(&cmdLine[i],"CONSOLE"))
		{
			console_service(argn,argc);
		}
#ifdef WIN32
		if(!lstrcmpi(&cmdLine[i],"SERVICE"))
		{
			runService();
		}
#endif
	}

	return 0;
} 


void console_service (int, char **)
{
    printf ("starting in console mode\n");

#ifdef WIN32
	SetConsoleCtrlHandler (NULL, TRUE);
	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),ENABLE_PROCESSED_INPUT);
#endif

	printf("started in console mode\n");
	server.start();
}


/*
*These functions are available only on the windows platform.
*/
#ifdef WIN32
SERVICE_STATUS          MyServiceStatus; 
SERVICE_STATUS_HANDLE   MyServiceStatusHandle; 
/*
*Entry-point for the NT service.
*/
void  __stdcall myServerMain (u_long, LPTSTR*)
{
	MyServiceStatus.dwServiceType = SERVICE_WIN32;
	MyServiceStatus.dwCurrentState = SERVICE_STOPPED;
	MyServiceStatus.dwControlsAccepted = 0;
	MyServiceStatus.dwWin32ExitCode = NO_ERROR;
	MyServiceStatus.dwServiceSpecificExitCode = NO_ERROR;
	MyServiceStatus.dwCheckPoint = 0;
	MyServiceStatus.dwWaitHint = 0;

	MyServiceStatusHandle = RegisterServiceCtrlHandler( "myServer", myServerCtrlHandler );
	if ( MyServiceStatusHandle )
	{
		MyServiceStatus.dwCurrentState = SERVICE_START_PENDING;
		SetServiceStatus( MyServiceStatusHandle, &MyServiceStatus );

		MyServiceStatus.dwControlsAccepted |= (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
		MyServiceStatus.dwCurrentState = SERVICE_RUNNING;
		SetServiceStatus( MyServiceStatusHandle, &MyServiceStatus );

		server.start();
	
		MyServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		SetServiceStatus( MyServiceStatusHandle, &MyServiceStatus );

		MyServiceStatus.dwControlsAccepted &= ~(SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
		MyServiceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus( MyServiceStatusHandle, &MyServiceStatus );
	}
}


/*
*Manage the NT service.
*/
void __stdcall myServerCtrlHandler(u_long fdwControl)
{
	switch ( fdwControl )
	{
		case SERVICE_CONTROL_INTERROGATE:
			break;

		case SERVICE_CONTROL_SHUTDOWN:
		case SERVICE_CONTROL_STOP:
			MyServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
			SetServiceStatus( MyServiceStatusHandle, &MyServiceStatus );
			server.stop();
			return;

		case SERVICE_CONTROL_PAUSE:
			break;

		case SERVICE_CONTROL_CONTINUE:
			break;

		default:
			if ( fdwControl >= 128 && fdwControl <= 255 )
				break;
			else
				break;
	}
	SetServiceStatus( MyServiceStatusHandle, &MyServiceStatus );
}


/*
*Run myServer like a NT service.
*/
void runService()
{
	printf("Running service...\n");
	SERVICE_TABLE_ENTRY serviceTable[] =
	{
		{ "myServer", myServerMain },
		{ 0, 0 }
	};
	if(!StartServiceCtrlDispatcher( serviceTable ))
	{
		if(GetLastError()==ERROR_INVALID_DATA)
			printf("Invalid data\n");
		else if(GetLastError()==ERROR_SERVICE_ALREADY_RUNNING)
			printf("Already running\n");
		else
			printf("Error running service\n");
	}
}



#endif
