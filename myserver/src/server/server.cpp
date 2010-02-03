/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
  Free Software Foundation, Inc.
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

#include "myserver.h"
#include <include/server/server.h>
#include <include/server/clients_thread.h>
#include <include/base/safetime/safetime.h>
#include <include/protocol/http/http.h>
#include <include/protocol/https/https.h>
#include <include/protocol/control/control_protocol.h>
#include <include/protocol/gopher/gopher.h>
#include <include/protocol/ftp/ftp.h>
#include <include/base/string/stringutils.h>
#include <include/base/socket/socket.h>
#include <include/filter/gzip/gzip.h>
#include <include/base/regex/myserver_regex.h>
#include <include/base/file/files_utility.h>
#include <include/base/ssl/ssl.h>
#include <include/base/socket/ssl_socket.h>

#include <include/base/crypt/md5.h>
#include <include/base/crypt/sha1.h>

#include <include/conf/main/xml_main_configuration.h>

#include <cstdarg>


extern "C"
{
#ifdef WIN32
# include <direct.h>
#else
# include <unistd.h>
# include <string.h>
# include <signal.h>
# include <stdlib.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>

# ifdef HAVE_PTHREAD
#  include <pthread.h>
# endif

#endif
}
#include <sstream>

Server* Server::instance = NULL;

Server::Server () : connectionsScheduler (this),
                   listenThreads (&connectionsScheduler, this),
                   authMethodFactory (&cryptAlgoManager),
                   validatorFactory (),
                   securityManager (&validatorFactory, &authMethodFactory),
                   connectionsPool (100)
{
  toReboot = false;
  autoRebootEnabled = true;
  rebooting = false;
  maxConnections = 0;
  serverReady = false;
  throttlingRate = 0;
  path = 0;
  ipAddresses = 0;
  vhostHandler = NULL;
  purgeThreadsThreshold = 1;
  freeThreads = 0;
  logManager = new LogManager (&filtersFactory);
  xmlValidator = new XmlValidator ();
  initLogManager ();
  connectionsPoolLock.init ();
  configurationFileManager = NULL;
  genMainConf = NULL;
}

void
Server::initLogManager ()
{
  if (!logLocation.size ())
    logLocation.assign ("console://stdout");

  if (!logManager->containsOwner (this))
    {
      list<string> filters;
      logManager->add (this, "MAINLOG", logLocation, filters, 0);
    }
}

/*!
 *Reinitialize the configuration paths, setting them to the specified ones.
 *Returns false on error.
 */
bool Server::resetConfigurationPaths (string &mainConf, string &mimeConf,
				     string &vhostConf, string &externPath)
{
  mainConfigurationFile = mainConf;
  mimeConfigurationFile = mimeConf;
  vhostConfigurationFile = vhostConf;
  externalPath = externPath;
  return true;
}

/*!
 * Load here all the libraries.
 */
int Server::loadLibraries ()
{
  /* Initialize the SSL library.  */
  initializeSSL ();
  Process::initialize ();
  XmlParser::startXML ();
  myserver_safetime_init ();

  gnutls_global_init ();
  if (Socket::startupSocketLib () != 0)
    {
      log (MYSERVER_LOG_MSG_ERROR, _("Error loading the socket library"));
      return 1;
    }

  return 0;
}

/*!
 * Destroy the object.
 */
Server::~Server ()
{
  if (vhostHandler)
    delete vhostHandler;

  if (xmlValidator)
    delete xmlValidator;

  if (path)
    delete path;
  path = 0;

  if (ipAddresses)
    delete ipAddresses;

  if (logManager)
    delete logManager;

  if (configurationFileManager)
    delete configurationFileManager;

  connectionsPoolLock.destroy ();
}

/*!
 *Start the server.
 */
void Server::start (string &mainConf, string &mimeConf, string &vhostConf,
                    string &externPath, MainConfiguration* (*genMainConf)
                    (Server *server, const char *arg))

{
  int err = 0;
#ifdef WIN32
  DWORD eventsCount, cNumRead;
  INPUT_RECORD irInBuf[128];
#endif

  this->genMainConf = genMainConf;

  displayBoot ();

  try
  {
    log (MYSERVER_LOG_MSG_INFO, _("Initializing server configuration..."));

    if (loadLibraries ())
      {
        log (MYSERVER_LOG_MSG_INFO, _("The server could not be started"));
        return;
      }

    if (!resetConfigurationPaths (mainConf, mimeConf, vhostConf, externPath))
      {
        log (MYSERVER_LOG_MSG_INFO, _("The server could not be started"));
        return;
      }

    err = initialize ();
    if (err)
      {
        log (MYSERVER_LOG_MSG_INFO, _("The server could not be started"));
        return;
      }

    log (MYSERVER_LOG_MSG_INFO, _("Loading server configuration..."));

    if (postLoad ())
      {
        log (MYSERVER_LOG_MSG_INFO, _("The server could not be started"));
        return;
      }

    setProcessPermissions ();

    if (getGid ()[0])
      log (MYSERVER_LOG_MSG_INFO, _("Using gid: %s"), gid.c_str ());

    if (getUid ()[0])
      log (MYSERVER_LOG_MSG_INFO, _("Using uid: %s"), uid.c_str ());

    log (MYSERVER_LOG_MSG_INFO, _("Server is ready!"));

    if (logLocation.find ("console://") != string::npos)
      log (MYSERVER_LOG_MSG_INFO, _("Press Ctrl-C to terminate its execution"));

    serverReady = true;

    /* Finally we can give control to the main loop.  */
    mainLoop ();

  }
  catch (bad_alloc &ba)
    {
      log (MYSERVER_LOG_MSG_ERROR, _("Bad alloc: %s"), ba.what ());
    }
  catch (exception &e)
    {
      log (MYSERVER_LOG_MSG_ERROR, _("Error: %s"), e.what ());
    };

  this->terminate ();
  finalCleanup ();

#ifdef WIN32
  WSACleanup ();
#endif

  log (MYSERVER_LOG_MSG_INFO, _("Server terminated"));
}

