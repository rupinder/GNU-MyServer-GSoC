/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
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
#include "../include/security.h"
extern "C" {
#ifdef WIN32
#include <direct.h>
#endif
#ifdef NOT_WIN
#include <argp.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#endif
}


/*! External libraries to be included in the project. */
#ifdef WIN32
#pragma comment(lib,"winmm.lib")
#endif

#define MYSERVER_RUNAS_CONSOLE 1
#define MYSERVER_RUNAS_SERVICE 2

void console_service (int, char **);

#ifdef WIN32
void __stdcall myServerCtrlHandler(u_long fdwControl);
void __stdcall myServerMain (u_long , LPTSTR *argv); 
void runService();
#endif

static char path[MAX_PATH];

int cmdShow;
/*!
*Change this to reflect the version of the software.
*/
const char *versionOfSoftware="0.7.2";
cserver server;
int argn;
char **argv;
/*
*Flag to determine if reboot the console at the end of its execution.
*/
int rebootMyServerConsole;
/*
*Reboot the console application.
*/
int reboot_console()
{
	server.stop();	
	console_service(argn,argv);
	return 0;
}

#ifdef NOT_WIN
void Sig_Quit(int signal)
{
	printf("Exiting...\n");
	sync();
	server.stop();
}
#endif


#ifdef NOT_WIN

struct argp_input
{
  /*! Print the version for MyServer? */
  int version;
  /*! Define how run the server. */
  int runas;
};

static char doc[] = "MyServer ";
static char args_doc[] = "";

/*! Use the GNU C argp parser under not windows environments. */
static struct argp_option options[] = 
{
  /*LONG NAME - SHORT NAME - PARAMETER NAME - FLAGS - DESCRIPTION*/
	{"version",'v',"VERSION",OPTION_ARG_OPTIONAL,"Print the version for the application"},
	{"run",'r',"RUN",OPTION_ARG_OPTIONAL,"Specify how run the server(by default console mode)"},
	{0}
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{	
  argp_input *in = (argp_input*)state->input;
  switch(key)
  {
     case 'v':
       in->version = 1;
       break;
     case 'r':
       /*! At the moment only CONSOLE mode is available. */
       if(!strcmpi(arg, "CONSOLE"))
         in->runas = MYSERVER_RUNAS_CONSOLE;
         break;
     case ARGP_KEY_ARG:
     case ARGP_KEY_END:
       break;
     default:
       return (error_t)ARGP_ERR_UNKNOWN;
  }
  return (error_t)0;
}

static struct argp myserver_argp = {options, parse_opt, args_doc, doc};

#endif


/*!
*Main function for MyServer
*/
int main (int argn, char **argv)
{
  int runas=MYSERVER_RUNAS_CONSOLE;
	::argn=argn;
	::argv=argv;
	rebootMyServerConsole=0;
#ifdef NOT_WIN
	struct sigaction sig1, sig2;
	sig1.sa_handler=SIG_IGN;
	sig2.sa_handler=Sig_Quit;
	sigaction(SIGPIPE,&sig1,NULL); // catch broken pipes
	sigaction(SIGINT, &sig2,NULL); // catch ctrl-c
	sigaction(SIGTERM,&sig2,NULL); // catch the kill command
#endif
	lstrcpy(path,argv[0]);
	u_long len=(u_long)strlen(path);
	while((path[len]!='\\')&&(path[len]!='/'))
		len--;
	path[len]='\0';
  	/*! Configure the current working directory. */
	setcwd(path);
	
	cmdShow=0;
#ifdef NOT_WIN
	struct argp_input input;
	/*! Reset the struct. */
	input.version = 0;
	input.runas = MYSERVER_RUNAS_CONSOLE;
	/*! Call the parser. */
	argp_parse(&myserver_argp, argn, argv, 0, 0, &input);
	runas=input.runas;
	/*! If the version flag is up, show the version and exit. */
	if(input.version)
	{
		printf("MyServer %s\r\n",versionOfSoftware);
		return 0;   
	}
#endif

#ifdef WIN32
	if(argn > 1)
	{	
		if(!lstrcmpi(argv[1],"VERSION"))
		{
			printf("MyServer %s\r\n",versionOfSoftware);
			return 0;
		}
		if(!lstrcmpi(argv[1],"CONSOLE"))
		{
			runas = MYSERVER_RUNAS_CONSOLE;
		}
		if(!lstrcmpi(argv[1],"SERVICE"))
		{
			runas = MYSERVER_RUNAS_SERVICE;
		}
	}
#endif
  switch(runas)
  {
	case MYSERVER_RUNAS_CONSOLE:
		console_service(argn,argv);
		if(rebootMyServerConsole)
		reboot_console();
		break;
	case MYSERVER_RUNAS_SERVICE:
#ifdef WIN32
		runService();
#endif
		break;
  }

	return 0;
} 

/*!
*Start MyServer in console mode
*/
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


/*!
*These functions are available only on the windows platform.
*/
#ifdef WIN32
SERVICE_STATUS          MyServiceStatus; 
SERVICE_STATUS_HANDLE   MyServiceStatusHandle; 

/*!
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

	MyServiceStatusHandle = RegisterServiceCtrlHandler( "MyServer", myServerCtrlHandler );
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

/*!
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


/*!
*Run MyServer like a NT service.
*/
void runService()
{
	printf("Running service...\n");
	SERVICE_TABLE_ENTRY serviceTable[] =
	{
		{ "MyServer", myServerMain },
		{ 0, 0 }
	};
	if(!StartServiceCtrlDispatcher( serviceTable ))
	{
		if(GetLastError()==ERROR_INVALID_DATA)
		{
			printf("Invalid data\n");
		}
		else if(GetLastError()==ERROR_SERVICE_ALREADY_RUNNING)
		{
			printf("Already running\n");
		}
		else
		{
			printf("Error running service\n");
		}
	}
}



#endif
