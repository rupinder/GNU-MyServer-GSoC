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

#include "stdafx.h"
#include <include/server/server.h>
#include <include/server/clients_thread.h>
#include <include/base/safetime/safetime.h>

/*! Include headers for built-in protocols.  */
#include <include/protocol/http/http.h>  /*Include the HTTP protocol.  */
#include <include/protocol/https/https.h> /*Include the HTTPS protocol.  */
#include <include/protocol/control/control_protocol.h> /*Include the control protocol.  */
#include <include/protocol/ftp/ftp.h>
#include <include/base/string/stringutils.h>
#include <include/base/socket/socket.h>
#include <include/filter/gzip/gzip.h>
#include <include/base/regex/myserver_regex.h>
#include <include/base/file/files_utility.h>
#include <include/base/ssl/ssl.h>
#include <include/base/socket/ssl_socket.h>

extern "C"
{
#ifdef WIN32
#include <direct.h>
#endif
#ifdef NOT_WIN
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

#endif
}
#include <sstream>

#ifdef NOT_WIN
#define LPINT int *
#define INVALID_SOCKET -1
#define WORD unsigned int
#define BYTE unsigned char
#define MAKEWORD(a, b) ((WORD) (((BYTE) (a)) | ((WORD) ((BYTE) (b))) << 8))
#endif

#if HAVE_IPV6
const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
#endif

/*!
 *At startup the server instance is null.
 */
Server* Server::instance = 0;

Server::Server() : connectionsScheduler (this),
                   listenThreads (&connectionsScheduler, this),
                   authMethodFactory (),
                   validatorFactory (),
                   securityManager (&validatorFactory, &authMethodFactory)
{
  toReboot = false;
  autoRebootEnabled = true;
  rebooting = false;
  maxConnections = 0;
  serverReady = false;
  throttlingRate = 0;
  uid = 0;
  gid = 0;
  serverAdmin = 0;
  mimeManager = 0;
  mimeConfigurationFile = 0;
  mainConfigurationFile = 0;
  vhostConfigurationFile = 0;
  languagesPath = 0;
  languageFile = 0;
  externalPath = 0;
  path = 0;
  ipAddresses = 0;
  vhostList = 0;
  purgeThreadsThreshold = 1;
  freeThreads = 0;
  logManager = new LogManager (&filtersFactory);
  initLogManager ();
}

void
Server::initLogManager ()
{
  if (!logLocation.size ())
    {
      logLocation.assign ("console://stdout");
    }
  if (!logManager->contains (this))
    {
      list<string> filters;
      logManager->add (this, "MAINLOG", logLocation, filters, 0);
    }
}

/*!
 *Reinitialize the configuration paths, setting them to the specified ones.
 *Returns false on error.
 */
bool Server::resetConfigurationPaths(string &mainConf, string &mimeConf, string &vhostConf, string &externPath, string &langPath)
{
  if (!mainConfigurationFile)
    mainConfigurationFile = new string(mainConf);
  else
    mainConfigurationFile->assign(mainConf);

  if (!mimeConfigurationFile)
    mimeConfigurationFile = new string(mimeConf);
  else
    mimeConfigurationFile->assign(mimeConf);

  if (!vhostConfigurationFile)
    vhostConfigurationFile = new string(vhostConf);
  else
    vhostConfigurationFile->assign(vhostConf);

  if (!externalPath)
    externalPath = new string(externPath);
  else
    externalPath->assign(externPath);

  if (!languagesPath)
    languagesPath = new string(langPath);
  else
    languagesPath->assign(langPath);

  return true;
}

/*!
 *Check that the configuration paths are not empty, otherwise fall back to
 *the default ones.
 *Returns nonzero on error.
 */
int Server::checkConfigurationPaths()
{
  if (mainConfigurationFile->length() == 0)
  {
    mainConfigurationFile->assign("myserver.xml");

    if (copyConfigurationFromDefault("myserver.xml"))
    {
      logWriteln("Error loading configuration file", MYSERVER_LOG_ERROR);
      return -1;
    }
  }

  if (mimeConfigurationFile->length() == 0)
  {
    mimeConfigurationFile->assign("MIMEtypes.xml");

    if (copyConfigurationFromDefault("MIMEtypes.xml"))
    {
      logWriteln(languageParser.getValue("ERR_LOADMIME"), MYSERVER_LOG_ERROR);
      return -1;
    }
  }

  if (vhostConfigurationFile->length() == 0)
  {
    vhostConfigurationFile->assign("virtualhosts.xml");

    if (copyConfigurationFromDefault("virtualhosts.xml") != 0)
    {
      logWriteln(languageParser.getValue("ERR_LOADVHOSTS"), MYSERVER_LOG_ERROR);
      return -1;
    }
  }

  return 0;
}


/*!
 *Copy a configuration file from the default one.
 *Return nonzero on errors.
 */
int Server::copyConfigurationFromDefault(const char *fileName)
{
  File inputF;
  File outputF;
  int ret;
  string sSource(fileName);

  sSource.append(".default");
  ret = inputF.openFile(sSource, File::MYSERVER_OPEN_READ
      | File::MYSERVER_OPEN_IFEXISTS);
  if (ret)
    return -1;

  ret = outputF.openFile(fileName, File::MYSERVER_OPEN_WRITE
      | File::MYSERVER_OPEN_ALWAYS);
  if (ret)
    return -1;

  FilesUtility::copyFile(inputF, outputF);

  inputF.close();
  outputF.close();

  return 0;
}


/*!
 *Load here all the libraries.
 */
int Server::loadLibraries()
{
  Process::initialize();

  XmlParser::startXML();
  myserver_safetime_init();

  gnutls_global_init ();

  /* Startup the socket library.  */
  logWriteln(languageParser.getValue("MSG_ISOCK") );

  if(startupSocketLib(MAKEWORD( 1, 1)) != 0)
  {
    logWriteln( languageParser.getValue ("MSG_SERVER_CONF"), MYSERVER_LOG_ERROR);
    return 1;
  }

  logWriteln(languageParser.getValue("MSG_SOCKSTART") );

  return 0;
}


/*!
 *Destroy the object.
 */