/*!
 * Complete the loading phase.
 */
int Server::postLoad ()
{
  string strCPU;
  string buffer;

  /* Get the name of the local machine.  */
  memset (serverName, 0, HOST_NAME_MAX + 1);
  Socket::gethostname (serverName, HOST_NAME_MAX);

  log (MYSERVER_LOG_MSG_INFO, _("Host name: %s"), serverName);

  /* Find the IP addresses of the local machine.  */
  if (ipAddresses)
    delete ipAddresses;
  ipAddresses = new string ();

  if (Socket::getLocalIPsList (*ipAddresses))
    {
      log (MYSERVER_LOG_MSG_ERROR, _("Error reading IP list"));
      return -1;
    }

  log (MYSERVER_LOG_MSG_INFO, _("IP: %s"), ipAddresses->c_str ());

  log (MYSERVER_LOG_MSG_INFO, _("Detected %i CPUs"), (int) getCPUCount ());

  connectionsScheduler.restart ();

  listenThreads.initialize ();

  getProcessServerManager ()->load ();

  if (path == NULL)
    path = new string ();

  if (getdefaultwd (*path))
    return -1;

  /* Load the home directories configuration.  */
  homeDir.load ();

  loadStaticComponents ();
  loadPlugins ();

  if (loadVHostConf ())
    return -1;

  /* Load the MIME types.  */
  log (MYSERVER_LOG_MSG_INFO, _("Loading MIME types..."));

  string handlerStr (getData ("server.mime_handler", "xml"));
  mimeManager.setDefaultHandler (handlerStr);

  MimeManagerHandler *mimeHandler = mimeManager.getDefaultHandler ();
  if (!mimeHandler)
    {
      log (MYSERVER_LOG_MSG_ERROR,
          _("Error creating the MIME types handler: %s"), handlerStr.c_str ());
      return -1;
    }

  if (int nMIMEtypes = mimeHandler->load (mimeConfigurationFile.c_str ()))
    log (MYSERVER_LOG_MSG_INFO, _("Using %i MIME types"), nMIMEtypes);
  else
    log (MYSERVER_LOG_MSG_ERROR, _("Error while loading MIME types"));

  for (u_long i = 0; i < nStaticThreads; i++)
    {
      log (MYSERVER_LOG_MSG_INFO, _("Creating thread %i..."), (int) (i + 1));

      if (addThread (true))
        {
          log (MYSERVER_LOG_MSG_ERROR, _("Error while creating thread"));
          return -1;
        }

      log (MYSERVER_LOG_MSG_INFO, _("Thread %i created"),  (int)(i + 1));
    }

  configurationFileManager->close ();
  delete configurationFileManager;
  configurationFileManager = NULL;

  return 0;
}

/*!
 * Reload the virtual hosts configuration.
 */
int Server::loadVHostConf ()
{
  string vhostHandlerStr (getData ("server.vhost_handler", "xml"));
  vhostHandler = vhostManager.buildHandler (vhostHandlerStr, &listenThreads,
                                            logManager);

  if (!vhostHandler)
    {
      log (MYSERVER_LOG_MSG_ERROR,
           _("Error creating the virtual hosts handler: %s"),
           vhostHandlerStr.c_str ());
      return -1;
    }

  vhostManager.setHandler (vhostHandler);

  /* Load the virtual hosts configuration.  */
  if (vhostHandler->load (vhostConfigurationFile.c_str ()))
    {
      log (MYSERVER_LOG_MSG_ERROR,
           _("Error loading the vhost configuration file %s"),
           vhostConfigurationFile.c_str ());
      return -1;
    }

  return 0;
}

/*!
 * Load static built-in components.
 */
void Server::loadStaticComponents ()
{
  string xml ("xml");

  mimeManager.registerHandler (xml, new XmlMimeHandler ());

  XmlVhostHandler::registerBuilder (vhostManager);
  XmlMimeHandler::registerBuilder (mimeManager);

  validatorFactory.addValidator (xml, xmlValidator);
  authMethodFactory.addAuthMethod (xml, (AuthMethod*) xmlValidator);

  if (filtersFactory.insert ("gzip", Gzip::factory))
    log (MYSERVER_LOG_MSG_ERROR, _("Error while loading plugins"));

  HttpProtocol *http = new HttpProtocol ();
  Protocol *protocolsSet[] = {http,
                              new HttpsProtocol (http),
                              new GopherProtocol (),
                              new FtpProtocol (),
                              new ControlProtocol (),
                              NULL};

  for (int j = 0; protocolsSet[j]; j++)
    {
      Protocol *protocol = protocolsSet[j];
      protocol->loadProtocol ();
      const char *protocolName = protocol->getName ();
      getProtocolsManager ()->addProtocol (protocolName, protocol);
    }

  Md5::initialize (&cryptAlgoManager);
  Sha1::initialize (&cryptAlgoManager);
}

/*!
 * Load the plugins.
 */
