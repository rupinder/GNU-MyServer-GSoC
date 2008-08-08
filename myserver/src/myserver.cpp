/*
MyServer
Copyright (C) 2002-2008 Free Software Foundation, Inc.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../stdafx.h"
#include "../include/server.h"
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

/* External libraries to be included in the project.  */
#ifdef WIN32
#pragma comment(lib,"winmm.lib")
#endif

#define MYSERVER_RUNAS_CONSOLE 1
#define MYSERVER_RUNAS_SERVICE 2

void consoleService ();

#ifdef WIN32
void __stdcall myServerCtrlHandler(u_long fdwControl);
void __stdcall myServerMain (u_long , LPTSTR *argv); 
static BOOL SignalHandler(DWORD type) ;
#else
int  writePidfile(const char*);
#endif
void runService();
void registerService();
void removeService();
void RunAsService();
static char *path;

/*
 *Change this to reflect the version of the software.
 */
#ifdef MYSERVER_VERSION
const char *versionOfSoftware = MYSERVER_VERSION;
#else
const char *versionOfSoftware = "0.9.0";
#endif
int argn;
char **argv;
void registerSignals();

#ifdef NOT_WIN
void Sig_Quit(int signal)
{
  Server::getInstance()->logWriteln("Exiting...");
  sync();
  Server::getInstance()->stop();
  registerSignals();
}

void Sig_Hup(int signal)
{
  /*
   *On the SIGHUP signal reboot the server.
   */
  Server::getInstance()->rebootOnNextLoop();
  registerSignals();
}
#else
static BOOL SignalHandler(DWORD type) 
{ 
  Server::getInstance()->logWriteln("Exiting...");
  Server::getInstance()->stop();
  registerSignals();
}

#endif


void registerSignals()
{
#ifdef NOT_WIN
  struct sigaction sig1, sig2, sig3;
  sig1.sa_flags = sig2.sa_flags = sig3.sa_flags = SA_RESETHAND;
  memset(&sig1, 0, sizeof(sig1));
  memset(&sig2, 0, sizeof(sig2));
  memset(&sig3, 0, sizeof(sig3));
  sig1.sa_handler = SIG_IGN;
  sig2.sa_handler = Sig_Quit;
  sig3.sa_handler = Sig_Hup;
  sigaction(SIGPIPE,&sig1,NULL); // catch broken pipes
  sigaction(SIGINT, &sig2,NULL); // catch ctrl-c
  sigaction(SIGTERM,&sig2,NULL); // catch the kill signal
  sigaction(SIGHUP,&sig3,NULL); // catch the HUP signal
#else
  SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_PROCESSED_INPUT);
  SetConsoleCtrlHandler( (PHANDLER_ROUTINE) SignalHandler, TRUE );
#endif
}



#ifdef ARGP

struct argp_input
{
  /* Print the version for MyServer?  */
  int version;
  char* logFileName;
  /* Define how run the server.  */
  int runas;
  char* pidFileName;
};

static char doc[] = "GNU MyServer ";
static char argsDoc[] = "";

/* Use the GNU C argp parser under not windows environments.  */
static struct argp_option options[] = 
{
  /* LONG NAME - SHORT NAME - PARAMETER NAME - FLAGS - DESCRIPTION.  */
  {"version", 'v', "VERSION", OPTION_ARG_OPTIONAL , "Print the version for the application"},
  {"run", 'r', "RUN", OPTION_ARG_OPTIONAL, "Specify how run the server (by default console mode)"},
  {"logfile", 'l', "log", 0, "Specify the file to use to log main myserver messages"},
  {"pidfile", 'p', "pidfile", OPTION_HIDDEN, "Specify the file where write the PID"},     
  {0}
};

static error_t parseOpt(int key, char *arg, struct argp_state *state)
{       
  argp_input *in = static_cast<argp_input*>(state->input);
  switch(key)
  {
  case 'v':
    in->version = 1;
    break;
    
  case 'r':
    if(arg)
    {
      if(!strcmpi(arg, "CONSOLE"))
        in->runas = MYSERVER_RUNAS_CONSOLE;
      else if(!strcmpi(arg, "SERVICE"))
        in->runas = MYSERVER_RUNAS_SERVICE;
    }
    else
      in->runas = MYSERVER_RUNAS_CONSOLE;
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
    return static_cast<error_t>(ARGP_ERR_UNKNOWN);
  }
  return static_cast<error_t>(0);
}

static struct argp myserverArgp = {options, parseOpt, argsDoc, doc};

#endif

/*!
 *Main function for MyServer.
 */