Server::~Server()
{
  if(languagesPath)
    delete languagesPath;
  languagesPath = 0;

  if(languageFile)
    delete languageFile;
  languageFile = 0;
  if(vhostList)
    delete vhostList;

  if(serverAdmin)
    delete serverAdmin;
  serverAdmin = 0;

  delete vhostConfigurationFile;
  vhostConfigurationFile = 0;

  delete mainConfigurationFile;
  mainConfigurationFile = 0;

  delete mimeConfigurationFile;
  mimeConfigurationFile = 0;

  if(externalPath)
    delete externalPath;
  externalPath = 0;

  if(path)
    delete path;
  path = 0;

  if(ipAddresses)
    delete ipAddresses;
  
  if (logManager)
    delete logManager;
}

/*!
 *Start the server.
 */
void Server::start(string &mainConf, string &mimeConf, string &vhostConf, string &externPath, string &langPath)
{
  int err = 0;
#ifdef WIN32
  DWORD eventsCount, cNumRead;
  INPUT_RECORD irInBuf[128];
#endif

  displayBoot();

  try
  {
    if(loadLibraries())
      return;

    /*
     *Setup the server configuration.
     */
    logWriteln("Initializing server configuration...");

    if (!resetConfigurationPaths(mainConf, mimeConf, vhostConf, externPath, langPath))
      return;

    err = checkConfigurationPaths();
    if (err)
      return;

    err = initialize();
    if(err)
      return;

    /* Initialize the SSL library.  */
    initializeSSL();

    logWriteln( languageParser.getValue("MSG_SERVER_CONF") );

    if(postLoad())
      return;

    setProcessPermissions();

    if(getGid())
    {
      ostringstream out;
      out << "gid: " << gid;
      logWriteln(out.str().c_str());
    }

    if(getUid())
    {
      ostringstream out;
      out << "uid: " << uid;
      logWriteln(out.str().c_str());
    }

    logWriteln(languageParser.getValue("MSG_READY"));

    if (logLocation.find ("console://") != string::npos)
      {
        logWriteln (languageParser.getValue ("MSG_BREAK"));
      }

    serverReady = true;

    /* Finally we can give control to the main loop.  */
    mainLoop();

  }
  catch(bad_alloc &ba)
  {
    ostringstream s;
    s << "Bad alloc: " << ba.what();
    logWriteln(s.str().c_str());
  }
  catch(exception &e)
  {
    ostringstream s;
    s << "Error: " << e.what();
    logWriteln(s.str().c_str());
  };
  this->terminate();
  finalCleanup();

#ifdef WIN32
  WSACleanup();
#endif// WIN32
}

/*!
 *Complete the loading phase.
 */
int Server::postLoad()
{
  ostringstream nCPU;
  string strCPU;
  string buffer;

  /*
   *Get the name of the local machine.
   */
  memset(serverName, 0, HOST_NAME_MAX+1);
  Socket::gethostname(serverName, HOST_NAME_MAX);
  
  buffer.assign(languageParser.getValue("MSG_GETNAME"));
  buffer.append(" ");
  buffer.append(serverName);
  logWriteln(buffer.c_str());
  
  /*
   *Find the IP addresses of the local machine.
   */
  if(ipAddresses)
    delete ipAddresses;
  ipAddresses = new string();
  buffer.assign("Host: ");
  buffer.append(serverName);
  logWriteln(buffer.c_str() );
  
  if(Socket::getLocalIPsList(*ipAddresses))
  {
    string msg;
    msg.assign(languageParser.getValue("ERR_ERROR"));
    msg.append(" : Reading IP list");
    logWriteln(msg.c_str(), MYSERVER_LOG_ERROR);
    return -1;
  }
  else
  {
    string msg;
    msg.assign("IP: ");
    msg.append(*ipAddresses);
    logWriteln(msg.c_str());
  }

  /* Load the MIME types.  */
  logWriteln(languageParser.getValue("MSG_LOADMIME"));

  if(mimeManager)
    delete mimeManager;

  mimeManager = new MimeManager();
  
  if(int nMIMEtypes = mimeManager->loadXML(mimeConfigurationFile->c_str()))
  {
    ostringstream stream;
    stream << languageParser.getValue("MSG_MIMERUN") << ": " << nMIMEtypes;
    logWriteln(stream.str().c_str());
  }
  else
  {
    logWriteln(languageParser.getValue("ERR_LOADMIME"), MYSERVER_LOG_ERROR);
  }

  nCPU << (u_int)getCPUCount();
  
  strCPU.assign(languageParser.getValue("MSG_NUM_CPU"));
  strCPU.append(" ");
  strCPU.append(nCPU.str());
  logWriteln(strCPU.c_str());
  
  connectionsScheduler.restart();
  
  listenThreads.initialize(&languageParser);
  
  if(vhostList)
    delete vhostList;
  
  vhostList = new VhostManager(&listenThreads, logManager);
  
  if(vhostList == NULL)
    return -1;

  getProcessServerManager()->load();
    
  /* Load the home directories configuration.  */
  homeDir.load();

  loadPlugins();

  /* Load the virtual hosts configuration from the xml file.  */
  vhostList->loadXMLConfigurationFile(vhostConfigurationFile->c_str());

  if(path == 0)
    path = new string();
  
  if(getdefaultwd(*path))
    return -1;

  for(u_long i = 0; i < nStaticThreads; i++)
  {
    logWriteln(languageParser.getValue("MSG_CREATET"));
    if(addThread(true))
      return -1;

    logWriteln(languageParser.getValue("MSG_THREADR"));
  }

  return 0;
}

/*!
 *Load the plugins.
 */