void Server::loadPlugins ()
{
  getPluginsManager ()->preLoad (this, externalPath);
  getPluginsManager ()->load (this);
  getPluginsManager ()->postLoad (this);
}

/*!
 * Server main loop.
 */
void Server::mainLoop ()
{
  time_t mainConfTime;
  time_t hostsConfTime;
  time_t mimeConfTime;

  u_long purgeThreadsCounter = 0;

  mainConfTime = FilesUtility::getLastModTime (mainConfigurationFile.c_str ());
  hostsConfTime = FilesUtility::getLastModTime (vhostConfigurationFile.c_str ());
  mimeConfTime = FilesUtility::getLastModTime (mimeConfigurationFile.c_str ());

  /*
   * Keep thread alive.
   * When the endServer flag is set to True exit
   * from the loop and terminate the server execution.
   */
  while (!endServer)
    {
      Thread::wait (1000000);

      /* Check threads.  */
      if (purgeThreadsCounter++ >= 10)
        {
          purgeThreadsCounter = 0;
          purgeThreads ();
        }

      if (autoRebootEnabled)
        {
          time_t mainConfTimeNow =
            FilesUtility::getLastModTime (mainConfigurationFile.c_str ());
          time_t hostsConfTimeNow =
            FilesUtility::getLastModTime (vhostConfigurationFile.c_str ());
          time_t mimeConfNow =
            FilesUtility::getLastModTime (mimeConfigurationFile.c_str ());

          /* If a configuration file was modified reboot the server. */
          if (((mainConfTimeNow != -1) && (hostsConfTimeNow != -1)  &&
               (mimeConfNow != -1)) || toReboot)
            {
              if ( (mainConfTimeNow  != mainConfTime) || toReboot)
                {
                  string msg ("main-conf-changed");
                  notifyMulticast (msg, 0);

                  reboot ();
                  /* Store new mtime values.  */
                  mainConfTime = mainConfTimeNow;
                  mimeConfTime = mimeConfNow;
                }
              else if (mimeConfNow != mimeConfTime)
                {
                  string msg ("mime-conf-changed");
                  notifyMulticast (msg, 0);

                  if (logLocation.find ("console://") != string::npos)
                    {
                      char beep[] = { static_cast<char>(0x7), '\0' };
                      string beepStr (beep);
                      logManager->log (this, "MAINLOG", logLocation, beepStr);
                    }
                  log (MYSERVER_LOG_MSG_INFO, _("Reloading MIME types"));

                  getMimeManager ()->reload ();

                  log (MYSERVER_LOG_MSG_INFO, _("Reloaded"));

                  mimeConfTime = mimeConfNow;
                }
              else if (hostsConfTimeNow != hostsConfTime)
                {
                  VhostManagerHandler* oldvhost = vhostHandler;
                  string msg ("vhosts-conf-changed");
                  notifyMulticast (msg, 0);

                  /* Do a beep if outputting to console.  */
                  if (logLocation.find ("console://") != string::npos)
                    {
                      char beep[] = { static_cast<char>(0x7), '\0' };
                      string beepStr (beep);
                      logManager->log (this, "MAINLOG", logLocation, beepStr);
                    }

                  log (MYSERVER_LOG_MSG_INFO, _("Rebooting..."));

                  connectionsScheduler.release ();

                  listenThreads.beginFastReboot ();

                  listenThreads.terminate ();

                  clearAllConnections ();

                  connectionsScheduler.restart ();
                  listenThreads.initialize ();

                  delete oldvhost;

                  if (loadVHostConf ())
                    listenThreads.rollbackFastReboot ();
                  else
                    listenThreads.commitFastReboot ();

                  hostsConfTime = hostsConfTimeNow;
                  log (MYSERVER_LOG_MSG_INFO, _("Reloaded"));
                }
            }
        }
    }
}

/*!
 * Display the MyServer boot.
 */
void Server::displayBoot ()
{

#ifdef CLEAR_BOOT_SCREEN

  if (logLocation.find ("console://") != string::npos)
    {
# ifdef WIN32
      _flushall ();
      system ("cls");
# else
      sync ();
      system ("clear");
# endif
    }

#endif /* CLEAR_BOOT_SCREEN.  */

  /*
   * Print the MyServer signature only if the log writes to the console.
   */
  if (logLocation.find ("console://") != string::npos)
    {
      try
        {
          size_t length;
          string softwareSignature = "************ GNU MyServer "
            MYSERVER_VERSION " ************";

          string tmp (softwareSignature.length (), '*');

          logManager->log (this, "MAINLOG", logLocation, tmp, true);
          logManager->log (this, "MAINLOG", logLocation, softwareSignature, true);
          logManager->log (this, "MAINLOG", logLocation, tmp, true);
        }
      catch (exception& e)
        {
          ostringstream err;
          err << "Error: " << e.what ();
          string str = err.str ();
          logManager->log (this, "MAINLOG", logLocation, str, true);
          return;
        }
      catch (...)
        {
          return;
        };
    }

}

/*!
 * Removed threads that can be destroyed.
 * The function returns the number of threads that were destroyed.
 */