int main (int argn, char **argv)
{
  int runas = MYSERVER_RUNAS_CONSOLE;
#ifdef ARGP
  struct argp_input input;
#endif
#ifdef NOT_WIN
  pid_t pid;
  pid_t sid;
#endif
  
  ::argn=argn;
  ::argv=argv;

  registerSignals();

  try
  {
    Server::createInstance();
    Process::initialize();
  }
  catch(...)
  {
    /* Die if we get exceptions here.  */
    return(1);
  };
  
  {
    u_int pathLen = 0;
    u_int len = 0;
    bool differentCwd = false;

    pathLen = strlen(argv[0]);
    path = new char[pathLen + 1];
    if(path == 0)
      return 1;
    strncpy(path, argv[0], pathLen);

    for(len = 0; len < pathLen; len++)
    {
      if(path[len] == '/' || path[len] == '\\')
      {
        differentCwd = true;
        break;
      }
    }
    len = pathLen;
    while((path[len] != '\\') && (path[len] != '/'))
      len--;

    path[len] = '\0';

    /* Current working directory is where the myserver executable is.  */
    if(differentCwd)
    {
      setcwd(path);
    }

    /* We can free path memory now.  */
    delete [] path;
  }
  
  
#ifdef ARGP
  /* Reset the struct.  */
  input.version = 0;
  input.logFileName = 0;
  input.runas = MYSERVER_RUNAS_CONSOLE;
  input.pidFileName = 0;
  /* Call the parser.  */
  argp_parse(&myserverArgp, argn, argv, 0, 0, &input);
  runas=input.runas;
  if(input.logFileName)
  {
    if(Server::getInstance()->setLogFile(input.logFileName))
    {
      cout << "Error loading log file" << endl;
      return 1;
    }
  }
  /* If the version flag is up, show the version and exit.  */
  if(input.version)
  {
    cout << "GNU MyServer "<< versionOfSoftware << endl;
    
    cout 
#ifdef __DATE__
      << "Compiled on " << __DATE__  
#endif
      << endl;

    cout << "http://www.gnu.org/software/myserver" << endl;
    return 0;   
  }
#else
  if(argn > 1)
  {       
    if(!strcmpi(argv[1], "VERSION"))
    {
      cout << "MyServer " << versionOfSoftware << endl;
      return 0;
    }
    if(!strcmpi(argv[1], "CONSOLE"))
    {
      runas = MYSERVER_RUNAS_CONSOLE;
    }
    if(!strcmpi(argv[1], "REGISTER"))
    {
      registerService();
#ifndef ARGP
      RunAsService();
#endif
      runas = MYSERVER_RUNAS_SERVICE;
      return 0;
    }
    if(!strcmpi(argv[1], "RUNSERVICE"))
    {
      RunAsService();
      return 0;
    }
    if(!strcmpi(argv[1], "UNREGISTER"))
    {
      removeService();
      runas = MYSERVER_RUNAS_SERVICE;
    }
    if(!strcmpi(argv[1], "SERVICE"))
    {
      /*
       *Set the log file to use when in service mode.
       */
      runas = MYSERVER_RUNAS_SERVICE;
    }
  }
#endif
  /*
   *Start here the MyServer execution.
   */
  try
     {
       switch(runas)
       {
       case MYSERVER_RUNAS_CONSOLE:
         consoleService();
         break;
       case MYSERVER_RUNAS_SERVICE:
#ifdef WIN32
         runService();
#else
         /*
          *Run the daemon.
          *pid is the process ID for the forked process.
          *Fork the process.
          */
         pid = fork();

         /*  
          *An error happened, return with errors.
          */
         if (pid < 0) 
         {
           return 1;
         }
         /*
          *A good process id, return with success. 
          */
         if (pid > 0) 
         {
           return 0;
         }
         /*
          *Store the PID.
          */
#ifdef ARGP
         if(input.pidFileName)
           writePidfile(input.pidFileName);
         else
           writePidfile("/var/run/myserver.pid");
#else
         writePidfile("/var/run/myserver.pid");
#endif
         /*
          *Create a SID for the new process.
          */
         sid = setsid();

         /*
          *Error in setting a new sid, return the error.
          */
         if (sid < 0) 
         {
           return 1;
         }
    
         /*
          *Finally run the server from the forked process.
          */
      consoleService();
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
int writePidfile(const char* filename)
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
 *Start MyServer in console mode.
 */
void consoleService ()
{
  Server::getInstance()->start();
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
  
  try
  {
    Server::createInstance();
  }
  catch(...)
  {
    /* Die if we get exceptions here.  */
    return;
  };

  
  MyServiceStatusHandle = RegisterServiceCtrlHandler( "MyServer", 
                                                      myServerCtrlHandler );
  if(MyServiceStatusHandle)
  {
    MyServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    SetServiceStatus( MyServiceStatusHandle, &MyServiceStatus );
    
    MyServiceStatus.dwControlsAccepted |= (SERVICE_ACCEPT_STOP 
                                           | SERVICE_ACCEPT_SHUTDOWN);
    MyServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus( MyServiceStatusHandle, &MyServiceStatus );
    
    Server::getInstance()->start();
    
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
    Server::getInstance()->stop();
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
  Server::getInstance()->logWriteln("Running service...");
#ifdef WIN32
  SERVICE_TABLE_ENTRY serviceTable[] =
    {
      { "MyServer", myServerMain },
      { 0, 0 }
    };
  if(!StartServiceCtrlDispatcher( serviceTable ))
  {
    if(GetLastError() == ERROR_INVALID_DATA)
    {
      Server::getInstance()->logWriteln("Invalid data");
    }
    else if(GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)
    {
      Server::getInstance()->logWriteln("Already running");
    }
    else
    {
      Server::getInstance()->logWriteln("Error running service");
    }
  }
#endif
}

/*!
 *Register the service.
 */
void registerService()
{
#ifdef WIN32
  SC_HANDLE service,manager;
  char path [MAX_PATH];
  GetCurrentDirectory(MAX_PATH,path);
  strcat(path,"\\");
  strcat(path,"myServer.exe SERVICE");
  
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

/*
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
       {
         if (MyServiceStatus.dwCurrentState != SERVICE_START_PENDING)
           break;
       }

       CloseServiceHandle (service);
       CloseServiceHandle (manager);
     }
   }
#endif
}
