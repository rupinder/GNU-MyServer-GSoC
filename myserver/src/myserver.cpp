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

extern "C"
{
#ifdef WIN32
# include <direct.h>
#endif

#ifdef ARGP
# include <argp.h>
#endif

#ifndef WIN32
# include <string.h>
# include <unistd.h>
# include <signal.h>
#endif
}

#ifdef HAVE_GETTEXT
# include "localedir.h"
#endif


#define MYSERVER_RUNAS_CONSOLE 1
#define MYSERVER_RUNAS_SERVICE 2

void consoleService (string &, string &, string &, string &);

#ifdef WIN32
void __stdcall myServerCtrlHandler (u_long fdwControl);
void __stdcall myServerMainNT (u_long , LPTSTR *argv);
static BOOL SignalHandler (DWORD type) ;
#else
int  writePidfile (const char* file = NULL);
#endif
void runService ();
void registerService ();
void removeService ();
void runAsService ();

int argn;
char **argv;
void registerSignals ();

#ifndef WIN32
void Sig_Quit (int signal)
{
  Server::getInstance ()->log ("Exiting...");
  sync ();
  Server::getInstance ()->stop ();
  registerSignals ();
}

void Sig_Hup (int signal)
{
  /*
   *On the SIGHUP signal reboot the server.
   */
  Server::getInstance ()->rebootOnNextLoop ();
  registerSignals ();
}
#else
static BOOL SignalHandler (DWORD type)
{
  Server::getInstance ()->log ("Exiting...");
  Server::getInstance ()->stop ();
  registerSignals ();
}
#endif


void registerSignals ()
{
#ifndef WIN32
  struct sigaction sig1, sig2, sig3;
  struct sigaction sa;

  memset (&sa, 0, sizeof (sa));
  sa.sa_handler = SIG_IGN;
  sa.sa_flags   = SA_RESTART;
  memset (&sig1, 0, sizeof (sig1));
  memset (&sig2, 0, sizeof (sig2));
  memset (&sig3, 0, sizeof (sig3));
  sig1.sa_handler = SIG_IGN;
  sig2.sa_handler = Sig_Quit;
  sig3.sa_handler = Sig_Hup;
  sigaction (SIGPIPE,&sig1,NULL); // catch broken pipes
  sigaction (SIGINT, &sig2,NULL); // catch ctrl-c
  sigaction (SIGTERM,&sig2,NULL); // catch the kill signal
  sigaction (SIGHUP,&sig3,NULL); // catch the HUP signal

  /* Avoid zombie processes.  */
  sigaction (SIGCHLD, &sa, (struct sigaction *)NULL);
#else
  SetConsoleMode (GetStdHandle (STD_INPUT_HANDLE), ENABLE_PROCESSED_INPUT);
  SetConsoleCtrlHandler ((PHANDLER_ROUTINE) SignalHandler, TRUE);
#endif
}



#ifdef ARGP

struct argp_input
{
  /* Print the version for MyServer?  */
  int version;

  /* Define the main log file.  */
  const char *logFileName;

  /* Define how run the server.  */
  int runas;

  /* If executed as a daemon, write the pid to this file.  */
  const char *pidFileName;

  /* Define an alternate location for the configuration files.  */
  const char *confFilesLocation;

  /* Specify if the fork server is used.  */
  int useForkServer;
};

static char doc[] = "GNU MyServer ";
static char argsDoc[] = "";

enum
{
 CONFIG_OPT = UCHAR_MAX + 1
};

/* Use the GNU C argp parser under not windows environments.  */
static struct argp_option options[] =
  {
    /* LONG NAME - SHORT NAME - PARAMETER NAME - FLAGS - DESCRIPTION.  */
    {"version", 'v', "VERSION", OPTION_ARG_OPTIONAL , _("Print the version for the application")},
    {"run", 'r', "RUN", OPTION_ARG_OPTIONAL, _("Specify how run the server (by default console mode)")},
    {"log", 'l', "location", 0, _("Specify the location (using the format protocol://resource) to use as the main log.")},
    {"pidfile", 'p', "file", 0, _("Specify the file where write the PID")},
    {"fork_server", 'f', NULL, 0, _("Specify if use a fork server")},
    {"cfgdir", CONFIG_OPT, "dir", 0, _("Specify an alternate directory where look for configuration files")},
    {0}
  };