int Server::purgeThreads ()
{
  u_long ticks = getTicks ();
  u_long destroyed = 0;

  purgeThreadsThreshold = std::min (purgeThreadsThreshold << 1,
                                    nMaxThreads);
  threadsMutex->lock ();
  for (list<ClientsThread*>::iterator it = threads.begin ();
       it != threads.end ();)
    {
      ClientsThread* thread = *it;


      /*
       *Shutdown all threads that can be destroyed.
       */
      if (thread->isStopped ())
        {
          ClientsThread* thread = *it;
          list<ClientsThread*>::iterator next = it;
          next++;

          thread->join ();
          threads.erase (it);
          delete thread;

          destroyed++;
          it = next;
        }
      else if (thread->isToDestroy ())
        {
          if (destroyed < purgeThreadsThreshold)
            thread->stop ();

          it++;
        }
      else
        {
          if (!thread->isStatic ())
            if (ticks - thread->getTimeout () > MYSERVER_SEC (15))
              thread->setToDestroy (1);

          it++;
        }
    }
  threadsMutex->unlock ();

  return threads.size ();
}

/*!
 * Do the final cleanup.  Called once when the process is terminated.
 */
void Server::finalCleanup ()
{
  XmlParser::cleanXML ();
  freecwd ();
  myserver_safetime_destroy ();
}

/*!
 * Return the user identifier to use for the process.
 */
const char *Server::getUid ()
{
  return uid.c_str ();
}

/*!
 * Return the group identifier to use for the process.
 */
const char *Server::getGid ()
{
  return gid.c_str ();
}

/*!
 * Returns the numbers of active connections the list.
 */
u_long Server::getNumConnections ()
{
  return connectionsScheduler.getNumAliveConnections ();
}

/*!
 * Returns the numbers of all the connections to the server.
 */
u_long Server::getNumTotalConnections ()
{
  return connectionsScheduler.getNumTotalConnections ();
}

/*!
 * Return a home directory object.
 */
HomeDir* Server::getHomeDir ()
{
  return &homeDir;
}

/*!
 * Stop the execution of the server.
 */
void Server::stop ()
{
  endServer = true;
}

/*!
 * Unload the server.
 * Return nonzero on errors.
 */
int Server::terminate ()
{
  log (MYSERVER_LOG_MSG_INFO, _("Stopping threads"));

  listenThreads.terminate ();

  threadsMutex->lock ();

  for (list<ClientsThread*>::iterator it = threads.begin ();
       it != threads.end (); it++)
    (*it)->stop ();

  threadsMutex->unlock ();

  connectionsScheduler.release ();

  for (list<ClientsThread*>::iterator it = threads.begin ();
       it != threads.end (); it++)
    {
      Thread::join ((*it)->getThreadId ());
      delete *it;
    }

  clearAllConnections ();

  /* Clear the home directories data.  */
  homeDir.clear ();

  log (MYSERVER_LOG_MSG_INFO, _("Threads stopped"));

  log (MYSERVER_LOG_MSG_INFO, _("Cleaning memory"));

  freeHashedData ();

  ipAddresses = NULL;

  if (vhostHandler)
    delete vhostHandler;
  vhostHandler = NULL;

#ifdef WIN32
  /*
   * Under WIN32 cleanup environment strings.
   */
  FreeEnvironmentStrings ((LPTSTR)envString);
#endif

  getProcessServerManager ()->clear ();

  filtersFactory.free ();

  getPluginsManager ()->unLoad ();

  delete connectionsMutex;

  clearMulticastRegistry ();

  /*
   * Free all the threads.
   */
  threadsMutex->lock ();
  threads.clear ();
  threadsMutex->unlock ();
  delete threadsMutex;

  nStaticThreads = 0;

  log (MYSERVER_LOG_MSG_INFO, _("MyServer has stopped"));

  logManager->clear ();

  return 0;
}

/*!
 * Get a pointer to the configuration file.  It is
 * valid only at startup!
 */
MainConfiguration *Server::getConfiguration ()
{
  return configurationFileManager;
}

/*!
 * Here is loaded the configuration of the server.
 * The configuration file is a XML file.
 * Return nonzero on errors.
 */
