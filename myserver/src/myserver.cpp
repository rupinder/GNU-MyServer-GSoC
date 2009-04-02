/*
  MyServer
  Copyright (C) 2002-2009 Free Software Foundation, Inc.
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

#include "stdafx.h"
#include <include/server/server.h>
#include <include/base/file/files_utility.h>
#include <include/base/string/stringutils.h>
#include <include/base/process/process.h>

extern "C" {
#ifdef WIN32
#include <direct.h>
#endif

#ifdef ARGP
#include <argp.h>
#endif

#ifndef WIN32
#include <string.h>
#include <unistd.h>
#include <signal.h>
#endif
}

#define MYSERVER_RUNAS_CONSOLE 1
#define MYSERVER_RUNAS_SERVICE 2

void consoleService(string &, string &, string &, string &, string &);

#ifdef WIN32
void __stdcall myServerCtrlHandler(u_long fdwControl);
void __stdcall myServerMain (u_long , LPTSTR *argv);
static BOOL SignalHandler(DWORD type) ;
#else
int  writePidfile(const char* file = NULL);
#endif
void runService();
void registerService();
void removeService();
void RunAsService();

int argn;
char **argv;
void registerSignals();

#ifndef WIN32
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
#ifndef WIN32
  struct sigaction sig1, sig2, sig3;
  struct sigaction sa;

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = SIG_IGN;
  sa.sa_flags   = SA_RESTART;
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

  /* Avoid zombie processes.  */
  sigaction(SIGCHLD, &sa, (struct sigaction *)NULL);
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
  int useForkServer;
};

static char doc[] = "GNU MyServer ";
static char argsDoc[] = "";

