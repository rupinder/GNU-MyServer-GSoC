/*
*myServer
*Copyright (C) Rocky_10_Balboa
*This library is free software; you can redistribute it and/or
*modify it under the terms of the GNU Library General Public
*License as published by the Free Software Foundation; either
*version 2 of the License, or (at your option) any later version.

*This library is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*Library General Public License for more details.

*You should have received a copy of the GNU Library General Public
*License along with this library; if not, write to the
*Free Software Foundation, Inc., 59 Temple Place - Suite 330,
*Boston, MA  02111-1307, USA.
*/

#include "..\stdafx.h"
#include "..\include\cserver.h"
#include <direct.h>

/*
*External libraries to be included in the project.
*/
#ifdef WIN32
#pragma comment(lib,"wsock32.lib")
#pragma comment(lib,"winmm.lib")
#endif


void console_service (int, char **);

#ifdef WIN32

BOOL __stdcall control_handler(DWORD control_type);
VOID __stdcall myServerCtrlHandler(DWORD fdwControl);
VOID __stdcall myServerMain (DWORD argc, LPTSTR *argv); 
void runService();

#endif

static char path[MAX_PATH];
INT hInst;
int cmdShow;
/*
*Change this for every new version of this software.
*/
const char *versionOfSoftware="0.3";
cserver server;

int main (int argn, char **argc)
{ 
	/*
	*By default use the console mode.
	*/
	if(argn==1)
		argc[1]="CONSOLE";
	lstrcpy(path,argc[0]);
	int len=lstrlen(path);
	while((path[len]!='\\')&(path[len]!='/'))
		len--;
	path[len]='\0';
	_chdir(path);
	

	hInst=0;
	cmdShow=0;
	char* cmdLine=argc[1];
	int i;
	for(i=0;i<lstrlen(cmdLine);i++)
	{
		if(!lstrcmpi(&cmdLine[i],"CONSOLE"))
		{
			console_service(0,0);
			return 0;
		}
	}
	for(i=0;i<lstrlen(cmdLine);i++)
	{
		if(!lstrcmpi(&cmdLine[i],"SERVICE"))
		{
#ifdef WIN32
			runService();
#endif
			return 0;
		}
	}

	return 0;
} 


void console_service (int, char **)
{
    printf ("starting in console mode\n");

#ifdef WIN32
	SetConsoleCtrlHandler (control_handler, TRUE);
#endif

	printf("started in console mode\n");
	server.start(hInst);
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
VOID  __stdcall myServerMain (DWORD, LPTSTR*)
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

		server.start(hInst);
	
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
VOID __stdcall myServerCtrlHandler(DWORD fdwControl)
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
*Terminate the application on the pression of CTRL+C or CTRL+BREAK.
*/
BOOL __stdcall control_handler (DWORD control_type)
{
	switch (control_type)
	{
		case CTRL_BREAK_EVENT:
		case CTRL_C_EVENT:
			printf ("%s\n",lserver->languageParser.getValue("MSG_SERVICESTOP"));
			server.stop();
			return (TRUE);

	}
	return (FALSE);
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