int Server::initialize ()
{
  const char *data;
#ifdef WIN32
  envString = GetEnvironmentStrings ();
#endif
  connectionsMutex = new Mutex ();
  threadsMutex = new Mutex ();

  /* Store the default values.  */
  nStaticThreads = 20;
  nMaxThreads = 50;
  currentThreadID = 0;
  freeThreads = 0;
  connectionTimeout = MYSERVER_SEC (180);
  endServer = false;
  purgeThreadsThreshold = 1;
  throttlingRate = 0;
  maxConnections = 0;
  maxConnectionsToAccept = 0;

  if (genMainConf)
    {
      configurationFileManager = genMainConf (this,
                                              mainConfigurationFile.c_str ());
      if (!configurationFileManager)
        {
          log (MYSERVER_LOG_MSG_ERROR,
               _("Error while loading the %s configuration file"),
               mainConfigurationFile.c_str ());
          return -1;
        }

    }
  else
    {
      XmlMainConfiguration *xmlMainConf = new XmlMainConfiguration ();
      if (xmlMainConf->open (mainConfigurationFile.c_str ()))
        {
          log (MYSERVER_LOG_MSG_ERROR,
               _("Error while loading the %s configuration file"),
               mainConfigurationFile.c_str ());
          delete xmlMainConf;
          return -1;
        }
      configurationFileManager = xmlMainConf;
    }

  configurationFileManager->readData (&hashedDataTrees, &hashedData);

  /*
   * Process console colors information.
   */
  list<string> levels = logManager->getLoggingLevelsByNames ();
  for (list<string>::iterator it = levels.begin (); it != levels.end (); it++)
    {
      string fg (*it + "_fg");
      string bg (*it + "_bg");

      string fullFg ("log_color." + *it + "_fg");
      string fullBg ("log_color." + *it + "_bg");
      data = getData (fullFg.c_str ());
      if (data)
        consoleColors[fg] = string (data);

      data = getData (fullBg.c_str ());
      if (data)
        consoleColors[bg] = string (data);
    }

  initLogManager ();

  data = getData ("server.buffer_size");
  if (data)
    buffersize = (atol (data) > 81920) ?  atol (data) :  81920 ;

  data = getData ("server.connection_timeout");
  if (data)
    connectionTimeout = MYSERVER_SEC ((u_long) atol (data));

  data = getData ("server.static_threads");
  if (data)
    nStaticThreads = atoi (data);

  data = getData ("server.max_threads");
  if (data)
    nMaxThreads = atoi (data);

  /* Get the max connections number to allow.  */
  data = getData ("server.max_connections");
  if (data)
    maxConnections = atoi (data);

  /* Get the max connections number to accept.  */
  data = getData ("server.max_accepted_connections");
  if (data)
    maxConnectionsToAccept = atoi (data);

  data = getData ("server.connections_pool.size");
  if (data)
    connectionsPool.init (atoi (data));

  /* Get the default throttling rate to use on connections.  */
  data = getData ("connection.throttling");
  if (data)
    throttlingRate = (u_long) atoi (data);

  data = getData ("server.max_log_size");
  if (data)
    maxLogFileSize=(u_long) atol (data);

  data = getData ("server.max_files_cache");
  if (data)
    {
      u_long maxSize = (u_long) atol (data);
      cachedFiles.initialize (maxSize);
    }
  else
    cachedFiles.initialize (1 << 23);

  data = getData ("server.temp_directory");
  if (data)
    {
      string tmpPath (data);
      FilesUtility::completePath (tmpPath);
      FilesUtility::setTmpPath (tmpPath);
    }
  else
    FilesUtility::resetTmpPath ();

  data = getData ("server.max_file_cache");
  if (data)
    {
      u_long maxSize = (u_long) atol (data);
      cachedFiles.setMaxSize (maxSize);
    }

  data = getData ("server.min_file_cache");
  if (data)
    {
      u_long minSize = (u_long) atol (data);
      cachedFiles.setMinSize (minSize);
    }

  data = getData ("server.uid");
  if (data)
    uid.assign (data);
  else
    uid.assign ("");

  data = getData ("server.gid");
  if (data)
    gid.assign (data);
  else
    gid.assign ("");

  data = getData ("server.max_servers");
  if (data)
    {
      int maxServersProcesses = atoi (data);
      getProcessServerManager ()->setMaxServers (maxServersProcesses);
    }

  return 0;
}

/*!
 * Check if there are free threads to handle a new request.  If there
 * are not enough threads create a new one.
 */
void Server::checkThreadsNumber ()
{
  threadsMutex->lock ();

  /*
   *Create a new thread if there are not available threads and
   *we did not reach the limit.
   */
  if ((threads.size () < nMaxThreads) && (freeThreads < 1))
    addThread (false);

  threadsMutex->unlock ();
}

/*!
 * Get the default throttling rate to use with connections to the server.
 */
u_long Server::getThrottlingRate ()
{
  return throttlingRate;
}

/*!
 * This function returns the max size of the logs file as defined in the
 * configuration file.
 */
int Server::getMaxLogFileSize ()
{
  return maxLogFileSize;
}

/*!
 * Returns the connection timeout.
 */
u_long Server::getTimeout ()
{
  return connectionTimeout;
}

/*!
 * This function add a new connection to the list.
 * On a failure, it is responsibility of the caller to
 * free S.
 * \return 0 on success.
 */