void Server::loadPlugins()
{
  string xml("xml"); 
  //FIXME: xmlV is never freed.
  XmlValidator *xmlV = new XmlValidator ();
    
  validatorFactory.addValidator (xml, xmlV);
  authMethodFactory.addAuthMethod (xml, (AuthMethod*) xmlV);

  if(filtersFactory.insert("gzip", Gzip::factory))
  {
    ostringstream stream;
    stream <<  languageParser.getValue("ERR_ERROR") << ": Gzip Filter";
    logWriteln(stream.str().c_str(), MYSERVER_LOG_ERROR);
  }

  Protocol *protocolsSet[] = {new HttpProtocol(),
                              new HttpsProtocol(),
                              new FtpProtocol(),
                              new ControlProtocol(),
                              0};

  for (int j = 0; protocolsSet[j]; j++)
  {
    char protocolName[32];
    Protocol *protocol = protocolsSet[j];
    protocol->loadProtocol(&languageParser);
    protocol->registerName(protocolName, 32);
    getProtocolsManager()->addProtocol(protocolName, protocol);
  }


  //getPluginsManager()->addNamespace(&executors);
  //getPluginsManager()->addNamespace(&protocols);
  //getPluginsManager()->addNamespace(&filters);
  //getPluginsManager()->addNamespace(&genericPluginsManager);

  getPluginsManager()->preLoad(this, &languageParser, *externalPath);
  getPluginsManager()->load(this, &languageParser, *externalPath);
  getPluginsManager()->postLoad(this, &languageParser);
}

/*!
 *Server main loop.
 */
void Server::mainLoop()
{
  time_t mainConfTime;
  time_t hostsConfTime;
  time_t mimeConfTime;

  u_long purgeThreadsCounter = 0;

  mainConfTime =
    FilesUtility::getLastModTime(mainConfigurationFile->c_str());
  hostsConfTime =
    FilesUtility::getLastModTime(vhostConfigurationFile->c_str());
  mimeConfTime =
    FilesUtility::getLastModTime(mimeConfigurationFile->c_str());

  /*
   *Keep thread alive.
   *When the endServer flag is set to True exit
   *from the loop and terminate the server execution.
   */
  while(!endServer)
  {
    Thread::wait(1000000);

    /* Check threads.  */
    if(purgeThreadsCounter++ >= 10)
    {
      purgeThreadsCounter = 0;
      purgeThreads();
    }

    if(autoRebootEnabled)
    {
      time_t mainConfTimeNow =
        FilesUtility::getLastModTime(mainConfigurationFile->c_str());
      time_t hostsConfTimeNow =
        FilesUtility::getLastModTime(vhostConfigurationFile->c_str());
      time_t mimeConfNow =
        FilesUtility::getLastModTime(mimeConfigurationFile->c_str());
      
      /* If a configuration file was modified reboot the server. */
      if(((mainConfTimeNow != -1) && (hostsConfTimeNow != -1)  &&
          (mimeConfNow != -1)) || toReboot)
      {
        if( (mainConfTimeNow  != mainConfTime) || toReboot)
        {
          string msg("main-conf-changed");
          notifyMulticast(msg, 0);

          reboot();
          /* Store new mtime values.  */
          mainConfTime = mainConfTimeNow;
          mimeConfTime = mimeConfNow;
        }
        else if(mimeConfNow != mimeConfTime)
        {
          string msg("mime-conf-changed");
          notifyMulticast(msg, 0);

          if (logLocation.find ("console://") != string::npos)
            {
              char beep[] = { static_cast<char>(0x7), '\0' };
              logManager->log (this, "MAINLOG", logLocation, string (beep));
            }
          logWriteln("Reloading MIMEtypes.xml");
          
          getMimeManager()->loadXML(getMIMEConfFile());

          logWriteln("Reloaded");

          mimeConfTime = mimeConfNow;
        }
        else if(hostsConfTimeNow != hostsConfTime)
        {
          VhostManager* oldvhost = vhostList;
          string msg("vhosts-conf-changed");
          notifyMulticast(msg, 0);

          /* Do a beep if outputting to console.  */
          if (logLocation.find ("console://") != string::npos)
            {
              char beep[] = { static_cast<char>(0x7), '\0' };
              logManager->log (this, "MAINLOG", logLocation, string (beep));
            }

          logWriteln("Rebooting...");

          Socket::stopBlockingOperations(true);

          connectionsScheduler.release();

          listenThreads.beginFastReboot();

          listenThreads.terminate();

          clearAllConnections();

          Socket::stopBlockingOperations(false);

          connectionsScheduler.restart();
          listenThreads.initialize(&languageParser);
            
          vhostList = new VhostManager(&listenThreads, logManager);

          if(vhostList == 0)
            continue;

          delete oldvhost;

            /* Load the virtual hosts configuration from the xml file.  */
          if(vhostList->loadXMLConfigurationFile(vhostConfigurationFile->c_str()))
          {
            listenThreads.rollbackFastReboot();
          }
          else
          {
            listenThreads.commitFastReboot();
          }

          hostsConfTime = hostsConfTimeNow;
          logWriteln("Reloaded");
        }
      }
    }//end  if(autoRebootEnabled)

  }
}

void Server::logWriteNTimes(string str, unsigned n)
{
  while (n--)
    logManager->log (this, "MAINLOG", logLocation, str);
  logManager->log(this, "MAINLOG", logLocation, string (""), true);
}

/*!
 *Display the MyServer boot.
 */
void Server::displayBoot()
{

#ifdef CLEAR_BOOT_SCREEN

  if (logLocation.find ("console://") != string::npos)
  {
#ifdef WIN32

    /*
     *Under the windows platform use the cls operating-system
     *command to clear the screen.
     */
    _flushall();
    system("cls");
#endif
#ifdef NOT_WIN

  /*
   *Under an UNIX environment, clearing the screen
   *can be done in a similar method
   */
    sync();
    system("clear");
#endif
  }

#endif /* CLEAR_BOOT_SCREEN.  */

  /*
   *Print the MyServer signature only if the log writes to the console.
   */
  if(logLocation.find ("console://") != string::npos)
  {
    try
    {
      size_t length;
      string softwareSignature;
      softwareSignature.assign("************ GNU MyServer ");
      softwareSignature.append(MYSERVER_VERSION);
      softwareSignature.append("************");
      length = softwareSignature.length();

      logWriteNTimes("*", length);
      logManager->log (this, "MAINLOG", logLocation, softwareSignature, true);
      logWriteNTimes("*", length);
    }
    catch(exception& e)
    {
      ostringstream err;
      err << "Error: " << e.what();
      logManager->log (this, "MAINLOG", logLocation, err.str (), true);
      return;
    }
    catch(...)
    {
      return;
    };
  }

}