static error_t parseOpt (int key, char *arg, struct argp_state *state)
{
  argp_input *in = static_cast<argp_input*>(state->input);
  switch (key)
    {
    case 'v':
      in->version = 1;
      break;

    case 'r':
      if (arg)
        {
          if (!strcmpi (arg, "CONSOLE"))
            in->runas = MYSERVER_RUNAS_CONSOLE;
          else if (!strcmpi (arg, "SERVICE"))
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

    case CONFIG_OPT:
      in->confFilesLocation = arg;
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
 * Load the external path.
 * Return nonzero on errors.
 */
int loadExternalPath (string &externalPath)
{
  try
    {
      externalPath = "";
#ifdef WIN32
      externalPath.assign ("plugins");
#else
      if (FilesUtility::fileExists ("plugins"))
        externalPath.assign ("plugins");
      else
        {
# ifdef PREFIX
          externalPath.assign (PREFIX "/lib/myserver/plugins");
# else
          externalPath.assign ("/usr/local/lib/myserver/plugins");
# endif
        }

#endif
    }
  catch (...)
    {
    }
  return 0;
}


/*!
 * Load the vhost configuration files locations.
 * If DIR is specified, look only in this directory.
 * Return nonzero on errors.
 */
int loadConfFileLocation (string &outFile, string fileName, const char *dir)
{
  try
    {
      outFile = "";

#ifdef WIN32
      outFile = fileName;
#else

      if (dir)
        {
          outFile = dir;
          if (outFile.at (outFile.length () - 1) != '/')
            outFile += "/";

          outFile += fileName;
          return !FilesUtility::fileExists (outFile);
        }

      /* Look for .xml files in the following order:

         1) current working directory
         2) ~/.myserver/
         3) /etc/myserver/
      */
      if (FilesUtility::fileExists (fileName))
        {
          outFile = fileName;
          return 0;
        }

      outFile = "~/.myserver/" + fileName;
      if (FilesUtility::fileExists (outFile))
        return 0;

      outFile = "/etc/myserver/" + fileName;
      if (FilesUtility::fileExists (outFile))
        return 0;
#endif
    }
  catch (...)
    {
    }
  return 1;
}

/*!
 * Load the configuration files locations.
 * Return nonzero on errors.
 */
int loadConfFilesLocation (string &mainConfigurationFile,
                           string &mimeConfigurationFile,
                           string &vhostConfigurationFile,
                           string &externalPath,
                           const char *dir)
{
  if (loadConfFileLocation (mainConfigurationFile, "myserver.xml", dir))
    return -1;

  if (loadConfFileLocation (mimeConfigurationFile, "MIMEtypes.xml", dir))
    return -1;

  if (loadConfFileLocation (vhostConfigurationFile, "virtualhosts.xml", dir))
    return -1;

  if (loadExternalPath (externalPath))
    return -1;

  return 0;
}

int main  (int argn, char **argv)
{
  int runas = MYSERVER_RUNAS_CONSOLE;
#ifdef ARGP
  struct argp_input input;
#endif
#ifndef WIN32
  pid_t pid;
  pid_t sid;
#endif
  string mainConf, mimeConf, vhostConf, externPath;

  ::argn = argn;
  ::argv = argv;

  registerSignals ();

#if HAVE_GETTEXT
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
#endif

  try
    {
      Server::createInstance ();
    }
  catch (...)
    {
      /* Die if we get exceptions here.  */
      return (1);
    };

  {
    u_int pathLen = 0;
    u_int len = 0;
    bool differentCwd = false;
    char *path;

    pathLen = strlen (argv[0]);
    path = new char[pathLen + 1];
    if (path == 0)
      return 1;
    strncpy (path, argv[0], pathLen);

    for (len = 0; len < pathLen; len++)
      {
        if (path[len] == '/' || path[len] == '\\')
          {
            differentCwd = true;
            break;
          }
      }

    /* Current working directory is where the myserver executable is.  */
    if (differentCwd)
      {
        len = pathLen;
        while ((path[len] != '\\') && (path[len] != '/'))
          len--;
        path[len] = '\0';

        setcwd (path);
      }

    /* We can free path memory now.  */
    delete [] path;
  }


#ifdef ARGP
  /* Reset the struct.  */
  input.version = 0;
  input.confFilesLocation = NULL;
  input.logFileName = NULL;
  input.runas = MYSERVER_RUNAS_CONSOLE;
  input.pidFileName = NULL;
  input.useForkServer = 0;

  /* Call the parser.  */
  argp_parse (&myserverArgp, argn, argv, 0, 0, &input);
  runas = input.runas;

  if (input.logFileName
      && Server::getInstance ()->setLogLocation (input.logFileName))
    {
      cout << "Error setting the location for the MyServer's main log" << endl;
      return 1;
    }

  /* If the version flag is up, show the version and exit.  */
  if (input.version)
    {
      cout << "GNU MyServer " << MYSERVER_VERSION << endl
           << "Copyright (C) 2009 Free Software Foundation, Inc." << endl
           << "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>." << endl
           << "This is free software: you are free to change and redistribute it." << endl
           << "There is NO WARRANTY, to the extent permitted by law." << endl
           << endl
           << "http://www.gnu.org/software/myserver" << endl;
      return 0;
    }
#else
  if (argn > 1)
    {
      if (!strcmpi (argv[1], "VERSION"))
        {
          cout << MYSERVER_VERSION << endl;
          return 0;
        }
      if (!strcmpi (argv[1], "CONSOLE"))
        {
          runas = MYSERVER_RUNAS_CONSOLE;
        }
      if (!strcmpi (argv[1], "REGISTER"))
        {
          registerService ();
# ifndef ARGP
          runAsService ();
# endif
          runas = MYSERVER_RUNAS_SERVICE;
          return 0;
        }
      if (!strcmpi (argv[1], "RUNSERVICE"))
        {
          runAsService ();
          return 0;
        }
      if (!strcmpi (argv[1], "UNREGISTER"))
        {
          removeService ();
          runas = MYSERVER_RUNAS_SERVICE;
          return 0;
        }
      if (!strcmpi (argv[1], "SERVICE"))
        {
          /*
           * Set the log file to use when in service mode.
           */
          runas = MYSERVER_RUNAS_SERVICE;
        }
    }
#endif

#ifdef ARGP
  if (input.useForkServer)
    {
      FilesUtility::resetTmpPath ();
      Process::getForkServer ()->startForkServer ();
    }
#endif

  /*
   * Start here the MyServer execution.
   */
  try
    {
      setcwdBuffer ();
      loadConfFilesLocation (mainConf, mimeConf, vhostConf, externPath,
                             input.confFilesLocation);

      switch (runas)
        {
        case MYSERVER_RUNAS_CONSOLE:
          consoleService (mainConf, mimeConf, vhostConf, externPath);
          break;
        case MYSERVER_RUNAS_SERVICE:
#ifdef WIN32
          runService ();
#else
          /*
           * Run the daemon.
           * pid is the process ID for the forked process.
           * Fork the process.
           */
          pid = fork ();

          /*
           * An error happened, return with errors.
           */
          if (pid < 0)
            {
              return 1;
            }

          if (pid)
            {
# ifdef ARGP
              if (input.pidFileName)
                writePidfile (input.pidFileName);
              else
                writePidfile ();
# else
              writePidfile ();
# endif
              return 0;
            }
          /*
           * Create a SID for the new process.
           */
          sid = setsid ();
          if (sid < 0)
            return 1;

          consoleService (mainConf, mimeConf, vhostConf, externPath);
#endif
          break;
        }
    }
  catch (...)
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
 * Write the current PID to the file.
 */
int writePidfile (const char* filename)
{
  int pidfile;
  pid_t pid = getpid ();
  char buff[12];
  int ret;
  string file = "";

  if (filename == NULL)
    {
# ifdef PREFIX
      file.assign (PREFIX);
      file.append ("/");
# endif
      file.append ("/var/run/myserver.pid");
    }

  pidfile = open (filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

  if (pidfile == -1)
    return -1;

  sprintf (buff,"%i\n", pid);
  ret = write (pidfile, buff, strlen (buff));
  if (ret == -1)
    {
      close (pidfile);
      return -1;
    }
  return close (pidfile);
}
#endif

/*!
 * Start MyServer in console mode.
 */
void consoleService (string &mainConf, string &mimeConf, string &vhostConf, string &externPath)
{
  Server::getInstance ()->start (mainConf, mimeConf, vhostConf, externPath);
}


/*!
 * These functions are available only on the windows platform.
 */
#ifdef WIN32
SERVICE_STATUS          MyServiceStatus;
SERVICE_STATUS_HANDLE   MyServiceStatusHandle;

/*!
 * Entry-point for the NT service.
 */
void  __stdcall myServerMainNT (u_long, LPTSTR*)
{
  string mainConf, mimeConf, vhostConf, externPath;

  MyServiceStatus.dwServiceType = SERVICE_WIN32;
  MyServiceStatus.dwCurrentState = SERVICE_STOPPED;
  MyServiceStatus.dwControlsAccepted = 0;
  MyServiceStatus.dwWin32ExitCode = NO_ERROR;
  MyServiceStatus.dwServiceSpecificExitCode = NO_ERROR;
  MyServiceStatus.dwCheckPoint = 0;
  MyServiceStatus.dwWaitHint = 0;

  try
    {
      Server::createInstance ();
    }
  catch (...)
    {
      /* Die if we get exceptions here.  */
      return;
    };


  MyServiceStatusHandle = RegisterServiceCtrlHandler ("GNU MyServer",
                                                      myServerCtrlHandler);
  if (MyServiceStatusHandle)
    {
      MyServiceStatus.dwCurrentState = SERVICE_START_PENDING;
      SetServiceStatus (MyServiceStatusHandle, &MyServiceStatus);

      MyServiceStatus.dwControlsAccepted |= (SERVICE_ACCEPT_STOP
                                             | SERVICE_ACCEPT_SHUTDOWN);
      MyServiceStatus.dwCurrentState = SERVICE_RUNNING;
      SetServiceStatus (MyServiceStatusHandle, &MyServiceStatus);


      loadConfFilesLocation (mainConf, mimeConf, vhostConf, externPath, NULL);
      Server::getInstance ()->start (mainConf, mimeConf, vhostConf, externPath);

      MyServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
      SetServiceStatus (MyServiceStatusHandle, &MyServiceStatus);

      MyServiceStatus.dwControlsAccepted &= ~(SERVICE_ACCEPT_STOP
                                              | SERVICE_ACCEPT_SHUTDOWN);
      MyServiceStatus.dwCurrentState = SERVICE_STOPPED;
      SetServiceStatus (MyServiceStatusHandle, &MyServiceStatus);
    }

}

/*!
 * Manage the NT service.
 */
void __stdcall myServerCtrlHandler (u_long fdwControl)
{
  switch (fdwControl)
    {
    case SERVICE_CONTROL_INTERROGATE:
      break;

    case SERVICE_CONTROL_SHUTDOWN:
    case SERVICE_CONTROL_STOP:
      MyServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
      SetServiceStatus (MyServiceStatusHandle, &MyServiceStatus);
      Server::getInstance ()->stop ();
      return;

    case SERVICE_CONTROL_PAUSE:
      break;

    case SERVICE_CONTROL_CONTINUE:
      break;
    }

  SetServiceStatus (MyServiceStatusHandle, &MyServiceStatus);
}
#endif

/*!
 * Run MyServer service.
 */
void runService ()
{
  Server::getInstance ()->log ("Running service...");
#ifdef WIN32
  SERVICE_TABLE_ENTRY serviceTable[] =
    {
      { (CHAR*) "GNU MyServer", myServerMainNT },
      { 0, 0 }
    };

  if (!StartServiceCtrlDispatcher (serviceTable))
    {
      if (GetLastError () == ERROR_INVALID_DATA)
        Server::getInstance ()->log ("Invalid data");
      else if (GetLastError () == ERROR_SERVICE_ALREADY_RUNNING)
        Server::getInstance ()->log ("Already running");
      else
        Server::getInstance ()->log ("Error running service");
    }
#endif
}

/*!
 * Register the service.
 */
void registerService ()
{
#ifdef WIN32
  SC_HANDLE service, manager;
  char path [MAX_PATH];
  GetCurrentDirectory (MAX_PATH, path);
  strcat (path, "\\");
  strcat (path, "myserver.exe SERVICE");

  manager = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if (manager)
    {
      service = CreateService (manager, "GNU MyServer", "GNU MyServer",
                               SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
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
void removeService ()
{
#ifdef WIN32
  SC_HANDLE service,manager;
  manager = OpenSCManager (NULL,NULL,SC_MANAGER_ALL_ACCESS);
  if (manager)
    {
      service = OpenService (manager, "GNU MyServer", SERVICE_ALL_ACCESS);
      if (service)
        {
          ControlService (service, SERVICE_CONTROL_STOP,&MyServiceStatus);
          while (QueryServiceStatus (service, &MyServiceStatus))
            if (MyServiceStatus.dwCurrentState != SERVICE_STOP_PENDING)
              break;
          DeleteService (service);
          CloseServiceHandle (service);
          CloseServiceHandle (manager);
        }
    }
#endif
}

/*!
 * Start the service.
 */
void runAsService ()
{
#ifdef WIN32
  SC_HANDLE service, manager;

  manager = OpenSCManager (NULL,NULL,SC_MANAGER_ALL_ACCESS);
  if (manager)
    {
      service = OpenService (manager, "GNU MyServer", SERVICE_ALL_ACCESS);
      if (service)
        {
          StartService (service, 0, NULL);

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