int Server::addConnection (Socket *s, MYSERVER_SOCKADDRIN *asockIn)
{
  char ip[MAX_IP_STRING_LEN];
  char localIp[MAX_IP_STRING_LEN];
  u_short port;
  u_short myPort;
  MYSERVER_SOCKADDRIN  localSockIn = { 0 };
  int dim;

  /*
   * We can use MAX_IP_STRING_LEN only because we use NI_NUMERICHOST
   * in getnameinfo call; Otherwise we should have used NI_MAXHOST.
   * ip is the string containing the address of the remote host connecting
   * to the server.
   * localIp is the local address used by the connection.
   * port is the remote port used by the client to open the connection.
   * myPort is the port used by the server to listen.
   */
  if ( asockIn == NULL ||
       (asockIn->ss_family != AF_INET && asockIn->ss_family != AF_INET6))
    return -1;

  memset (ip, 0, MAX_IP_STRING_LEN);
  memset (localIp, 0, MAX_IP_STRING_LEN);

  if (s->getHandle () < 0)
    return -1;

  /*
   * Do not accept this connection if a MAX_CONNECTIONS_TO_ACCEPT limit is
   * defined.
   */
  if (maxConnectionsToAccept && connectionsScheduler.getNumAliveConnections ()
      >= maxConnectionsToAccept)
    return -1;

#if ( HAVE_IPV6 )
  if ( asockIn->ss_family == AF_INET )
    ret = getnameinfo (reinterpret_cast<const sockaddr *>(asockIn),
                       sizeof (sockaddr_in),
                       ip, MAX_IP_STRING_LEN, NULL, 0, NI_NUMERICHOST);
  else
    ret = getnameinfo (reinterpret_cast<const sockaddr *>(asockIn),
                       sizeof (sockaddr_in6),  ip, MAX_IP_STRING_LEN,
                       NULL, 0, NI_NUMERICHOST);
  if (ret)
    return -1;

  if ( asockIn->ss_family == AF_INET )
    dim = sizeof (sockaddr_in);
  else
    dim = sizeof (sockaddr_in6);
  s->getsockname ((MYSERVER_SOCKADDR*)&localSockIn, &dim);

  if ( asockIn->ss_family == AF_INET )
    ret = getnameinfo (reinterpret_cast<const sockaddr *>(&localSockIn),
                       sizeof (sockaddr_in), localIp, MAX_IP_STRING_LEN,
                       NULL, 0, NI_NUMERICHOST);
  else// AF_INET6
    ret = getnameinfo (reinterpret_cast<const sockaddr *>(&localSockIn),
                       sizeof (sockaddr_in6), localIp, MAX_IP_STRING_LEN,
                       NULL, 0, NI_NUMERICHOST);
  if (ret)
    return -1;
#else// !HAVE_IPV6
  dim = sizeof (localSockIn);
  s->getsockname ((MYSERVER_SOCKADDR*)&localSockIn, &dim);
  strncpy (ip,  inet_ntoa (((sockaddr_in *)asockIn)->sin_addr),
           MAX_IP_STRING_LEN);
  strncpy (localIp, inet_ntoa (((sockaddr_in *)&localSockIn)->sin_addr),
           MAX_IP_STRING_LEN);
#endif//HAVE_IPV6

  /* Port used by the client.  */
  if ( asockIn->ss_family == AF_INET )
    port = ntohs (((sockaddr_in *)(asockIn))->sin_port);
#if HAVE_IPV6
  else
    port = ntohs (((sockaddr_in6 *)(asockIn))->sin6_port);
#endif

  /* Port used by the server. */
  if ( localSockIn.ss_family == AF_INET )
    myPort = ntohs (((sockaddr_in *)(&localSockIn))->sin_port);
#if HAVE_IPV6
  else
    myPort = ntohs (((sockaddr_in6 *)(&localSockIn))->sin6_port);
#endif

  return addConnectionToList (s, asockIn, &ip[0], &localIp[0], port, myPort, 1) ? 0 : -1;
}

/*!
 * Return the max number of threads that the server can start.
 */
int Server::getMaxThreads ()
{
  return nMaxThreads;
}

/*!
 * Add a new connection.
 *A connection is defined using a connection struct.
 */
ConnectionPtr Server::addConnectionToList (Socket* s,
                                           MYSERVER_SOCKADDRIN* /*asockIn*/,
                                           char *ipAddr, char *localIpAddr,
                                           u_short port, u_short localPort,
                                           int /*id*/)
{
  int doSSLhandshake = 0;
  int doFastCheck = 0;
  Protocol* protocol;
  int opts = 0;
  ConnectionPtr newConnection;
  vector<Multicast<string, void*, int>*>* handlers;

  connectionsPoolLock.lock ();
  newConnection = connectionsPool.forcedGet ();
  connectionsPoolLock.unlock ();

  if (!newConnection)
    return NULL;
  else
    newConnection->init ();

  newConnection->setPort (port);
  newConnection->setTimeout ( getTicks () );
  newConnection->setLocalPort (localPort);
  newConnection->setIpAddr (ipAddr);
  newConnection->setLocalIpAddr (localIpAddr);
  newConnection->host = vhostManager.getVHost (0, localIpAddr, localPort);
  if (newConnection->host == NULL)
    {
      connectionsPoolLock.lock ();
      connectionsPool.put (newConnection);
      connectionsPoolLock.unlock ();
      return 0;
    }

  protocol = getProtocol (newConnection->host->getProtocolName ());
  if (protocol)
    opts = protocol->getProtocolOptions ();

  if (opts & Protocol::SSL)
    doSSLhandshake = 1;

  if (opts & Protocol::FAST_CHECK)
    doFastCheck = 1;

  string msg ("new-connection");
  handlers = getHandlers (msg);
  if (handlers)
    {
      for (size_t i = 0; i < handlers->size (); i++)
        if ((*handlers)[i]->updateMulticast (this, msg, newConnection) == 1)
          {
            connectionsPoolLock.lock ();
            connectionsPool.put (newConnection);
            connectionsPoolLock.unlock ();
            return 0;
          }
    }

  /* Do the SSL handshake if required.  */
  if (doSSLhandshake)
    {
      int ret = 0;
      SSL_CTX* ctx = newConnection->host->getSSLContext ();
      SslSocket *sslSocket = new SslSocket (s);

      sslSocket->setSSLContext (ctx);
      ret = sslSocket->sslAccept ();

      if (ret < 0)
        {
          connectionsPoolLock.lock ();
          connectionsPool.put (newConnection);
          connectionsPoolLock.unlock ();
          delete sslSocket;
          return 0;
        }
      newConnection->socket = sslSocket;
    }
  else
    newConnection->socket = s;

  if (doFastCheck)
    {
      newConnection->setScheduled (1);
      newConnection->setForceControl (1);
      newConnection->socket->setNonBlocking (1);
      connectionsScheduler.addNewReadyConnection (newConnection);
    }
  else
    connectionsScheduler.addNewWaitingConnection (newConnection);

  /*
   * If defined maxConnections and the number of active connections
   * is bigger than it say to the protocol that will parse the connection
   * to remove it from the active connections list.
   */
  if (maxConnections
      && connectionsScheduler.getNumAliveConnections () > maxConnections)
    newConnection->setToRemove (Connection::REMOVE_OVERLOAD);

  connectionsScheduler.registerConnectionID (newConnection);

  return newConnection;
}