/*!
 *Removed threads that can be destroyed.
 *The function returns the number of threads that were destroyed.
 */
int Server::purgeThreads()
{
  u_long ticks = getTicks();
  u_long destroyed = 0;

  purgeThreadsThreshold = std::min(purgeThreadsThreshold << 1, nMaxThreads);
  threadsMutex->lock();
  for(list<ClientsThread*>::iterator it = threads.begin(); it != threads.end();)
  {
    ClientsThread* thread = *it;


    /*
     *Shutdown all threads that can be destroyed.
     */
    if(thread->isToDestroy())
    {
      if(destroyed < purgeThreadsThreshold)
      {
        list<ClientsThread*>::iterator next = it;
        next++;

        thread->stop();
        threads.erase(it);
        destroyed++;

        it = next;
      }
      else
        it++;
    }
    else
    {

      if(!thread->isStatic())
        if(ticks - thread->getTimeout() > MYSERVER_SEC(15))
          thread->setToDestroy(1);
      it++;
    }
  }
  threadsMutex->unlock();

  return threads.size();
}

/*!
 *Do the final cleanup.  Called once when the process is terminated.
 */
void Server::finalCleanup()
{
  XmlParser::cleanXML();
  freecwdBuffer();
  myserver_safetime_destroy();
}

/*!
 *Return the user identifier to use for the process.
 */
u_long Server::getUid()
{
  return uid;
}

/*!
 *Return the group identifier to use for the process.
 */
u_long Server::getGid()
{
  return gid;
}

/*!
 *Get a pointer to the language parser.
 */
XmlParser* Server::getLanguageParser()
{
  return &languageParser;
}

/*!
 *Returns the numbers of active connections the list.
 */
u_long Server::getNumConnections()
{
  return connectionsScheduler.getConnectionsNumber();
}

/*!
 *Returns the numbers of all the connections to the server.
 */
u_long Server::getNumTotalConnections()
{
  return connectionsScheduler.getNumTotalConnections();
}

/*!
 *Get the verbosity value.
 */
u_long Server::getVerbosity()
{
  return verbosity;
}

/*!
 *Set the verbosity value.
 */
void  Server::setVerbosity(u_long nv)
{
  verbosity=nv;
}

/*!
 *Return a home directory object.
 */
HomeDir* Server::getHomeDir()
{
  return &homeDir;
}

/*!
 *Stop the execution of the server.
 */
void Server::stop()
{
  endServer = true;
}

/*!
 *Unload the server.
 *Return nonzero on errors.
 */
int Server::terminate()
{
  list<ThreadID> threadsIds;

  if(verbosity > 1)
    logWriteln(languageParser.getValue("MSG_STOPT"));

  listenThreads.terminate();

  threadsMutex->lock();

  for(list<ClientsThread*>::iterator it = threads.begin(); it != threads.end(); it++)
  {
    threadsIds.push_back((*it)->getThreadId());
    (*it)->stop();
  }

  threadsMutex->unlock();
  Socket::stopBlockingOperations(true);

  connectionsScheduler.release();

  for(list<ThreadID>::iterator it = threadsIds.begin(); it != threadsIds.end(); it++)
  {
    Thread::join(*it);
  }

  clearAllConnections();

  /* Clear the home directories data.  */
  homeDir.clear();

  if(verbosity > 1)
    logWriteln(languageParser.getValue("MSG_TSTOPPED"));


  if(verbosity > 1)
  {
    logWriteln(languageParser.getValue("MSG_MEMCLEAN"));
  }

  freeHashedData();

  /* Restore the blocking status in case of a reboot.  */
  Socket::stopBlockingOperations(false);

  ipAddresses = 0;
  vhostList = 0;
  languageParser.close();

  if(mimeManager)
  {
    mimeManager->clean();
    delete mimeManager;
    mimeManager = 0;
  }

#ifdef WIN32
  /*
   *Under WIN32 cleanup environment strings.
   */
  FreeEnvironmentStrings((LPTSTR)envString);
#endif

  getProcessServerManager()->clear();

  filtersFactory.free();

  configurationFileManager.close();

  getPluginsManager()->unLoad(this, &languageParser);

  delete connectionsMutex;

  clearMulticastRegistry();

  globalData.clear();

  /*
   *Free all the threads.
   */
  threadsMutex->lock();
  threads.clear();
  threadsMutex->unlock();
  delete threadsMutex;

  nStaticThreads = 0;
  if(verbosity > 1)
  {
    logWriteln("MyServer is stopped");
  }
  
  logManager->clear ();
  
  return 0;
}

/*!
 *Get the server administrator e-mail address.
 *To change this use the main configuration file.
 */
const char *Server::getServerAdmin()
{
  return serverAdmin ? serverAdmin->c_str() : "";
}

/*!
 *Here is loaded the configuration of the server.
 *The configuration file is a XML file.
 *Return nonzero on errors.
 */
