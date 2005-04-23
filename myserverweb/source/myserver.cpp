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

#ifdef ARGP
#include <argp.h>
#endif

#ifdef NOT_WIN
#include <string.h>
#include <unistd.h>
#include <signal.h>
#endif
}

Server *server;


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
#else
int  write_pidfile(char*);
#endif
void runService();
void registerService();
void removeService();
void RunAsService();
static char *path;

/*!
 *Change this to reflect the version of the software.
 */
const char *versionOfSoftware="0.8.1";
int argn;
char **argv;

#ifdef NOT_WIN
void Sig_Quit(int signal)
{
	server->logWriteln("Exiting...");
	sync();
	server->stop();
}

void Sig_Hup(int signal)
{
  /*!
   *On the SIGHUP signal reboot the server.
   */
	server->rebootOnNextLoop();
}

#endif


#ifdef ARGP

struct argp_input
{
  /*! Print the version for MyServer? */
  int version;
  char* logFileName;
  /*! Define how run the server. */
  int runas;
  char* pidFileName;
};

static char doc[] = "MyServer ";
static char args_doc[] = "";

/*! Use the GNU C argp parser under not windows environments. */
static struct argp_option options[] = 
{
  /*LONG NAME - SHORT NAME - PARAMETER NAME - FLAGS - DESCRIPTION*/
	{"version", 'v', "VERSION", OPTION_ARG_OPTIONAL , "Print the version for the application"},
	{"run", 'r', "RUN", OPTION_ARG_OPTIONAL, "Specify how run the server(by default console mode)"},
	{"logfile", 'l', "log", 0, "Specify the file to use to log main myserver messages"},
	{"pidfile", 'p', "pidfile", OPTION_HIDDEN, "Specify the file where write the PID"},	{0}
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
       if(!strcmpi(arg, "CONSOLE"))
         in->runas = MYSERVER_RUNAS_CONSOLE;
       if(!strcmpi(arg, "SERVICE"))
         in->runas = MYSERVER_RUNAS_SERVICE;
       break;
     case 'l':
       in->logFileName = arg;
       break;
     case 'p':
       in->pidFileName = arg;
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
  int path_len;
#ifdef ARGP
	struct argp_input input;
#endif
#ifdef NOT_WIN
    pid_t pid;
    pid_t sid;
#endif

	::argn=argn;
	::argv=argv;
#ifdef NOT_WIN
	struct sigaction sig1, sig2, sig3;
  sig1.sa_flags = sig2.sa_flags = sig3.sa_flags = SA_RESETHAND;
	sig1.sa_handler = SIG_IGN;
	sig2.sa_handler = Sig_Quit;
  sig3.sa_handler = Sig_Hup;
	sigaction(SIGPIPE,&sig1,NULL); // catch broken pipes
	sigaction(SIGINT, &sig2,NULL); // catch ctrl-c
	sigaction(SIGTERM,&sig2,NULL); // catch the kill signal
	sigaction(SIGHUP,&sig3,NULL); // catch the HUP signal
#endif

  try
  {
    server = new Server();
  }
  catch(...)
  {
    /*! Die if we get exceptions here. */
    return(1);
  };
  path_len = strlen(argv[0]) +1 ;
  path = new char[path_len];
  if(path == 0)
    return 1;
	lstrcpy(path,argv[0]);
	u_long len=(u_long)strlen(path);
	while((path[len]!='\\')&&(path[len]!='/'))
		len--;
	path[len]='\0';

  /*! Current working directory is where the myserver executable is. */
	setcwd(path);
  
  /*! We can free path memory now. */
  delete [] path;
	
#ifdef ARGP
	/*! Reset the struct. */
	input.version = 0;
  input.logFileName = 0;
	input.runas = MYSERVER_RUNAS_CONSOLE;
  input.pidFileName = 0;
	/*! Call the parser. */
	argp_parse(&myserver_argp, argn, argv, 0, 0, &input);
	runas=input.runas;
  if(input.logFileName)
  {
    if(server->setLogFile(input.logFileName))
    {
      printf("Error loading log file\n");
      return 1;
    }
  }
	/*! If the version flag is up, show the version and exit. */
	if(input.version)
	{
		printf("MyServer %s\r\n",versionOfSoftware);
		return 0;   
	}
#else
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
		if(!lstrcmpi(argv[1],"REGISTER"))
		{
      registerService();
			runas = MYSERVER_RUNAS_SERVICE;
      return 0;
		}
		if(!lstrcmpi(argv[1],"RUNSERVICE"))
		{
      RunAsService();
      return 0;
		}
		if(!lstrcmpi(argv[1],"UNREGISTER"))
		{
      removeService();
			runas = MYSERVER_RUNAS_SERVICE;
		}
		if(!lstrcmpi(argv[1],"SERVICE"))
		{
      /*!
       *Set the log file to use when in service mode.
       */
			runas = MYSERVER_RUNAS_SERVICE;
		}
	}