/*!
 * Delete a connection.
 */
int Server::deleteConnection (ConnectionPtr s)
{
  notifyDeleteConnection (s);
  connectionsScheduler.removeConnection (s);

  s->destroy ();

  connectionsPoolLock.lock ();
  connectionsPool.put (s);
  connectionsPoolLock.unlock ();

  return 0;
}

/*!
 * Get notified when a connection is closed.
 */
int Server::notifyDeleteConnection (ConnectionPtr s)
{
  string msg ("remove-connection");
  vector<Multicast<string, void*, int>*>* handlers;

  handlers = getHandlers (msg);

  if (handlers)
    for (size_t i = 0; i < handlers->size (); i++)
      (*handlers)[i]->updateMulticast (this, msg, s);

  return 0;
}

/*!
 * Get a connection to parse.
 */
ConnectionPtr Server::getConnection (int /*id*/)
{
  return connectionsScheduler.getConnection ();
}

/*!
 * Delete all the active connections.
 */
void Server::clearAllConnections ()
{
  list<ConnectionPtr> connections;
  list<ConnectionPtr>::iterator it;

  connectionsScheduler.getConnections (connections);

  try
    {
      for (it = connections.begin (); it != connections.end (); it++)
        deleteConnection (*it);
    }
  catch (...)
    {
      throw;
    };
}


/*!
 * Returns the full path of the binaries directory.
 * The directory where the file myserver (.exe) is.
 */
const char *Server::getPath ()
{
  return path ? path->c_str () : "";
}

/*!
 * Add a free thread.
 */
void Server::increaseFreeThread ()
{
  threadsMutex->lock ();
  freeThreads++;
  threadsMutex->unlock ();
}

/*!
 * Remove a free thread.
 */
void Server::decreaseFreeThread ()
{
  threadsMutex->lock ();
  freeThreads--;
  threadsMutex->unlock ();
}


/*!
 * Returns the name of the server (the name of the current PC).
 */
const char *Server::getServerName ()
{
  return serverName;
}

/*!
 * Gets the number of threads.
 */
u_long Server::getNumThreads ()
{
  u_long ret;
  threadsMutex->lock ();
  ret = threads.size ();
  threadsMutex->unlock ();
  return ret;
}

/*!
 * Returns a comma-separated local machine IPs list.
 * For example: 192.168.0.1, 61.62.63.64, 65.66.67.68.69
 */
const char *Server::getAddresses ()
{
  return ipAddresses ? ipAddresses->c_str () : "";
}

/*!
 * Clear the data dictionary.
 * Returns zero on success.
 */
int Server::freeHashedData ()
{
  try
    {
      list<NodeTree<string>*>::iterator it = hashedDataTrees.begin ();
      while (it != hashedDataTrees.end ())
        {
          delete *it;
          it++;
        }

      hashedDataTrees.clear ();
      hashedData.clear ();
    }
  catch (...)
    {
      return 1;
    }
  return 0;
}

/*!
 * Get the value for name in the hash dictionary.
 * If the value is not found then defValue is returned.
 */
const char *Server::getData (const char *name, const char *defValue)
{
  NodeTree<string> *s = hashedData.get (name);

  return s ? s->getValue ()->c_str () : defValue;
}

/*!
 * Get the specified protocol.
 */
Protocol* Server::getProtocol (const char *protocolName)
{
  string protocol (protocolName);

  return protocols.getProtocol (protocol);
}

/*!
 * If specified set the uid/gid for the process.
 */
void Server::setProcessPermissions ()
{
  /*
   * If the configuration specify a group id, change the current group for
   * the process.
   */
  if (gid.length ())
    {
      ostringstream out;

      if (Process::setAdditionalGroups (0, 0))
        log (MYSERVER_LOG_MSG_ERROR, _("Error setting additional groups"));

      if (Process::setgid (gid.c_str ()))
        log (MYSERVER_LOG_MSG_ERROR, _("Error setting gid"));

      autoRebootEnabled = false;
    }

  /*
   * If the configuration file provides a user identifier, change the
   * current user for the process. Disable the reboot when this feature
   * is used.
   */
  if (uid.length ())
    {
      ostringstream out;
      if (Process::setuid (uid.c_str ()))
        log (MYSERVER_LOG_MSG_ERROR, _("Error setting uid"));

      autoRebootEnabled = false;
    }
}

/*!
 * Reboot the server.
 * Returns non zero on errors.
 */
int Server::reboot ()
{
  int ret;
  string msg ("reboot-server");
  vector<Multicast<string, void*, int>*>* handlers;

  handlers = getHandlers (msg);

  if (handlers)
    {
      for (size_t i = 0; i < handlers->size (); i++)
        (*handlers)[i]->updateMulticast (this, msg, 0);
    }

  serverReady = false;
  /* Reset the toReboot flag.  */
  toReboot = false;

  rebooting = true;

  /* Do nothing if the reboot is disabled.  */
  if (!autoRebootEnabled)
    return 0;

  /* Do a beep if outputting to console.  */
  if (logLocation.find ("console://") != string::npos)
    {
      char beep[] = { static_cast<char>(0x7), '\0' };
      string beepStr (beep);
      logManager->log (this, "MAINLOG", logLocation, beepStr);
    }

  log (MYSERVER_LOG_MSG_INFO, _("Rebooting"));

  if (mustEndServer)
    return 0;

  mustEndServer = true;

  ret = terminate ();
  if (ret)
    return ret;

  mustEndServer = false;


  rebooting = false;

  ret = initialize () || postLoad ();

  if (ret)
    {
      log (MYSERVER_LOG_MSG_INFO, _("The server could not be restarted"));
      return ret;
    }

  serverReady = true;

  log (MYSERVER_LOG_MSG_INFO, _("Restarted"));

  return 0;
}