int Server::initialize()
{
  initLogManager ();
  char *data;
#ifdef WIN32
  envString = GetEnvironmentStrings();
#endif
  connectionsMutex = new Mutex();

  threadsMutex = new Mutex();

  /* Store the default values.  */
  nStaticThreads = 20;
  nMaxThreads = 50;
  currentThreadID = 0;
  freeThreads = 0;
  connectionTimeout = MYSERVER_SEC(25);
  endServer = false;
  verbosity = 1;
  purgeThreadsThreshold = 1;
  throttlingRate = 0;
  maxConnections = 0;
  maxConnectionsToAccept = 0;
  if(serverAdmin)
    delete serverAdmin;
  serverAdmin = 0;

  if(configurationFileManager.open(mainConfigurationFile->c_str()))
    return -1;

  data = configurationFileManager.getValue("LANGUAGE");
  if(languageFile)
    delete languageFile;
  languageFile = new string();
  if(data)
  {
    languageFile->assign(*languagesPath);
    languageFile->append("/");
    languageFile->append(data);
  }
  else
  {
    languageFile->assign("languages/english.xml");
  }

  if(languageParser.open(languageFile->c_str()))
  {
    string err;
    err.assign("Error loading: ");
    logWriteln(err.c_str(), MYSERVER_LOG_ERROR);
    return -1;
  }
  logWriteln(languageParser.getValue("MSG_LANGUAGE"));

  data = configurationFileManager.getValue("VERBOSITY");
  if(data)
  {
    verbosity = (u_long)atoi(data);
  }


  data = configurationFileManager.getValue("BUFFER_SIZE");
  if(data)
  {
    buffersize=secondaryBufferSize= (atol(data) > 81920) ?  atol(data) :  81920 ;
  }
  data = configurationFileManager.getValue("CONNECTION_TIMEOUT");
  if(data)
  {
    connectionTimeout = MYSERVER_SEC((u_long)atol(data));
  }

  data = configurationFileManager.getValue("NTHREADS_STATIC");
  if(data)
  {
    nStaticThreads = atoi(data);
  }
  data = configurationFileManager.getValue("NTHREADS_MAX");
  if(data)
  {
    nMaxThreads = atoi(data);
  }

  /* Get the max connections number to allow.  */
  data = configurationFileManager.getValue("MAX_CONNECTIONS");
  if(data)
  {
    maxConnections = atoi(data);
  }

  /* Get the max connections number to accept.  */
  data = configurationFileManager.getValue("MAX_CONNECTIONS_TO_ACCEPT");
  if(data)
  {
    maxConnectionsToAccept = atoi(data);
  }

  /* Get the default throttling rate to use on connections.  */
  data = configurationFileManager.getValue("THROTTLING_RATE");
  if(data)
  {
    throttlingRate = (u_long)atoi(data);
  }

  /* Load the server administrator e-mail.  */
  data = configurationFileManager.getValue("SERVER_ADMIN");
  if(data)
  {
    if(serverAdmin == 0)
      serverAdmin = new string();
    serverAdmin->assign(data);
  }

  data = configurationFileManager.getValue("CONNECTION_TIMEOUT");
  if(data)
  {
    connectionTimeout=MYSERVER_SEC((u_long)atol(data));
  }

  data = configurationFileManager.getValue("MAX_LOG_FILE_SIZE");
  if(data)
  {
    maxLogFileSize=(u_long)atol(data);
  }

  data = configurationFileManager.getValue("MAX_FILESCACHE_SIZE");
  if(data)
  {
    u_long maxSize = (u_long)atol(data);
    cachedFiles.initialize(maxSize);
  }
  else
    cachedFiles.initialize(1 << 23);

  data = configurationFileManager.getValue("TEMP_DIRECTORY");
  if (data)
  {
    string tmpPath (data);
    FilesUtility::completePath (tmpPath);
    FilesUtility::setTmpPath (tmpPath);
  }
  else
    FilesUtility::resetTmpPath ();

  data = configurationFileManager.getValue("MAX_FILESCACHE_FILESIZE");
  if(data)
  {
    u_long maxSize = (u_long)atol(data);
    cachedFiles.setMaxSize(maxSize);
  }

  data = configurationFileManager.getValue("MIN_FILESCACHE_FILESIZE");
  if(data)
  {
    u_long minSize = (u_long)atol(data);
    cachedFiles.setMinSize(minSize);
  }

  data = configurationFileManager.getValue("PROCESS_USER_ID");
  if(data)
  {
    uid = atoi(data);
  }
  data = configurationFileManager.getValue("PROCESS_GROUP_ID");
  if(data)
  {
    gid = atoi(data);
  }

  data = configurationFileManager.getValue("MAX_SERVERS_PROCESSES");
  if(data)
  {
    int maxServersProcesses = atoi(data);
    getProcessServerManager()->setMaxServers(maxServersProcesses);
  }

  data = configurationFileManager.getValue("SERVERS_PROCESSES_INITIAL_PORT");
  if(data)
  {
    int serversProcessesInitialPort = atoi(data);
    getProcessServerManager()->setInitialPort(serversProcessesInitialPort);
  }

  {
    xmlNodePtr node =
      xmlDocGetRootElement(configurationFileManager.getDoc())->xmlChildrenNode;
    for(;node; node = node->next)
    {
      if(node->children && node->children->content)
      {
        string* old;
        string *value = new string((const char*)node->children->content);
        string key((const char*)node->name);
        if(value == 0)
          return -1;
        old = hashedData.put(key, value);
        if(old)
        {
          delete old;
        }
      }
    }
  }

  return 0;
}


/*!
 *Check if there are free threads to handle a new request.  If there
 *are not enough threads create a new one.
 */
void Server::checkThreadsNumber()
{
  threadsMutex->lock();

  /*
   *Create a new thread if there are not available threads and
   *we did not reach the limit.
   */
  if((threads.size() < nMaxThreads) && (freeThreads < 1))
    addThread(false);

  threadsMutex->unlock();
}

/*!
 *Get the default throttling rate to use with connections to the server.
 */
u_long Server::getThrottlingRate()
{
  return throttlingRate;
}

/*!
 *This function returns the max size of the logs file as defined in the
 *configuration file.
 */
int Server::getMaxLogFileSize()
{
  return maxLogFileSize;
}

/*!
 *Returns the connection timeout.
 */
u_long Server::getTimeout()
{
  return connectionTimeout;
}

/*!
 *This function add a new connection to the list.
 */
