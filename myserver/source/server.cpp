/*
MyServer
Copyright (C) 2002-2008 The MyServer Team
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
#include "../include/safetime.h"

/*! Include headers for built-in protocols.  */
#include "../include/http.h"	/*Include the HTTP protocol.  */
#include "../include/https.h" /*Include the HTTPS protocol.  */
#include "../include/control_protocol.h" /*Include the control protocol.  */
#include "../include/ftp.h"

#include "../include/security.h"
#include "../include/stringutils.h"
#include "../include/sockets.h"
#include "../include/gzip.h"
#include "../include/myserver_regex.h"
#include "../include/files_utility.h"
#include "../include/ssl.h"
#include "../include/ssl_sockets.h"

extern "C" 
{
#ifdef WIN32
#include <Ws2tcpip.h>
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

const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;


/*!
 *At startup the server instance is null.
 */
Server* Server::instance = 0;

Server::Server()
{
  toReboot = 0;
  autoRebootEnabled = 1;
  pausing = 0;
	rebooting = 0;
  maxConnections = 0;
  serverReady = 0;
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
	nTotalConnections = 0;
	purgeThreadsThreshold = 1;
	freeThreads = 0;
}

/*!
 *Destroy the object.
 */
Server::~Server()
{

}

/*!
 *Start the server.
 */
void Server::start()
{
  u_long i;
  u_long configsCheck = 0;
	u_long purgeThreadsCounter = 0;
  time_t mainConfTime;
  time_t hostsConfTime;
  time_t mimeConf;
  string buffer;
  int err = 0;
  int osVer = getOSVersion();
#ifdef WIN32
  DWORD eventsCount, cNumRead;
  INPUT_RECORD irInBuf[128];
#endif

#ifdef CLEAR_BOOT_SCREEN

  if(logManager.getType() == LogManager::TYPE_CONSOLE )
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
  if(logManager.getType() == LogManager::TYPE_CONSOLE )
  {
    try
    {
      string software_signature;
      software_signature.assign("************MyServer ");
      software_signature.append(versionOfSoftware);
      software_signature.append("************");

      i = software_signature.length();
      while(i--)
        logManager.write("*");
      logManager.writeln("");
      logManager.write(software_signature.c_str());
      logManager.write("\n");
      i = software_signature.length();
      while(i--)
        logManager.write("*");
      logManager.writeln("");
    }
    catch(exception& e)
    {
      ostringstream err;
      err << "Error: " << e.what();
      logManager.write(err.str().c_str());
      logManager.write("\n");
      return;
    }
    catch(...)
    {
      return;
    };
  }

  try
  {
    /*
     *Set the current working directory.
     */
    setcwdBuffer();

    XmlParser::startXML();

		myserver_safetime_init();

		/*
     *Setup the server configuration.
     */
    logWriteln("Initializing server configuration...");
    err = 0;
    osVer = getOSVersion();

    err = initialize(osVer);
    if(err)
      return;

    /* Initialize the SSL library.  */
		initializeSSL();

    logWriteln( languageParser.getValue("MSG_SERVER_CONF") );

    /* Startup the socket library.  */
    logWriteln( languageParser.getValue("MSG_ISOCK") );
    err = startupSocketLib(/* MAKEWORD( 2, 2 ) */MAKEWORD( 1, 1));
    if (err != 0)
    {
      logPreparePrintError();
      logWriteln( languageParser.getValue("MSG_SERVER_CONF") );
      logEndPrintError();
      return;
    }
    logWriteln( languageParser.getValue("MSG_SOCKSTART") );

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
      logPreparePrintError();
      logWriteln(msg.c_str());
      logEndPrintError();
      return;
    }
    else
    {
      string msg;
      msg.assign("IP: ");
      msg.append(*ipAddresses);
      logWriteln(msg.c_str());
    }

    loadSettings();

    mainConfTime = 
			FilesUtility::getLastModTime(mainConfigurationFile->c_str());
    hostsConfTime = 
			FilesUtility::getLastModTime(vhostConfigurationFile->c_str());
    mimeConf = 
			FilesUtility::getLastModTime(mimeConfigurationFile->c_str());

    /*
     *Keep thread alive.
     *When the mustEndServer flag is set to True exit
     *from the loop and terminate the server execution.
     */
    while(!mustEndServer)
    {
      Thread::wait(100000);

      /* Check threads.  */
			if(purgeThreadsCounter++ >= 100)
			{
				purgeThreadsCounter = 0;
				purgeThreads();
			}

      if(autoRebootEnabled)
      {
        configsCheck++;
        /* Do not check for modified configuration files every cycle.  */
        if(configsCheck > 10)
        {
          time_t mainConfTimeNow =
            FilesUtility::getLastModTime(mainConfigurationFile->c_str());
          time_t hostsConfTimeNow =
            FilesUtility::getLastModTime(vhostConfigurationFile->c_str());
          time_t mimeConfNow =
            FilesUtility::getLastModTime(mimeConfigurationFile->c_str());

          /* If a configuration file was modified reboot the server. */
          if(((mainConfTimeNow!=-1) && (hostsConfTimeNow!=-1)  &&
              (mimeConfNow!=-1)) || toReboot)
          {
            if( (mainConfTimeNow  != mainConfTime) || toReboot)
            {
							string msg("main-conf-changed");
							notifyMulticast(msg, 0);
							
              reboot();
              /* Store new mtime values.  */
              mainConfTime = mainConfTimeNow;
              mimeConf = mimeConfNow;
            }
						else if(mimeConfNow != mimeConf)
						{
							string msg("mime-conf-changed");
							notifyMulticast(msg, 0);

							if(logManager.getType() == LogManager::TYPE_CONSOLE)
							{
								char beep[]={static_cast<char>(0x7), '\0'};
								logManager.write(beep);
							}

							logWriteln("Reloading MIMEtypes.xml");
							
							getMimeManager()->loadXML(getMIMEConfFile());

							logWriteln("Reloaded");

							mimeConf = mimeConfNow;
						}
						else if(hostsConfTimeNow != hostsConfTime)
            {
							VhostManager* oldvhost = vhostList;
							string msg("vhosts-conf-changed");
							notifyMulticast(msg, 0);

							/* Do a beep if outputting to console.  */
							if(logManager.getType() == LogManager::TYPE_CONSOLE)
							{
								char beep[]={static_cast<char>(0x7), '\0'};
								logManager.write(beep);
							}

							logWriteln("Rebooting...");
							

							connectionsScheduler.release();

							Socket::stopBlockingOperations(true);

							listenThreads.beginFastReboot();

							listenThreads.terminate();
							
							connectionsScheduler.terminateConnections();
							clearAllConnections();

							
							Socket::stopBlockingOperations(false);

							connectionsScheduler.restart();
							listenThreads.initialize(&languageParser);

							vhostList = new VhostManager(&listenThreads);

							if(vhostList == 0)
							{
								continue;
							}

							delete oldvhost;

							/* Load the virtual hosts configuration from the xml file.  */
							if(vhostList->loadXMLConfigurationFile(vhostConfigurationFile->c_str(), getMaxLogFileSize()))
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

            configsCheck = 0;
          }
          else
          {
            /*
             *If there are problems in loading mtimes
             *check again after a bit.
             */
            configsCheck = 7;
          }
        }
      }//end  if(autoRebootEnabled)
    }
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
	return nTotalConnections;
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
	mustEndServer = 1;
}

/*!
 *Unload the server.
 *Return nonzero on errors.
 */
int Server::terminate()
{

  if(verbosity > 1)
    logWriteln(languageParser.getValue("MSG_STOPT"));

	listenThreads.terminate();

	connectionsScheduler.release();
	Socket::stopBlockingOperations(true);
	connectionsScheduler.terminateConnections();
	clearAllConnections();

	threadsMutex->lock();
	for(list<ClientsThread*>::iterator it = threads.begin(); it != threads.end(); it++)
	{
		(*it)->stop();
	}
	threadsMutex->unlock();

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

	ipAddresses = 0;
  vhostList = 0;
	languageParser.close();
	mimeManager->clean();
	delete mimeManager;
	mimeManager = 0;
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

	Http::unLoadProtocol(&languageParser);
	Https::unLoadProtocol(&languageParser);
  ControlProtocol::unLoadProtocol(&languageParser);
	/*
   *Destroy the connections mutex.
   */
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
int Server::initialize(int /*!osVer*/)
{
	char *data;
  int ret;
  char buffer[512];
  u_long nbr;
  u_long nbw;
#ifdef WIN32
	envString = GetEnvironmentStrings();
#endif

	/* Create the mutex for the connections.  */
	connectionsMutex = new Mutex();

  /* Create the mutex for the threads.  */
  threadsMutex = new Mutex();

	/* Store the default values.  */
  nStaticThreads = 20;
  nMaxThreads = 50;
  currentThreadID = 0;
	freeThreads = 0;
	connectionTimeout = MYSERVER_SEC(25);
	mustEndServer = 0;
	verbosity = 1;
	purgeThreadsThreshold = 1;
  throttlingRate = 0;
	maxConnections = 0;
  maxConnectionsToAccept = 0;
	if(serverAdmin)
		delete serverAdmin;
	serverAdmin = 0;
	autoRebootEnabled = 1;
	languagesPath = new string();
#ifndef WIN32

	/*
   *Do not use the files in the directory /usr/share/myserver/languages
   *if exists a local directory.
   */
	if(FilesUtility::fileExists("languages"))
	{
		languagesPath->assign(getdefaultwd(0, 0) );
    languagesPath->append("/languages/");

	}
	else
	{
#ifdef PREFIX
		languagesPath->assign(PREFIX);
    languagesPath->append("/share/myserver/languages/");
#else
    /* Default PREFIX is /usr/.  */
		languagesPath->assign("/usr/share/myserver/languages/");
#endif
	}
#endif

	if(mainConfigurationFile)
		delete mainConfigurationFile;
	mainConfigurationFile = new string();

#ifdef WIN32
  languagesPath->assign( "languages/" );
#endif

#ifndef WIN32

  /*
   *Under an *nix environment look for .xml files in the following order.
   *1) myserver executable working directory
   *2) ~/.myserver/
   *3) /etc/myserver/
   *4) default files will be copied in myserver executable working
   */
	if(FilesUtility::fileExists("myserver.xml"))
	{
		mainConfigurationFile->assign("myserver.xml");
	}
	else if(FilesUtility::fileExists("~/.myserver/myserver.xml"))
	{
		mainConfigurationFile->assign("~/.myserver/myserver.xml");
	}
	else if(FilesUtility::fileExists("/etc/myserver/myserver.xml"))
	{
		mainConfigurationFile->assign("/etc/myserver/myserver.xml");
	}
	else
#endif
	/* If the myserver.xml files doesn't exist copy it from the default one.  */
	if(!FilesUtility::fileExists("myserver.xml"))
	{
    mainConfigurationFile->assign("myserver.xml");
		File inputF;
		File outputF;
		ret = inputF.openFile("myserver.xml.default",
													File::MYSERVER_OPEN_READ | 
													File::MYSERVER_OPEN_IFEXISTS);
		if(ret)
		{
			logPreparePrintError();
			logWriteln("Error loading configuration file\n");
			logEndPrintError();
			return -1;
		}
		ret = outputF.openFile("myserver.xml", File::MYSERVER_OPEN_WRITE |
                     File::MYSERVER_OPEN_ALWAYS);
		if(ret)
		{
			logPreparePrintError();
			logWriteln("Error loading configuration file\n");
			logEndPrintError();
			return -1;
		}
		for(;;)
		{
			ret = inputF.readFromFile(buffer, 512, &nbr );
      if(ret)
        return -1;

			if(!nbr)
				break;

			ret = outputF.writeToFile(buffer, nbr, &nbw);
      if(ret)
        return -1;
		}
		inputF.closeFile();
		outputF.closeFile();
	}
	else
  {
		mainConfigurationFile->assign("myserver.xml");
  }
	configurationFileManager.open(mainConfigurationFile->c_str());


	data = configurationFileManager.getValue("VERBOSITY");
	if(data)
	{
		verbosity = (u_long)atoi(data);
	}
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

	data = configurationFileManager.getValue("BUFFER_SIZE");
	if(data)
	{
		buffersize=buffersize2= (atol(data) > 81920) ?  atol(data) :  81920 ;
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
	if(data)
	{
		tmpPath.assign(data);
		FilesUtility::completePath(tmpPath);
	}
	else
	{
		tmpPath.assign(getdefaultwd(0, 0));
	}

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

	if(languageParser.open(languageFile->c_str()))
  {
    string err;
    logPreparePrintError();
    err.assign("Error loading: ");
    err.append( *languageFile );
    logWriteln( err.c_str() );
    logEndPrintError();
    return -1;
  }
	logWriteln(languageParser.getValue("MSG_LANGUAGE"));
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
	{
		addThread(0);
  }
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
											sizeof(sockaddr_in6),	ip, MAX_IP_STRING_LEN, 
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
	else
		port = ntohs(((sockaddr_in6 *)(asockIn))->sin6_port);

  /* Port used by the server. */
		if ( localSockIn.ss_family == AF_INET )
			myPort = ntohs(((sockaddr_in *)(&localSockIn))->sin_port);
	else
		myPort = ntohs(((sockaddr_in6 *)(&localSockIn))->sin6_port);


	if(!addConnectionToList(&s, asockIn, &ip[0], &localIp[0], port, myPort, 1))
	{
		/* If we report error to add the connection to the thread.  */
		ret = 0;

    /* Shutdown the socket both on receive that on send.  */
		s.shutdown(2);

    /* Then close it.  */
		s.closesocket();
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
	ConnectionPtr newConnection = new Connection;
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
	newConnection->host = Server::getInstance()->vhostList->getVHost(0,
																					                   localIpAddr,
                                                             localPort);

  /* No vhost for the connection so bail.  */
	if(newConnection->host == 0)
	{
		delete newConnection;
		return 0;
	}

  protocol = Server::getInstance()->getProtocol(newConnection->host->getProtocolName());
  
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
		newConnection->setParsing(1);
		newConnection->setForceParsing(1);
		connectionsScheduler.addReadyConnection(newConnection, 0);
	}
	else
		connectionsScheduler.addWaitingConnection(newConnection, 0);

	nTotalConnections++;

	/*
   *If defined maxConnections and the number of active connections
   *is bigger than it say to the protocol that will parse the connection
   *to remove it from the active connections list.
   */
	if(maxConnections && 
		 ((u_long)connectionsScheduler.getConnectionsNumber() > maxConnections))
		newConnection->setToRemove(CONNECTION_REMOVE_OVERLOAD);

	/*
	 *Signal the new connection to the waiting threads.
	 */

	return newConnection;
}

/*!
 *Delete a connection from the list.
 */
int Server::deleteConnection(ConnectionPtr s, int /*id*/, int doLock)
{
	string msg("remove-connection");
	vector<Multicast<string, void*, int>*>* handlers;
	int ret = 0;

	/*
   *Remove the connection from the active connections list.
   */
	if(!s || !s->allowDelete())
	{
		return 0;
	}

	if(s->isParsing())
		s->setToRemove(CONNECTION_REMOVE_OVERLOAD);

	handlers = getHandlers(msg);

	if(handlers)
	{
		for(size_t i = 0; i < handlers->size(); i++)
		{
			(*handlers)[i]->updateMulticast(this, msg, s);
		}
	}

	if(doLock)
		connectionsScheduler.lockConnectionsList();

	connectionsScheduler.removeConnection(s);
	delete s;

	if(doLock)
		connectionsScheduler.unlockConnectionsList();

	return ret;
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

	connectionsScheduler.lockConnectionsList();

	connectionsScheduler.getConnections(connections);

	try
	{
		for(it = connections.begin(); it != connections.end(); it++)
		{
			deleteConnection(*it, 1, 0);
		}
  }
  catch(...)
  {
		connectionsScheduler.unlockConnectionsList();
    throw;
  };
	connectionsScheduler.unlockConnectionsList();
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
 *Load the main server settings.
 *Return nonzero on errors.
 */
int Server::loadSettings()
{
	u_long i;
  int ret;
  ostringstream nCPU;
  string strCPU;
  char buffer[512];
  u_long nbr, nbw;
  try
  {
		if(mimeConfigurationFile)
			delete mimeConfigurationFile;
		mimeConfigurationFile = new string();
#ifndef WIN32
    /*
     *Under an *nix environment look for .xml files in the following order.
     *1) myserver executable working directory
     *2) ~/.myserver/
     *3) /etc/myserver/
     *4) default files will be copied in myserver executable working
     */
    if(FilesUtility::fileExists("MIMEtypes.xml"))
    {
      mimeConfigurationFile->assign("MIMEtypes.xml");
    }
    else if(FilesUtility::fileExists("~/.myserver/MIMEtypes.xml"))
    {
      mimeConfigurationFile->assign("~/.myserver/MIMEtypes.xml");
    }
    else if(FilesUtility::fileExists("/etc/myserver/MIMEtypes.xml"))
    {
      mimeConfigurationFile->assign("/etc/myserver/MIMEtypes.xml");
    }
    else
#endif
      /*
       *If the MIMEtypes.xml files doesn't exist copy it
       *from the default one.
       */
      if(!FilesUtility::fileExists("MIMEtypes.xml"))
      {
        File inputF;
        File outputF;
        mimeConfigurationFile->assign("MIMEtypes.xml");
        ret = inputF.openFile("MIMEtypes.xml.default", 
															File::MYSERVER_OPEN_READ |
															File::MYSERVER_OPEN_IFEXISTS);
        if(ret)
        {
          logPreparePrintError();
          logWriteln(languageParser.getValue("ERR_LOADMIME"));
          logEndPrintError();
          return -1;
        }
        ret = outputF.openFile("MIMEtypes.xml", File::MYSERVER_OPEN_WRITE|
                               File::MYSERVER_OPEN_ALWAYS);
        if(ret)
          return -1;

        for(;;)
        {
          ret = inputF.readFromFile(buffer, 512, &nbr );
          if(ret)
            return -1;
          if(nbr==0)
            break;
          ret = outputF.writeToFile(buffer, nbr, &nbw);
          if(ret)
            return -1;
        }
        inputF.closeFile();
        outputF.closeFile();
      }
      else
      {
        mimeConfigurationFile->assign("MIMEtypes.xml");
      }

    if(filtersFactory.insert("gzip", Gzip::factory))
    {
      ostringstream stream;
      stream <<  languageParser.getValue("ERR_ERROR") << ": Gzip Filter";
      logPreparePrintError();
      logWriteln(stream.str().c_str());
      logEndPrintError();
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
      logPreparePrintError();
      logWriteln(languageParser.getValue("ERR_LOADMIME"));
      logEndPrintError();
      return -1;
    }
    nCPU << (u_int)getCPUCount();

    strCPU.assign(languageParser.getValue("MSG_NUM_CPU"));
    strCPU.append(" ");
    strCPU.append(nCPU.str());
    logWriteln(strCPU.c_str());

		vhostConfigurationFile = new string();

#ifndef WIN32
    /*
     *Under an *nix environment look for .xml files in the following order.
     *1) myserver executable working directory
     *2) ~/.myserver/
     *3) /etc/myserver/
     *4) default files will be copied in myserver executable working
     */
    if(FilesUtility::fileExists("virtualhosts.xml"))
    {
      vhostConfigurationFile->assign("virtualhosts.xml");
    }
    else if(FilesUtility::fileExists("~/.myserver/virtualhosts.xml"))
    {
      vhostConfigurationFile->assign("~/.myserver/virtualhosts.xml");
    }
    else if(FilesUtility::fileExists("/etc/myserver/virtualhosts.xml"))
    {
      vhostConfigurationFile->assign("/etc/myserver/virtualhosts.xml");
    }
    else
#endif
      /*
       *If the virtualhosts.xml file doesn't exist copy it
       *from the default one.
       */
    if(!FilesUtility::fileExists("virtualhosts.xml"))
    {
			File inputF;
			File outputF;
			vhostConfigurationFile->assign("virtualhosts.xml");
			ret = inputF.openFile("virtualhosts.xml.default", 
														File::MYSERVER_OPEN_READ |
														File::MYSERVER_OPEN_IFEXISTS );
			if(ret)
      {
				logPreparePrintError();
				logWriteln(languageParser.getValue("ERR_LOADMIME"));
				logEndPrintError();
				return -1;
			}
			ret = outputF.openFile("virtualhosts.xml",
														 File::MYSERVER_OPEN_WRITE | 
														 File::MYSERVER_OPEN_ALWAYS);
			if(ret)
				return -1;
			for(;;)
      {
				ret = inputF.readFromFile(buffer, 512, &nbr );
				if(ret)
					return -1;
				if(!nbr)
					break;
				ret = outputF.writeToFile(buffer, nbr, &nbw);
				if(ret)
					return -1;
			}
			
			inputF.closeFile();
			outputF.closeFile();
		}
		else
		{
			vhostConfigurationFile->assign("virtualhosts.xml");
		}

		connectionsScheduler.restart();

		listenThreads.initialize(&languageParser);
    if(vhostList)
    {
		  delete vhostList;
		  vhostList = 0;
    }

    vhostList = new VhostManager(&listenThreads);
    if(vhostList == 0)
    {
      return -1;
    }
    /* Load the virtual hosts configuration from the xml file.  */
    vhostList->loadXMLConfigurationFile(vhostConfigurationFile->c_str(),
                                        getMaxLogFileSize());

		if(externalPath)
			delete externalPath;
		externalPath = new string();
#ifdef NOT_WIN
    if(FilesUtility::fileExists("external"))
      externalPath->assign("external");
    else
    {
#ifdef PREFIX
      externalPath->assign(PREFIX);
      externalPath->append("/lib/myserver/external");
 #else
      externalPath->assign("/usr/lib/myserver/external");
#endif
    }

#endif

#ifdef WIN32
    externalPath->assign("external/protocols");

#endif
		getProcessServerManager()->load();

    Http::loadProtocol(&languageParser);
    Https::loadProtocol(&languageParser);
    ControlProtocol::loadProtocol(&languageParser);
    Ftp::loadProtocol(&languageParser);

		/* Load the home directories configuration.  */
		homeDir.load();

		getPluginsManager()->addNamespace(&executors);
		getPluginsManager()->addNamespace(&protocols);
		getPluginsManager()->addNamespace(&filters);
 		getPluginsManager()->addNamespace(&genericPluginsManager);

		{
			string res("external");
			getPluginsManager()->preLoad(this, &languageParser, res);
			getPluginsManager()->load(this, &languageParser, res);
			getPluginsManager()->postLoad(this, &languageParser);
		}

		if(path == 0)
			path = new string();

    /* Return 1 if we had an allocation problem.  */
    if(getdefaultwd(*path))
      return -1;

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

    for(i = 0; i < nStaticThreads; i++)
	  {
			logWriteln(languageParser.getValue("MSG_CREATET"));
      ret = addThread(1);
      if(ret)
				return -1;
			logWriteln(languageParser.getValue("MSG_THREADR"));
    }

    logWriteln(languageParser.getValue("MSG_READY"));

    /*
     *Print this message only if the log outputs to the console screen.
     */
    if(logManager.getType() == LogManager::TYPE_CONSOLE)
      logWriteln(languageParser.getValue("MSG_BREAK"));

    /*
     *Server initialization is now completed.
     */
    serverReady = 1;
  }
  catch(bad_alloc& ba)
  {
    logPreparePrintError();
    logWriteln("Error allocating memory");
    logEndPrintError();
    return 1;
  }
  catch(exception& e)
  {
    ostringstream out;
    out << "Error: " << e.what();
    logPreparePrintError();
    logWriteln(out.str().c_str());
    logEndPrintError();
    return 1;
  }
  catch(...)
  {
    return 1;
  }
  return 0;
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
         logPreparePrintError();
         logWriteln(out.str().c_str());
         logEndPrintError();
       }

       if(Process::setgid(gid))
       {
         out << languageParser.getValue("ERR_ERROR") << ": setgid " << gid;
         logPreparePrintError();
         logWriteln(out.str().c_str());
         logEndPrintError();
       }
       autoRebootEnabled = 0;
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
        logPreparePrintError();
        logWriteln(out.str().c_str());
        logEndPrintError();
      }
      autoRebootEnabled = 0;
    }
}


/*!
 *Lock the access to the log file.
 */
int Server::logLockAccess()
{
  return logManager.requestAccess();
}

/*!
 *Unlock the access to the log file.
 */
int Server::logUnlockAccess()
{
  return logManager.terminateAccess();
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
		{
			(*handlers)[i]->updateMulticast(this, msg, 0);
		}
	}

  serverReady = 0;
  /* Reset the toReboot flag.  */
  toReboot = 0;

	rebooting = 1;

  /* Do nothing if the reboot is disabled.  */
  if(!autoRebootEnabled)
    return 0;
  /* Do a beep if outputting to console.  */
  if(logManager.getType() == LogManager::TYPE_CONSOLE)
  {
    char beep[]={static_cast<char>(0x7), '\0'};
    logManager.write(beep);
  }

  logWriteln("Rebooting...");
	if(mustEndServer)
		return 0;
	mustEndServer = 1;

	ret = terminate();
  if(ret)
    return ret;
	mustEndServer = 0;


  rebooting = 0;

  ret = initialize(0);
  if(ret)
    return ret;

  ret = loadSettings();


  return ret;

}

/*!
 *Returns if the server is ready.
 */
int Server::isServerReady()
{
  return serverReady;
}

/*!
 *Reboot the server on the next loop.
 */
void Server::rebootOnNextLoop()
{
	serverReady = 0;
  toReboot = 1;
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
  autoRebootEnabled = 0;
}

/*!
 *Enable the autoreboot
 */
void Server::enableAutoReboot()
{
  autoRebootEnabled = 1;
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
int Server::isAutorebootEnabled()
{
  return autoRebootEnabled;
}

/*!
 *Create a new thread.
 */
int Server::addThread(int staticThread)
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

  newThread = new ClientsThread();

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
    logPreparePrintError();
    str.assign(languageParser.getValue("ERR_ERROR"));
    str.append(": Threads creation" );
    logWriteln(str.c_str());
    logEndPrintError();
    return -1;
  }

  /*
   *If everything was done correctly add the new thread to the linked list.
   */
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
int Server::logWriteln(const char* str)
{
  if(!str)
    return 0;

  /*
   *If the log receiver is not the console output a timestamp.
   */
  if(logManager.getType() != LogManager::TYPE_CONSOLE)
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
    if(logManager.write(time))
      return 1;
  }
  return logManager.writeln((char*)str);
}

/*!
 *Create an unique temporary file name.  This function doesn't create the
 *file or open it but generates only its name.
 *\param
 */
void Server::temporaryFileName(u_long tid, string &out)
{
	ostringstream stream;
	static u_long counter = 1;
	counter++;
	stream << tmpPath << "/tmp_" << counter  << "_" << tid;
	out.assign(stream.str());
}

/*!
 *Prepare the log to print an error.
 */
int Server::logPreparePrintError()
{
  logManager.preparePrintError();
  return 0;
}

/*!
 *Exit from the error printing mode.
 */
int Server::logEndPrintError()
{
  logManager.endPrintError();
  return 0;
}

/*!
 *Use a specified file as log.
 */
int Server::setLogFile(char* fileName)
{
  if(fileName == 0)
  {
    logManager.setType(LogManager::TYPE_CONSOLE);
    return 0;
  }
  return logManager.load(fileName);
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
  return buffersize2;
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