/* Use the GNU C argp parser under not windows environments.  */
static struct argp_option options[] =
  {
    /* LONG NAME - SHORT NAME - PARAMETER NAME - FLAGS - DESCRIPTION.  */
    {"version", 'v', "VERSION", OPTION_ARG_OPTIONAL , "Print the version for the application"},
    {"run", 'r', "RUN", OPTION_ARG_OPTIONAL, "Specify how run the server (by default console mode)"},
    {"log", 'l', "location", 0, "Specify the location (in the format protocol://resource) to use to log main myserver messages"},
    {"pidfile", 'p', "pidfile", 0, "Specify the file where write the PID"},
    {"fork_server", 'f', "", OPTION_ARG_OPTIONAL, "Specify if use a fork server"},
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

    case 'f':
      in->useForkServer = 1;
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
 *Load the external path.
 *Return nonzero on errors.
 */
int loadExternalPath(string &externalPath)
{
  try
    {
      externalPath = "";

#ifdef WIN32
      externalPath.assign("plugins");
#else
      if(FilesUtility::fileExists("plugins"))
        externalPath.assign("plugins");
      else
        {
#ifdef PREFIX
          externalPath.assign(PREFIX);
          externalPath.append("/lib/myserver/plugins");
#else
          externalPath.assign("/usr/lib/myserver/plugins");
#endif
        }

#endif

#ifdef WIN32
      externalPath.assign("plugins");
#endif
    }
  catch (...)
    {
    }
  return 0;
}


/*!
 *Load the languages path.
 *Return nonzero on errors.
 */
int loadLanguagesPath(string &languagesPath)
{
  try
    {
      languagesPath = "";

#ifdef WIN32
      languagesPath.assign(getdefaultwd(0, 0));
      languagesPath.append("/languages/");
#else
      /*
       *Do not use the files in the directory /usr/share/myserver/languages
       *if exists a local directory.
       */
      if (FilesUtility::fileExists("languages"))
        {
          languagesPath.assign(getdefaultwd(0, 0));
          languagesPath.append("/languages/");
        }
      else
        {
#ifdef PREFIX
          languagesPath.assign(PREFIX);
          languagesPath.append("/share/myserver/languages/");
#else
          /* Default PREFIX is /usr/.  */
          languagesPath.assign("/usr/share/myserver/languages/");
#endif
        }
#endif

#ifdef WIN32
      languagesPath.assign( "languages/" );
#endif
    }
  catch (...)
    {
    }
  return 0;
}

/*!
 *Load the vhost configuration files locations.
 *Return nonzero on errors.
 */
int loadVHostConfFilesLocation(string &vhostConfigurationFile)
{
  try
    {
      vhostConfigurationFile = "";

#ifdef WIN32
      vhostConfigurationFile.assign("virtualhosts.xml");
#else
      /*
       *Under an *nix environment look for .xml files in the following order.
       *1) myserver executable working directory
       *2) ~/.myserver/
       *3) /etc/myserver/
       *4) default files will be copied in myserver executable working
       */
      if (FilesUtility::fileExists("virtualhosts.xml"))
        {
          vhostConfigurationFile.assign("virtualhosts.xml");
        }
      else if (FilesUtility::fileExists("~/.myserver/virtualhosts.xml"))
        {
          vhostConfigurationFile.assign("~/.myserver/virtualhosts.xml");
        }
      else if (FilesUtility::fileExists("/etc/myserver/virtualhosts.xml"))
        {
          vhostConfigurationFile.assign("/etc/myserver/virtualhosts.xml");
        }
#endif
    }
  catch (...)
    {
    }
  return 0;
}


/*!
 *Load the mime configuration files locations.
 *Return nonzero on errors.
 */
int loadMimeConfFilesLocation(string &mimeConfigurationFile)
{
  try
    {
      mimeConfigurationFile = "";

#ifdef WIN32
      mimeConfigurationFile.assign("MIMEtypes.xml");
#else
      /*
       *Under an *nix environment look for .xml files in the following order.
       *1) myserver executable working directory
       *2) ~/.myserver/
       *3) /etc/myserver/
       *4) default files will be copied in myserver executable working
       */
      if (FilesUtility::fileExists("MIMEtypes.xml"))
        {
          mimeConfigurationFile.assign("MIMEtypes.xml");
        }
      else if (FilesUtility::fileExists("~/.myserver/MIMEtypes.xml"))
        {
          mimeConfigurationFile.assign("~/.myserver/MIMEtypes.xml");
        }
      else if (FilesUtility::fileExists("/etc/myserver/MIMEtypes.xml"))
        {
          mimeConfigurationFile.assign("/etc/myserver/MIMEtypes.xml");
        }
#endif
    }
  catch (...)
    {
    }
  return 0;
}


/*!
 *Load the main configuration files locations.
 *Return nonzero on errors.
 */
int loadMainConfFilesLocation(string &mainConfigurationFile)
{
  try
    {
      mainConfigurationFile = "";

#ifdef WIN32
      mainConfigurationFile.assign("myserver.xml");
#else
      /*
       *Under an *nix environment look for .xml files in the following order.
       *1) myserver executable working directory
       *2) ~/.myserver/
       *3) /etc/myserver/
       *4) default files will be copied in myserver executable working
       */
      if (FilesUtility::fileExists("myserver.xml"))
        {
          mainConfigurationFile.assign("myserver.xml");
        }
      else if (FilesUtility::fileExists("~/.myserver/myserver.xml"))
        {
          mainConfigurationFile.assign("~/.myserver/myserver.xml");
        }
      else if (FilesUtility::fileExists("/etc/myserver/myserver.xml"))
        {
          mainConfigurationFile.assign("/etc/myserver/myserver.xml");
        }
#endif
    }
  catch (...)
    {
    }
  return 0;
}


/*!
 *Load the configuration files locations.
 *Return nonzero on errors.
 */
int loadConfFilesLocation(string &mainConfigurationFile, string &mimeConfigurationFile,
                          string &vhostConfigurationFile, string &externalPath, string &languagesPath)
{
  if (loadMainConfFilesLocation(mainConfigurationFile))
    return -1;

  if (loadMimeConfFilesLocation(mimeConfigurationFile))
    return -1;

  if (loadVHostConfFilesLocation(vhostConfigurationFile))
    return -1;

  if (loadExternalPath(externalPath))
    return -1;

  if (loadLanguagesPath(languagesPath))
    return -1;

  return 0;
}


/*!
 *Main function for MyServer.
 */
int main (int argn, char **argv)
{
  int runas = MYSERVER_RUNAS_CONSOLE;
#ifdef ARGP
  struct argp_input input;
#endif
#ifndef WIN32
  pid_t pid;
  pid_t sid;
#endif
  string mainConf, mimeConf, vhostConf, externPath, langPath;

  ::argn=argn;
  ::argv=argv;

  registerSignals();

  try
    {
      Server::createInstance();
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
    char *path;

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

    /* Current working directory is where the myserver executable is.  */
    if(differentCwd)
      {
        len = pathLen;
        while((path[len] != '\\') && (path[len] != '/'))
          len--;
        path[len] = '\0';

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
  input.useForkServer = 0;

  /* Call the parser.  */
  argp_parse(&myserverArgp, argn, argv, 0, 0, &input);
  runas=input.runas;
  if(input.logFileName)
    {
      if (Server::getInstance ()->setLogLocation (input.logFileName))
        {
          cout << "Error setting the location for the MyServer's main log" << endl;
          return 1;
        }
    }
  /* If the version flag is up, show the version and exit.  */
  if(input.version)
    {
      cout << MYSERVER_VERSION << endl;

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
          cout << MYSERVER_VERSION << endl;
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

#ifdef ARGP
  if (input.useForkServer)
    Process::getForkServer ()->startForkServer ();
#endif

  /*
   *Start here the MyServer execution.
   */
  try
    {
      setcwdBuffer();
      loadConfFilesLocation(mainConf, mimeConf, vhostConf, externPath, langPath);

      switch(runas)
        {
        case MYSERVER_RUNAS_CONSOLE:
          consoleService(mainConf, mimeConf, vhostConf, externPath, langPath);
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

          if (pid)
            {
              /*
               *Store the PID.
               */
#ifdef ARGP
              if(input.pidFileName)
                writePidfile(input.pidFileName);
              else
                {
                  writePidfile();
                }
#else
              writePidfile();
#endif
              return 0;
            }
          /*
           *Create a SID for the new process.
           */
          sid = setsid();
   
          /*
           *Error in setting a new sid, return the error.
           */
          if (sid < 0)
            return 1;
   
   
          /*
           *Finally run the server from the forked process.
           */
          consoleService(mainConf, mimeConf, vhostConf, externPath, langPath);
#endif
          break;
        }
    }
  catch(...)
    {
      return 1;
    };

#ifdef ARGP
  if (input.useForkServer)
    Process::getForkServer ()->killServer ();
#endif

  return 0;
}

#ifndef WIN32

/*!
 *Write the current PID to the file.
 */
int writePidfile (const char* filename)
{
  int pidfile;
  pid_t pid = getpid();
  char buff[12];
  int ret;
  string file = "";

  if (filename == NULL)
    {
#ifdef PREFIX
      file.assign (PREFIX);
      file.append ("/");
#endif
      file.append ("/var/run/myserver.pid");
    }

  pidfile = open (filename, O_RDWR | O_CREAT);

  if (pidfile == -1)
    return -1;

  sprintf (buff,"%i\n", pid);
  ret = write (pidfile, buff, strlen(buff));
  if(ret == -1)
    return -1;
  return close (pidfile);
}
#endif

/*!
 *Start MyServer in console mode.
 */
void consoleService(string &mainConf, string &mimeConf, string &vhostConf, string &externPath, string &langPath)
{
  Server::getInstance()->start(mainConf, mimeConf, vhostConf, externPath, langPath);
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

      string null ("");
      Server::getInstance()->start(null, null, null, null, null);

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
      { (CHAR*) "MyServer", myServerMain },
      { 0, 0 }
    };

  if(!StartServiceCtrlDispatcher( serviceTable ))
    {
      if(GetLastError() == ERROR_INVALID_DATA)
          Server::getInstance()->logWriteln("Invalid data");
      else if(GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)
          Server::getInstance()->logWriteln("Already running");
      else
          Server::getInstance()->logWriteln("Error running service");
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
  strcat(path,"myserver.exe SERVICE");

  manager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
  if (manager)
    {
      service = CreateService(manager,"GNU MyServer","GNU MyServer",
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
      service = OpenService (manager, "GNU MyServer", SERVICE_ALL_ACCESS);
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