#endif
  /*!
   *Start here the MyServer execution.
   */
   
  try
  {
    switch(runas)
    {
	  case MYSERVER_RUNAS_CONSOLE:
	  	console_service(argn, argv);
	  	break;
	  case MYSERVER_RUNAS_SERVICE:
#ifdef WIN32
	  	runService();
#else
      /*!
       *Run the daemon.
       *pid is the process ID for the forked process.
       *Fork the process.
       */
      pid = fork();

      /*!  
       *An error happened, return with errors.
       */
      if (pid < 0) 
      {
        return 1;
      }
      /*!
       *A good process id, return with success. 
       */
      if (pid > 0) 
      {
        return 0;
      }
      /*!
       *Store the PID.
       */
#ifdef ARGP
      if(input.pidFileName)
        write_pidfile(input.pidFileName);
      else
        write_pidfile("/var/run/myserver.pid");
#else
      write_pidfile("/var/run/myserver.pid");
#endif
      /*!
       *Create a SID for the new process.
       */
      sid = setsid();

      /*!
       *Error in setting a new sid, return the error.
       */
      if (sid < 0) 
      {
        return 1;
      }   
    
      /*!
       *Finally run the server from the forked process.
       */
  		console_service(argn, argv);
#endif

	  	break;
    }
  }
  catch(...)
  {
    return 1;
  };
	return 0;
} 

#ifndef WIN32

/*!
 *Write the current PID to the file.
 */
int  write_pidfile(char* filename)
{
 	int pidfile;
	pid_t pid = getpid(); 
  char buff[12];
  int ret;
  pidfile = open(filename, O_RDWR | O_CREAT);

  if(pidfile == -1)
		return -1;

  sprintf(buff,"%i\n", pid);
  ret = write(pidfile, buff, strlen(buff));
  if(ret == -1)
    return -1;
  return close(pidfile);
}
#endif

/*!
 *Start MyServer in console mode
 */
void console_service (int, char **)
{
#ifdef WIN32
	SetConsoleCtrlHandler (NULL, TRUE);
	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_PROCESSED_INPUT );
#endif
	server->start();
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

	MyServiceStatusHandle = RegisterServiceCtrlHandler( "MyServer", 
                                                      myServerCtrlHandler );
	if ( MyServiceStatusHandle )
	{
		MyServiceStatus.dwCurrentState = SERVICE_START_PENDING;
		SetServiceStatus( MyServiceStatusHandle, &MyServiceStatus );

		MyServiceStatus.dwControlsAccepted |= (SERVICE_ACCEPT_STOP 
                                           | SERVICE_ACCEPT_SHUTDOWN);
		MyServiceStatus.dwCurrentState = SERVICE_RUNNING;
		SetServiceStatus( MyServiceStatusHandle, &MyServiceStatus );

		server->start();
	
		MyServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		SetServiceStatus( MyServiceStatusHandle, &MyServiceStatus );

		MyServiceStatus.dwControlsAccepted &= ~(SERVICE_ACCEPT_STOP 
                                            | SERVICE_ACCEPT_SHUTDOWN);
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
			server->stop();
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
#endif

/*!
 *Run MyServer service.
 */
void runService()
{
	server->logWriteln("Running service...");
#ifdef WIN32
	SERVICE_TABLE_ENTRY serviceTable[] =
	{
		{ "MyServer", myServerMain },
		{ 0, 0 }
	};
	if(!StartServiceCtrlDispatcher( serviceTable ))
	{
		if(GetLastError()==ERROR_INVALID_DATA)
		{
			server->logWriteln("Invalid data");
		}
		else if(GetLastError()==ERROR_SERVICE_ALREADY_RUNNING)
		{
			server->logWriteln("Already running");
		}
		else
		{
			server->logWriteln("Error running service");
		}
	}
#endif
}

/*!
 *Register the service
 */
void registerService()
{
#ifdef WIN32
	SC_HANDLE service,manager;
	char path [MAX_PATH];
	GetCurrentDirectory(MAX_PATH,path);
	lstrcat(path,"\\");
	lstrcat(path,"myServer.exe SERVICE");
	
	manager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
	if (manager)
	{
		service = CreateService(manager,"MyServer","MyServer",
                            SERVICE_ALL_ACCESS,SERVICE_WIN32_OWN_PROCESS,
                            SERVICE_AUTO_START, SERVICE_ERROR_IGNORE, path,
                            0, 0, 0, 0, 0);
		if (service)
		{
			CloseServiceHandle (service);
		}

		CloseServiceHandle (manager);
	}
#endif
}

/*!
*Unregister the OS service.
*/
void removeService()
{
#ifdef WIN32
	SC_HANDLE service,manager;
  manager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
	if (manager)
	{
		service = OpenService (manager, "MyServer", SERVICE_ALL_ACCESS);
		if (service)
		{
			ControlService (service, SERVICE_CONTROL_STOP,&MyServiceStatus);
			while (QueryServiceStatus (service, &MyServiceStatus))
        if (MyServiceStatus.dwCurrentState != SERVICE_STOP_PENDING)
          break;
			DeleteService(service);
			CloseServiceHandle (service);
			CloseServiceHandle (manager);
		}
	}
#endif
}

/*!
 *Start the service.  
 */
void RunAsService()
{
#ifdef WIN32
   SC_HANDLE service,manager;

   manager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
   if (manager)
   {
     service = OpenService (manager, "MyServer", SERVICE_ALL_ACCESS);
     if (service)
     {
       StartService(service,0,NULL);
       while (QueryServiceStatus (service, &MyServiceStatus))
         if (MyServiceStatus.dwCurrentState != SERVICE_START_PENDING)
               break;
       CloseServiceHandle (service);
       CloseServiceHandle (manager);
     }
   }
#endif
}