int Server::addConnection(Socket s, MYSERVER_SOCKADDRIN *asockIn)
{

  int ret = 0;
  char ip[MAX_IP_STRING_LEN];
  char localIp[MAX_IP_STRING_LEN];
  u_short port;
  u_short myPort;
  MYSERVER_SOCKADDRIN  localSockIn = { 0 };
  int dim;

  /*
   *We can use MAX_IP_STRING_LEN only because we use NI_NUMERICHOST
   *in getnameinfo call; Otherwise we should have used NI_MAXHOST.
   *ip is the string containing the address of the remote host connecting
   *to the server.
   *localIp is the local address used by the connection.
   *port is the remote port used by the client to open the connection.
   *myPort is the port used by the server to listen.
   */
  if ( asockIn == NULL ||
       (asockIn->ss_family != AF_INET && asockIn->ss_family != AF_INET6))
    return 0;

  memset(ip, 0, MAX_IP_STRING_LEN);
  memset(localIp, 0, MAX_IP_STRING_LEN);

  if( s.getHandle() == 0 )
    return 0;

  /*
   *Do not accept this connection if a MAX_CONNECTIONS_TO_ACCEPT limit is
   *defined.
   */
  if(maxConnectionsToAccept &&
     ((u_long)connectionsScheduler.getConnectionsNumber() >= maxConnectionsToAccept))
    return 0;

#if ( HAVE_IPV6 )
  if ( asockIn->ss_family == AF_INET )
    ret = getnameinfo(reinterpret_cast<const sockaddr *>(asockIn),
                      sizeof(sockaddr_in),
                      ip, MAX_IP_STRING_LEN, NULL, 0, NI_NUMERICHOST);
  else
    ret = getnameinfo(reinterpret_cast<const sockaddr *>(asockIn),
                      sizeof(sockaddr_in6),  ip, MAX_IP_STRING_LEN,
                      NULL, 0, NI_NUMERICHOST);
  if(ret)
     return 0;

  if ( asockIn->ss_family == AF_INET )
    dim = sizeof(sockaddr_in);
  else
    dim = sizeof(sockaddr_in6);
  s.getsockname((MYSERVER_SOCKADDR*)&localSockIn, &dim);

  if ( asockIn->ss_family == AF_INET )
    ret = getnameinfo(reinterpret_cast<const sockaddr *>(&localSockIn),
                      sizeof(sockaddr_in), localIp, MAX_IP_STRING_LEN,
                      NULL, 0, NI_NUMERICHOST);
  else// AF_INET6
    ret = getnameinfo(reinterpret_cast<const sockaddr *>(&localSockIn),
                      sizeof(sockaddr_in6), localIp, MAX_IP_STRING_LEN,
                      NULL, 0, NI_NUMERICHOST);
  if(ret)
     return 0;
#else// !HAVE_IPV6
  dim = sizeof(localSockIn);
  s.getsockname((MYSERVER_SOCKADDR*)&localSockIn, &dim);
  strncpy(ip,  inet_ntoa(((sockaddr_in *)asockIn)->sin_addr),
          MAX_IP_STRING_LEN);
  strncpy(localIp,  inet_ntoa(((sockaddr_in *)&localSockIn)->sin_addr),
          MAX_IP_STRING_LEN);
#endif//HAVE_IPV6

  /* Port used by the client.  */
    if ( asockIn->ss_family == AF_INET )
      port = ntohs(((sockaddr_in *)(asockIn))->sin_port);
#if HAVE_IPV6
  else
    port = ntohs(((sockaddr_in6 *)(asockIn))->sin6_port);
#endif

  /* Port used by the server. */
  if ( localSockIn.ss_family == AF_INET )
      myPort = ntohs(((sockaddr_in *)(&localSockIn))->sin_port);
#if HAVE_IPV6
  else
    myPort = ntohs(((sockaddr_in6 *)(&localSockIn))->sin6_port);
#endif

  if(!addConnectionToList(&s, asockIn, &ip[0], &localIp[0], port, myPort, 1))
  {
    /* If we report error to add the connection to the thread.  */
    ret = 0;

    /* Shutdown the socket both on receive that on send.  */
    s.shutdown(2);

    /* Then close it.  */
    s.close();
  }
  return ret;
}

/*!
 *Return the max number of threads that the server can start.
 */
int Server::getMaxThreads()
{
  return nMaxThreads;
}

/*!
 *Add a new connection.
 *A connection is defined using a connection struct.
 */
ConnectionPtr Server::addConnectionToList(Socket* s,
                                          MYSERVER_SOCKADDRIN* /*asockIn*/,
                                          char *ipAddr, char *localIpAddr,
                                          u_short port, u_short localPort,
                                          int /*id*/)
{
  int doSSLhandshake = 0;
  int doFastCheck = 0;
  Protocol* protocol;
  int opts = 0;
  ConnectionPtr newConnection = new Connection();
  vector<Multicast<string, void*, int>*>* handlers;

  if(!newConnection)
  {
    return NULL;
  }

  newConnection->setPort(port);
  newConnection->setTimeout( getTicks() );
  newConnection->setLocalPort(localPort);
  newConnection->setIpAddr(ipAddr);
  newConnection->setLocalIpAddr(localIpAddr);
  newConnection->host = vhostList->getVHost(0,
                                            localIpAddr,
                                            localPort);

  /* No vhost for the connection so bail.  */
  if(newConnection->host == 0)
  {
    delete newConnection;
    return 0;
  }

  protocol = getProtocol(newConnection->host->getProtocolName());

  if(protocol)
    opts = protocol->getProtocolOptions();

  if(opts & PROTOCOL_USES_SSL)
    doSSLhandshake = 1;

  if(opts & PROTOCOL_FAST_CHECK)
    doFastCheck = 1;


  {
    string msg("new-connection");

    handlers = getHandlers(msg);

    if(handlers)
    {
      for(size_t i = 0; i < handlers->size(); i++)
        if((*handlers)[i]->updateMulticast(this, msg, newConnection) == 1)
        {
          delete newConnection;
          return 0;
        }
    }
  }

  /* Do the SSL handshake if required.  */
  if(doSSLhandshake)
  {
    int ret = 0;
    SSL_CTX* ctx = newConnection->host->getSSLContext();
    SslSocket *sslSocket = new SslSocket(s);

    sslSocket->setSSLContext(ctx);
    ret = sslSocket->sslAccept();

    if(ret < 0)
    {
      /* Free the connection on errors. */
      delete newConnection;
      delete sslSocket;
      return 0;
    }
    newConnection->socket = sslSocket;
  }
  else
  {
    newConnection->socket = new Socket(s);
  }

  if ( doFastCheck )
  {
    newConnection->setScheduled(1);
    newConnection->setForceControl(1);
    connectionsScheduler.addNewReadyConnection(newConnection);
  }
  else
    connectionsScheduler.addNewWaitingConnection(newConnection);

  /*
   *If defined maxConnections and the number of active connections
   *is bigger than it say to the protocol that will parse the connection
   *to remove it from the active connections list.
   */
  if(maxConnections &&
     ((u_long)connectionsScheduler.getConnectionsNumber() > maxConnections))
    newConnection->setToRemove(CONNECTION_REMOVE_OVERLOAD);

  connectionsScheduler.registerConnectionID(newConnection);


  /*
   *Signal the new connection to the waiting threads.
   */
  return newConnection;
}