/*!
 * Returns if the server is ready.
 */
bool Server::isServerReady ()
{
  return serverReady;
}

/*!
 * Reboot the server on the next loop.
 */
void Server::delayedReboot ()
{
  serverReady = false;
  toReboot = true;
}

/*!
 * Return the factory object to create cached files.
 */
CachedFileFactory* Server::getCachedFiles ()
{
  return &cachedFiles;
}

/*!
 * Get a list with all the alive connections.
 */
void Server::getConnections (list<ConnectionPtr>& out)
{
  connectionsScheduler.getConnections (out);
}

/*!
 * Disable the autoreboot.
 */
void Server::disableAutoReboot ()
{
  autoRebootEnabled = false;
}

/*!
 * Enable the autoreboot
 */
void Server::enableAutoReboot ()
{
  autoRebootEnabled = true;
}

/*!
 * Return the ProtocolManager object.
 */
ProtocolsManager *Server::getProtocolsManager ()
{
  return &protocols;
}

/*!
 * Return nonzero if the autoreboot is enabled.
 */
bool Server::isAutorebootEnabled ()
{
  return autoRebootEnabled;
}

/*!
 * Create a new thread.
 */
int Server::addThread (bool staticThread)
{
  int ret;
  string msg ("new-thread");
  ClientsThread* newThread = 0;
  vector<Multicast<string, void*, int>*>* handlers;

  purgeThreadsThreshold = 1;

  if (isRebooting ())
    return -1;

  if (!staticThread)
    {
      bool restored = false;

      for (list<ClientsThread*>::iterator it = threads.begin (); it
             != threads.end (); it++)
        {
          ClientsThread* thread = *it;

          if (thread->isToDestroy ())
            {
              thread->setToDestroy (0);
              restored = true;
              break;
            }
        }
      if (restored)
        return 0;
    }

  newThread = new ClientsThread (this);

  handlers = getHandlers (msg);

  if (handlers)
    {
      for (size_t i = 0; i < handlers->size (); i++)
        {
          (*handlers)[i]->updateMulticast (this, msg, newThread);
        }
    }

  newThread->setStatic (staticThread);

  newThread->id = (u_long) (++currentThreadID);

  ret = newThread->run ();

  if (ret)
    {
      log (MYSERVER_LOG_MSG_ERROR, _("Error creating thread"));
      return -1;
    }

  threads.push_back (newThread);
  return 0;
}

/*!
 * Remove a thread.
 * Return zero if a thread was removed.
 */
int Server::removeThread (u_long ID)
{
  int ret = 1;
  string msg ("remove-thread");
  vector<Multicast<string, void*, int>*>* handlers;


  handlers = getHandlers (msg);

  threadsMutex->lock ();

  if (handlers)
    {
      for (size_t i = 0; i < handlers->size (); i++)
        {
          (*handlers)[i]->updateMulticast (this, msg, &ID);
        }
    }

  for (list<ClientsThread*>::iterator it = threads.begin (); it != threads.end (); it++)
    {
      if ((*it)->id == ID)
        {
          (*it)->stop ();
          ret = 0;
          threads.erase (it);
          break;
        }
    }
  threadsMutex->unlock ();
  return ret;

}

/*!
 * Create the class instance.  Call this before use
 * the Server class.  The instance is not created in
 * getInstance to have a faster inline function.
 */
void Server::createInstance ()
{
  if (instance == 0)
    instance = new Server ();
}

/*!
 * Get a pointer to a filters factory object.
 */
FiltersFactory* Server::getFiltersFactory ()
{
  return &filtersFactory;
}

/*!
 * Check how many threads are not working.
 */
int Server::countAvailableThreads ()
{
  int count = 0;

  threadsMutex->lock ();

  count = freeThreads;

  threadsMutex->unlock ();

  return count;
}

/*!
 * Write a message on a single LogStream specifying a formatting string.
 *
 * \see LogManager#log (void*, string, string, LoggingLeve, bool, va_list)
 * \return 0 on success, 1 on error.
 */
int Server::log (LoggingLevel level, const char *fmt, ...)
{
  int failure = 0;

  va_list argptr;

  va_start (argptr, fmt);
  if (!fmt)
    return 0;

  bool ts = (logLocation.find ("console://") == string::npos);
  failure = logManager->log (this, "MAINLOG", level, ts, true, fmt, argptr);

  va_end (argptr);

  return failure;
}

/*!
 * Write a string to the log file and terminate the line.
 */
int Server::log (char const* str, LoggingLevel level)
{
  return log (level, "%s", str);
}

/*!
 * Use a specified location as log.
 */
int
Server::setLogLocation (string location)
{
  list<string> filters;
  if (location.size ())
    {
      logManager->remove (this);
      logLocation.assign (location);
      return logManager->add (this, "MAINLOG", logLocation, filters, 0);
    }
  return !logManager->contains (this, "MAINLOG", logLocation);
}

/*!
 * Get the size for the first buffer.
 */
u_long Server::getBuffersize ()
{
  return buffersize;
}