/*!
 *Delete a connection.
 */
int Server::deleteConnection(ConnectionPtr s)
{
  notifyDeleteConnection(s);
  connectionsScheduler.removeConnection (s);

  return 0;
}

/*!
 *Get notified when a connection is closed.
 */
int Server::notifyDeleteConnection(ConnectionPtr s)
{
  string msg("remove-connection");
  vector<Multicast<string, void*, int>*>* handlers;

  handlers = getHandlers(msg);

  if(handlers)
  {
    for(size_t i = 0; i < handlers->size(); i++)
    {
      (*handlers)[i]->updateMulticast(this, msg, s);
    }
  }
  return 0;
}

/*!
 *Get a connection to parse.
 */
ConnectionPtr Server::getConnection(int /*id*/)
{
  return connectionsScheduler.getConnection();
}

/*!
 *Delete all the active connections.
 */
void Server::clearAllConnections()
{
  list<ConnectionPtr> connections;
  list<ConnectionPtr>::iterator it;

  connectionsScheduler.getConnections(connections);

  try
  {
    for(it = connections.begin(); it != connections.end(); it++)
    {
      deleteConnection(*it);
    }
  }
  catch(...)
  {
    throw;
  };
}


/*!
 *Returns the full path of the binaries directory.
 *The directory where the file myserver(.exe) is.
 */
const char *Server::getPath()
{
  return path ? path->c_str() : "";
}

/*!
 *Add a free thread.
 */
void Server::increaseFreeThread()
{
  threadsMutex->lock();
  freeThreads++;
  threadsMutex->unlock();
}

/*!
 *Remove a free thread.
 */
void Server::decreaseFreeThread()
{
  threadsMutex->lock();
  freeThreads--;
  threadsMutex->unlock();
}


/*!
 *Returns the name of the server(the name of the current PC).
 */
const char *Server::getServerName()
{
  return serverName;
}

/*!
 *Gets the number of threads.
 */
u_long Server::getNumThreads()
{
  u_long ret;
  threadsMutex->lock();
  ret = threads.size();
  threadsMutex->unlock();
  return ret;
}

/*!
 *Returns a comma-separated local machine IPs list.
 *For example: 192.168.0.1, 61.62.63.64, 65.66.67.68.69
 */
const char *Server::getAddresses()
{
  return ipAddresses ? ipAddresses->c_str() : "";
}

/*!
 *Clear the data dictionary.
 *Returns zero on success.
 */
int Server::freeHashedData()
{
  try
  {
    HashMap<string, string*>::Iterator it = hashedData.begin();
    for (;it != hashedData.end(); it++)
      delete *it;
    hashedData.clear();
  }
  catch(...)
  {
    return 1;
  }
  return 0;
}

/*!
 *Get the value for name in the hash dictionary.
 */
const char* Server::getHashedData(const char* name)
{
  string *s = hashedData.get(name);
  return s ? s->c_str() : 0;
}

/*!
 *Get the specified protocol.
 */
Protocol* Server::getProtocol(const char *protocolName)
{
  string protocol(protocolName);

  return protocols.getProtocol(protocol);
}

/*!
 *Get where external protocols are.
 */
const char* Server::getExternalPath()
{
  return externalPath ? externalPath->c_str() : "";
}


/*!
 *If specified set the uid/gid for the process.
 */
void Server::setProcessPermissions()
{
    /*
     *If the configuration specify a group id, change the current group for
     *the process.
     */
    if(gid)
     {
       ostringstream out;

       if(Process::setAdditionalGroups(0, 0))
       {
         out << languageParser.getValue("ERR_ERROR")
             << ": setAdditionalGroups";
         logWriteln(out.str().c_str(), MYSERVER_LOG_ERROR);
       }

       if(Process::setgid(gid))
       {
         out << languageParser.getValue("ERR_ERROR") << ": setgid " << gid;
         logWriteln(out.str().c_str(), MYSERVER_LOG_ERROR);
       }
       autoRebootEnabled = false;
     }

    /*
     *If the configuration file provides a user identifier, change the
     *current user for the process. Disable the reboot when this feature
     *is used.
     */
    if(uid)
    {
      ostringstream out;
      if(Process::setuid(uid))
      {
        out << languageParser.getValue("ERR_ERROR") << ": setuid " << uid;
        logWriteln(out.str().c_str(), MYSERVER_LOG_ERROR);
      }
      autoRebootEnabled = false;
    }
}

/*!
 *Reboot the server.
 *Returns non zero on errors.
 */
int Server::reboot()
{
  int ret;
  string msg("reboot-server");
  vector<Multicast<string, void*, int>*>* handlers;

  handlers = getHandlers(msg);

  if(handlers)
  {
    for(size_t i = 0; i < handlers->size(); i++)
      (*handlers)[i]->updateMulticast(this, msg, 0);
  }

  serverReady = false;
  /* Reset the toReboot flag.  */
  toReboot = false;

  rebooting = true;

  /* Do nothing if the reboot is disabled.  */
  if(!autoRebootEnabled)
    return 0;

  /* Do a beep if outputting to console.  */
  if (logLocation.find ("console://") != string::npos)
  {
    char beep[] = { static_cast<char>(0x7), '\0' };
    logManager->log (this, "MAINLOG", logLocation, string (beep));
  }

  logWriteln("Rebooting...");
  if(mustEndServer)
    return 0;

  mustEndServer = true;

  ret = terminate();
  if(ret)
    return ret;

  mustEndServer = false;


  rebooting = false;

  ret = initialize() || postLoad();

  if(ret)
    return ret;

  serverReady = true;

  logWriteln("Rebooted");

  return 0;
}

/*!
 *Returns if the server is ready.
 */
bool Server::isServerReady()
{
  return serverReady;
}

/*!
 *Reboot the server on the next loop.
 */
void Server::rebootOnNextLoop()
{
  serverReady = false;
  toReboot = true;
}


/*!
 *Return the factory object to create cached files.
 */
CachedFileFactory* Server::getCachedFiles()
{
  return &cachedFiles;
}

/*!
 *Return the path to the mail configuration file.
 */
const char *Server::getMainConfFile()
{
  return mainConfigurationFile
    ? mainConfigurationFile->c_str() : 0;
}

/*!
 *Return the path to the mail configuration file.
 */
const char *Server::getVhostConfFile()
{
  return vhostConfigurationFile ? vhostConfigurationFile->c_str() : 0;
}

/*!
 *Return the path to the mail configuration file.
 */
const char *Server::getMIMEConfFile()
{
  return mimeConfigurationFile ? mimeConfigurationFile->c_str() : "";
}

/*!
 *Get a list with all the alive connections.
 */
void Server::getConnections(list<ConnectionPtr>& out)
{
  connectionsScheduler.getConnections(out);
}

/*!
 *Disable the autoreboot.
 */
void Server::disableAutoReboot()
{
  autoRebootEnabled = false;
}

/*!
 *Enable the autoreboot
 */
void Server::enableAutoReboot()
{
  autoRebootEnabled = true;
}


/*!
 *Return the ProtocolManager object.
 */
ProtocolsManager *Server::getProtocolsManager()
{
  return &protocols;
}

/*!
 *Get the path to the directory containing all the language files.
 */
const char *Server::getLanguagesPath()
{
  return languagesPath ? languagesPath->c_str() : "";
}

/*!
 *Get the current language file.
 */
const char *Server::getLanguageFile()
{
  return languageFile ? languageFile->c_str() : "";
}

/*!
 *Return nonzero if the autoreboot is enabled.
 */
bool Server::isAutorebootEnabled()
{
  return autoRebootEnabled;
}

/*!
 *Create a new thread.
 */
int Server::addThread(bool staticThread)
{
  int ret;
  string msg("new-thread");
  ClientsThread* newThread = 0;
  vector<Multicast<string, void*, int>*>* handlers;

  purgeThreadsThreshold = 1;

  if(isRebooting())
    return -1;

  if(!staticThread)
  {
    bool restored = false;

    for(list<ClientsThread*>::iterator it = threads.begin(); it != threads.end(); it++)
    {
      ClientsThread* thread = *it;

      if(thread->isToDestroy())
      {
        thread->setToDestroy(0);
        restored = true;
        break;
      }
    }
    if(restored)
      return 0;
  }

  newThread = new ClientsThread(this);

  if(newThread == 0)
    return -1;

  handlers = getHandlers(msg);

  if(handlers)
  {
    for(size_t i = 0; i < handlers->size(); i++)
    {
      (*handlers)[i]->updateMulticast(this, msg, newThread);
    }
  }

  newThread->setStatic(staticThread);

  newThread->id = (u_long)(++currentThreadID);

  ret = newThread->run();

  if(ret)
  {
    string str;
    delete newThread;
    str.assign(languageParser.getValue("ERR_ERROR"));
    str.append(": Threads creation" );
    logWriteln(str.c_str(), MYSERVER_LOG_ERROR);
    return -1;
  }

  threads.push_back(newThread);
  return 0;
}

/*!
 *Remove a thread.
 *Return zero if a thread was removed.
 */
int Server::removeThread(u_long ID)
{
  int ret = 1;
  string msg("remove-thread");
  vector<Multicast<string, void*, int>*>* handlers;


  handlers = getHandlers(msg);

  threadsMutex->lock();

  if(handlers)
  {
    for(size_t i = 0; i < handlers->size(); i++)
    {
      (*handlers)[i]->updateMulticast(this, msg, &ID);
    }
  }

  for(list<ClientsThread*>::iterator it = threads.begin(); it != threads.end(); it++)
  {
    if((*it)->id == ID)
    {
      (*it)->stop();
      ret = 0;
      threads.erase(it);
      break;
    }
  }
  threadsMutex->unlock();
  return ret;

}

/*!
 *Create the class instance.  Call this before use
 *the Server class.  The instance is not created in
 *getInstance to have a faster inline function.
 */
void Server::createInstance()
{
  if(instance == 0)
    instance = new Server();
}

/*!
 *Get a pointer to a filters factory object.
 */
FiltersFactory* Server::getFiltersFactory()
{
  return &filtersFactory;
}

/*!
 *Check how many threads are not working.
 */
int Server::countAvailableThreads()
{
 int count = 0;

  threadsMutex->lock();

  count = freeThreads;

  threadsMutex->unlock();

  return count;
}

/*!
 *Write a string to the log file and terminate the line.
 */
int Server::logWriteln(char const* str, LoggingLevel level)
{
  if(!str)
    return 0;
  
  /*
   *If the log receiver is not the console output a timestamp.
   */
  if (logLocation.find ("console://") == string::npos)
  {
    char time[38];
    int len;
    time[0] = '[';
    getRFC822GMTTime(&time[1], 32);
    len = strlen(time);
    time[len + 0] = ']';
    time[len + 1] = ' ';
    time[len + 2] = '-';
    time[len + 3] = '-';
    time[len + 4] = ' ';
    time[len + 5] = '\0';
    if (logManager->log (this, "MAINLOG", logLocation, string (time), false, level))
      return 1;
  }
  return logManager->log (this, "MAINLOG", logLocation, string (str), true, level);
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
 *Get the size for the first buffer.
 */
u_long Server::getBuffersize()
{
  return buffersize;
}

/*!
 *Get the size for the second buffer.
 */
u_long Server::getBuffersize2()
{
  return secondaryBufferSize;
}

/*!
 *Set a global descriptor.
 */
void Server::setGlobalData(const char* name, void* data)
{
  string str(name);
  globalData.put(str, data);
}

/*!
 *Get a global descriptor.
 */
void* Server::getGlobalData(const char* name)
{
  string str(name);
  return globalData.get(str);
}
