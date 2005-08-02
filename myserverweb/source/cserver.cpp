/*
*MyServer
*Copyright (C) 2002, 2003, 2004, 2005 The MyServer Team
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "../stdafx.h"
#include "../include/cserver.h"

/*! Include headers for built-in protocols. */
#include "../include/http.h"	/*Include the HTTP protocol. */
#include "../include/https.h" /*Include the HTTPS protocol. */
#include "../include/control_protocol.h" /*Include the control protocol. */

#include "../include/security.h"
#include "../include/stringutils.h"
#include "../include/sockets.h"
#include "../include/gzip.h"
#include "../include/myserver_regex.h"

extern "C" {
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

/*!
 *This is the unique istance for the class Server in the application.
 */
Server *lserver=0;

/*!
 *When the flag mustEndServer is 1 all the threads are 
 *stopped and the application stop its execution.
 */
int mustEndServer;

Server::Server()
{
  threads = 0;
  toReboot = 0;
  autoRebootEnabled = 1;
  listeningThreads = 0;
  nThreads=0;
  pausing=0;
  maxConnections=0;
  nConnections=0;
  connections=0;
  connectionToParse=0;
  serverReady = 0;
  throttlingRate = 0;
  uid= 0;
  gid = 0;

  try
  {
    languages_path.assign("");
    main_configuration_file.assign("");
    vhost_configuration_file.assign("");
    mime_configuration_file.assign("");
  }
  catch(...)
  {
    throw;
  };

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
  u_long configsCheck=0;
  time_t myserver_main_conf;
  time_t myserver_hosts_conf;
  time_t myserver_mime_conf;
  string buffer;
  int err = 0;
  int os_ver=getOSVersion();
#ifdef WIN32
  DWORD eventsCount, cNumRead; 
  INPUT_RECORD irInBuf[128]; 
#endif
  /*!
   *Save the unique instance of this class.
   */
  lserver=this;
  
#ifdef CLEAR_BOOT_SCREEN

  if(logManager.getType() == LogManager::TYPE_CONSOLE )
  {
#ifdef WIN32
    /*!
     *Under the windows platform use the cls operating-system 
     *command to clear the screen.
     */
    _flushall();
    system("cls");
#endif
#ifdef NOT_WIN
	/*!
   *Under an UNIX environment, clearing the screen 
   *can be done in a similar method
   */
    sync();
    system("clear");
#endif 
  }

#endif /*! CLEAR_BOOT_SCREEN. */

  /*!
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
      
      i=software_signature.length();
      while(i--)
        logManager.write("*");
      logManager.writeln("");
      logManager.write(software_signature.c_str());
      logManager.write("\n");    
      i=software_signature.length();
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
    /*!
     *Set the current working directory.
     */
    setcwdBuffer();
    
    XmlParser::startXML();
    /*!
     *Setup the server configuration.
     */
    logWriteln("Initializing server configuration...");
    err = 0;
    os_ver=getOSVersion();
    
    err = initialize(os_ver);
    if(err)
      return;

    /*! Initialize the SSL library.  */
#ifndef DO_NOT_USE_SSL
    SSL_library_init();
    SSL_load_error_strings();
#endif
    logWriteln( languageParser.getValue("MSG_SERVER_CONF") );

    /*! *Startup the socket library.  */
    logWriteln( languageParser.getValue("MSG_ISOCK") );
    err= startupSocketLib(/*!MAKEWORD( 2, 2 )*/MAKEWORD( 1, 1));
    if (err != 0) 
    { 
      logPreparePrintError();
      logWriteln( languageParser.getValue("MSG_SERVER_CONF") );
      logEndPrintError();
      return; 
    } 
    logWriteln( languageParser.getValue("MSG_SOCKSTART") );

    /*!
     *Get the name of the local machine.
     */
    Socket::gethostname(serverName, MAX_COMPUTERNAME_LENGTH);
    
    buffer.assign(languageParser.getValue("MSG_GETNAME"));
    buffer.append(" ");
    buffer.append(serverName);
    logWriteln(buffer.c_str()); 
    
    /*!
     *find the IP addresses of the local machine.
     */
    ipAddresses.clear();
    buffer.assign("Host: ");
    buffer.append(serverName);
    logWriteln(buffer.c_str() ); 

    if(Socket::getLocalIPsList(ipAddresses))
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
      msg.append(ipAddresses);
      logWriteln(msg.c_str());  
    }

    loadSettings();
  
    myserver_main_conf = File::getLastModTime(main_configuration_file.c_str());
    myserver_hosts_conf = File::getLastModTime(vhost_configuration_file.c_str());
    myserver_mime_conf = File::getLastModTime(mime_configuration_file.c_str());
    
    /*!
     *Keep thread alive.
     *When the mustEndServer flag is set to True exit
     *from the loop and terminate the server execution.
     */
    while(!mustEndServer)
    {
      Thread::wait(500);
      if(autoRebootEnabled)
      {
        configsCheck++;
        /*! Do not check for modified configuration files every cycle. */
        if(configsCheck>10)
        {
          time_t myserver_main_conf_now=
            File::getLastModTime(main_configuration_file.c_str());
          time_t myserver_hosts_conf_now=
            File::getLastModTime(vhost_configuration_file.c_str());
          time_t myserver_mime_conf_now=
            File::getLastModTime(mime_configuration_file.c_str());

          /*! If a configuration file was modified reboot the server. */
          if(((myserver_main_conf_now!=-1) && (myserver_hosts_conf_now!=-1)  && 
              (myserver_mime_conf_now!=-1)) || toReboot)
          {
            if( ((myserver_main_conf_now  != myserver_main_conf)  || 
                 (myserver_hosts_conf_now != myserver_hosts_conf) || 
                 (myserver_mime_conf_now  != myserver_mime_conf)) || toReboot  )
            {
              reboot();
              /*! Store new mtime values. */
              myserver_main_conf = myserver_main_conf_now;
              myserver_hosts_conf=myserver_hosts_conf_now;
              myserver_mime_conf=myserver_mime_conf_now;
            }
            configsCheck=0;
          }
          else
          {
            /*! 
             *If there are problems in loading mtimes 
             *check again after a bit. 
             */
            configsCheck=7;
          }
        }
      }//end  if(autoRebootEnabled)
    
      /*! Check threads. */
      purgeThreads();

#ifdef WIN32
      /*!
       *ReadConsoleInput is a blocking call, so be sure that there are 
       *events before call it
       */
      err = GetNumberOfConsoleInputEvents(GetStdHandle(STD_INPUT_HANDLE), 
                                          &eventsCount);
      if(!err)
        eventsCount = 0;
      while(eventsCount--)
      {
        if(!mustEndServer)
          ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), irInBuf, 128, &cNumRead);
        else
          break;
        for (i = 0; i < cNumRead; i++) 
        {
          switch(irInBuf[i].EventType) 
          { 
					case KEY_EVENT:
						if(irInBuf[i].Event.KeyEvent.wRepeatCount!=1)
							continue;
						if(irInBuf[i].Event.KeyEvent.wVirtualKeyCode=='c'||
               irInBuf[i].Event.KeyEvent.wVirtualKeyCode=='C')
            {
							if((irInBuf[i].Event.KeyEvent.dwControlKeyState & 
                  LEFT_CTRL_PRESSED)|
                 (irInBuf[i].Event.KeyEvent.dwControlKeyState & 
                  RIGHT_CTRL_PRESSED))
							{
                logWriteln(languageParser.getValue("MSG_SERVICESTOP"));
								this->stop();
							}
						}
						break; 
          } 
        }
      }
#endif
    }
  }
  catch( bad_alloc &ba)
  {
    ostringstream s;
    s << "Bad alloc: " << ba.what();
    logWriteln(s.str().c_str());
  }
  catch( exception &e)
  {
    ostringstream s;
    s << "Error: " << e.what();
    logWriteln(s.str().c_str());
  };
	this->terminate();
	finalCleanup();
}

/*!
 *Removed threads that can be destroyed.
 *The function returns the number of threads that were destroyed.
 */
int Server::purgeThreads()
{
  /*!
   *We don't need to do anything.
   */
  ClientsThread *thread;
  ClientsThread *prev;
  int prev_threads_count;
  if(nThreads == nStaticThreads)
    return 0;

  threads_mutex->lock();
  thread = threads;
  prev = 0;
  prev_threads_count = nThreads;
  while(thread)
  {
    if(nThreads == nStaticThreads)
    {
      threads_mutex->unlock();
      return prev_threads_count- nThreads;
    }

    /*!
     *Shutdown all threads that can be destroyed.
     */
    if(thread->isToDestroy())
    {
      if(prev)
        prev->next = thread->next;
      else
        threads = thread->next;
      thread->stop();
      while(!thread->threadIsStopped)
      {
        Thread::wait(100);
      }
      nThreads--;
      {
        ClientsThread *toremove = thread;
        thread = thread->next;
        delete toremove;
      }
      continue;
    }
    prev = thread;
    thread = thread->next;
  }
  threads_mutex->unlock();

  return prev_threads_count- nThreads;
}

/*!
 *Do the final cleanup. Called only once.
 */
void Server::finalCleanup()
{
	XmlParser::cleanXML();
  freecwdBuffer();
}

/*!
 *This function is used to create a socket server and a thread listener 
 *for a protocol.
 */
int Server::createServerAndListener(u_long port)
{
  
	int optvalReuseAddr=1;
  ostringstream port_buff;
  string listen_port_msg;
	ThreadID threadId=0;
  listenThreadArgv* argv;
	MYSERVER_SOCKADDRIN sock_inserverSocket;
	Socket *serverSocket;
	/*!
   *Create the server socket.
   */
  try
  {
    logWriteln(languageParser.getValue("MSG_SSOCKCREATE"));
    serverSocket = new Socket();
    if(!serverSocket)
      return 0;
    serverSocket->socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket->getHandle()==(SocketHandle)INVALID_SOCKET)
    {
      logPreparePrintError();
      logWriteln(languageParser.getValue("ERR_OPENP"));
      logEndPrintError();
      return 0;
    }
    logWriteln(languageParser.getValue("MSG_SSOCKRUN"));
    sock_inserverSocket.sin_family=AF_INET;
    sock_inserverSocket.sin_addr.s_addr=htonl(INADDR_ANY);
    sock_inserverSocket.sin_port=htons((u_short)port);
 
#ifdef NOT_WIN
    /*!
     *Under the unix environment the application needs some time before
     * create a new socket for the same address. 
     *To avoid this behavior we use the current code.
     */
    if(serverSocket->setsockopt(SOL_SOCKET, SO_REUSEADDR, 
                                (const char *)&optvalReuseAddr, 
                                sizeof(optvalReuseAddr))<0)
    {
      logPreparePrintError();
      logWriteln(languageParser.getValue("ERR_ERROR"));
      logEndPrintError();
      return 0;
    }
#endif
	/*!
   *Bind the port.
   */
    logWriteln(languageParser.getValue("MSG_BIND_PORT"));
    
    if(serverSocket->bind((sockaddr*)&sock_inserverSocket, 
                          sizeof(sock_inserverSocket))!=0)
    {
      logPreparePrintError();
      logWriteln(languageParser.getValue("ERR_BIND"));
      logEndPrintError();
      return 0;
    }
    logWriteln(languageParser.getValue("MSG_PORT_BOUND"));
  
    /*!
     *Set connections listen queque to max allowable.
     */
    logWriteln( languageParser.getValue("MSG_SLISTEN"));
    if (serverSocket->listen(SOMAXCONN))
    { 
      logPreparePrintError();
      logWriteln(languageParser.getValue("ERR_LISTEN"));
      logEndPrintError();	
      return 0; 
    }

    port_buff << (u_int)port;
    
    listen_port_msg.assign(languageParser.getValue("MSG_LISTEN"));
    listen_port_msg.append(": ");
    listen_port_msg.append(port_buff.str());
    
    logWriteln(listen_port_msg.c_str());
    
    logWriteln(languageParser.getValue("MSG_LISTENTR"));
    
    /*!
     *Create the listen thread.
     */
    argv=new listenThreadArgv;
    argv->port=port;
    argv->serverSocket=serverSocket;
    
    Thread::create(&threadId, &::listenServer,  (void *)(argv));
    return (threadId) ? 1 : 0 ;
  }
  catch( bad_alloc &ba)
  {
    ostringstream s;
    s << "Bad allocation :" << ba.what();
    logWriteln(s.str().c_str());  
  }
  catch( exception &e)
  {
    ostringstream s;
    s << "Error :" << e.what();
    logWriteln(s.str().c_str());  
  };
  return 0;
}

/*!
 *Create a listen thread for every port used by MyServer.
 */
void Server::createListenThreads()
{
	/*!
   *Create the listens threads.
   *Every port uses a thread.
   */
	for(sVhostList *list=vhostList->getVHostList(); list; list=list->next)
	{
		int needThread=1;
		sVhostList *list2=vhostList->getVHostList();
    /*! No port was specified. */
		if(list->host->getPort()==0)
			continue;
		for(;;)
		{
			list2=list2->next;
			if(list2==0)
				break;
			if(list2==list)
				break;
      /*! 
       *If there is still a thread listening on the specified 
       *port do not create the thread here.
       */
			if(list2->host->getPort() == list->host->getPort())
				needThread=0;
		}

		if(needThread)
		{
			if(createServerAndListener(list->host->getPort())==0)
			{
        string err;
				logPreparePrintError();
        err.assign(languageParser.getValue("ERR_ERROR"));
        err.append(": Listen threads");
        logWriteln( err.c_str() );     
				logEndPrintError();
				return;
			}
		}
	}


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
 *This is the thread that listens for a new connection on the 
 *port specified by the protocol.
 */
#ifdef WIN32
unsigned int __stdcall listenServer(void* params)
#endif
#ifdef HAVE_PTHREAD
void * listenServer(void* params)
#endif
{
	char buffer[256];
	int err;
	listenThreadArgv *argv=(listenThreadArgv*)params;
	Socket *serverSocket=argv->serverSocket;
	MYSERVER_SOCKADDRIN asock_in;
	int asock_inLen = sizeof(asock_in);
	Socket asock;
  int ret;

#ifdef NOT_WIN
	// Block SigTerm, SigInt, and SigPipe in threads
	sigset_t sigmask;
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGPIPE);
	sigaddset(&sigmask, SIGINT);
	sigaddset(&sigmask, SIGTERM);
	sigprocmask(SIG_SETMASK, &sigmask, NULL);
#endif
	delete argv;

  ret = serverSocket->setNonBlocking(1);

	lserver->increaseListeningThreadCount();

  if(lserver->getUid() | lserver->getGid())
  {
    int uid = lserver->getUid();
    int gid = lserver->getGid();

    /**
     *Change the user and group identifier to -1 
     *if they are not specified.
     */    
    if(!uid)
      uid = -1;

    if(!gid)
      gid = -1;
 
    /*! 
     *Change the log files owner if a different user or group
     *identifier is specified.
     */
    for(int i=0; ; i++)
    {
      Vhost* vh = lserver->vhostList->getVHostByNumber(i);
      /*! Break if we reach the end of the list. */
      if(!vh)
        break;

      /*! Chown the log files. */
      err  = File::chown(vh->getAccessesLogFileName(), uid, gid);
      if(err)
      {
        string str;
        str.assign("Error changing owner for: ");
        str.append(vh->getAccessesLogFileName());
        lserver->logPreparePrintError();
        lserver->logWriteln(str);
        lserver->logEndPrintError();    
      }

      err = File::chown(vh->getWarningsLogFileName(), uid, gid);  
      if(err)
      {
        string str;
        str.assign("Error changing owner for: ");
        str.append(vh->getWarningsLogFileName());
        lserver->logPreparePrintError();
        lserver->logWriteln(str);
        lserver->logEndPrintError();
      }

    }

  }

  if(lserver->getUid() && Process::setuid(lserver->getUid()))
  {
    ostringstream out;
    out << lserver->getLanguageParser()->getValue("ERR_ERROR") 
        << ": setuid " << lserver->getUid();
    lserver->logPreparePrintError();
    lserver->logWriteln(out.str().c_str());
    lserver->logEndPrintError();
    Thread::terminate();
    return 0;

  }	
  if(lserver->getGid() && Process::setgid(lserver->getGid()))
  {
    ostringstream out;
    out << lserver->getLanguageParser()->getValue("ERR_ERROR")
        << ": setgid "  << lserver->getGid();
    lserver->logPreparePrintError();
    lserver->logWriteln(out.str().c_str());
    lserver->logEndPrintError();
    Thread::terminate();
    return 0;
  }	

	while(!mustEndServer)
	{
    int timeoutValue = 3;

#ifdef __linux__
    timeoutValue = 1;
#endif
#ifdef __HURD__
    timeoutValue = 5;
#endif

		/*!
     *Accept connections.
     *Every new connection is sended to Server::addConnection function;
     *this function sends connections between the various threads.
     */
		if(serverSocket->dataOnRead(timeoutValue, 0) == 0 )
		{
			Thread::wait(10);
			continue;
		}
		asock = serverSocket->accept((struct sockaddr*)&asock_in, 
                                 &asock_inLen);
		if(asock.getHandle()==0)
			continue;
		if(asock.getHandle()==(SocketHandle)INVALID_SOCKET)
			continue;
		asock.setServerSocket(serverSocket);
		lserver->addConnection(asock, &asock_in);
	}
	
	/*!
   *When the flag mustEndServer is 1 end current thread and clean
   *the socket used for listening.
   */
	serverSocket->shutdown( SD_BOTH);
	do
	{
		err=serverSocket->recv(buffer, 256, 0);
	}while(err!=-1);
	serverSocket->closesocket();
  delete serverSocket;
	lserver->decreaseListeningThreadCount();
	
	/*!
   *Automatically free the current thread.
   */	
	Thread::terminate();
	return 0;

}

/*!
 *Returns the numbers of active connections the list.
 */
u_long Server::getNumConnections()
{
	return nConnections;
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
 *Stop the execution of the server.
 */
void Server::stop()
{
	mustEndServer=1;
}

/*!
 *Unload the server.
 *Return nonzero on errors.
 */
int Server::terminate()
{
	/*!
   *Stop the server execution.
   */
  ClientsThread* thread = threads ;

  if(verbosity>1)
    logWriteln(languageParser.getValue("MSG_STOPT"));

  while(thread)
  {
    thread->stop();
    thread = thread->next;
  }

	while(lserver->getListeningThreadCount())
	{
		Thread::wait(1000);
	}

  /*! Stop the active hreads. */
	stopThreads();

  if(verbosity>1)
    logWriteln(languageParser.getValue("MSG_TSTOPPED"));


	if(verbosity>1)
	{
    logWriteln(languageParser.getValue("MSG_MEMCLEAN"));
	}
	
	/*!
   *If there are open connections close them.
   */
	if(connections)
	{
		clearAllConnections();
	}
  languages_path.assign("");
  languageFile.assign("");
	delete vhostList;
  main_configuration_file.assign("");
  vhost_configuration_file.assign("");
	mime_configuration_file.assign("");
	
  path.assign("");
  vhostList = 0;
	languageParser.close();
	mimeManager.clean();
#ifdef WIN32
	/*!
   *Under WIN32 cleanup environment strings.
   */
	FreeEnvironmentStrings((LPTSTR)envString);
#endif
	Http::unloadProtocol(&languageParser);
	Https::unloadProtocol(&languageParser);
  ControlProtocol::unloadProtocol(&languageParser);
	protocols.unloadProtocols(&languageParser);

  filters.clear();
  filtersFactory.free();

	/*!
   *Destroy the connections mutex.
   */
	delete connections_mutex;

  /*!
   *free all the threads.
   */
  thread = threads;
  while(thread)
  {
    ClientsThread* next = thread->next;
    delete thread;
    thread = next;
  }
	threads = 0;
  delete threads_mutex;

	nStaticThreads = 0;
	if(verbosity > 1)
	{
		logWriteln("MyServer is stopped");
	}
  return 0;
}

/*!
 *Stop all the threads.
 */
void Server::stopThreads()
{
	/*!
   *Clean here the memory allocated.
   */
	u_long threadsStopped=0;
	u_long threadsStopTime=0;

	/*!
   *Wait before clean the threads that all the threads are stopped.
   */
  ClientsThread* thread = threads;
  while(thread)
  {
    thread->clean();
    thread = thread->next;
  }

	threadsStopTime=0;
	for(;;)
	{
		threadsStopped=0;

    thread = threads;
    while(thread)
    {
      if( thread->isStopped() )
				threadsStopped++;
      thread = thread->next;
    }
		/*!
     *If all the threads are stopped break the loop.
     */
		if(threadsStopped == nStaticThreads)
			break;

		/*!
     *Do not wait a lot to kill the thread.
     */
		if(++threadsStopTime > 500 )
			break;
		Thread::wait(200);
	}

}
/*!
 *Get the server administrator e-mail address.
 *To change this use the main configuration file.
 */
const char *Server::getServerAdmin()
{
	return serverAdmin.c_str();
}

/*!
 *Here is loaded the configuration of the server.
 *The configuration file is a XML file.
 *Return nonzero on errors.
 */
int Server::initialize(int /*!os_ver*/)
{
	char *data;
  int ret;
  char buffer[512];
  u_long nbr;
  u_long nbw;
#ifdef WIN32
	envString=GetEnvironmentStrings();
#endif
	/*!
   *Create the mutex for the connections.
   */
	connections_mutex = new Mutex();

  /*!
   *Create the mutex for the threads.
   */
  threads_mutex = new Mutex();

	/*!
   *Store the default values.
   */
  nStaticThreads = 20;
  nMaxThreads = 50;
  currentThreadID = ClientsThread::ID_OFFSET;
	connectionTimeout = MYSERVER_SEC(25);
	mustEndServer=0;
	verbosity=1;
  throttlingRate = 0;
	maxConnections=0;
  maxConnectionsToAccept=0;
	serverAdmin.assign("");
	autoRebootEnabled = 1;
#ifndef WIN32
	/*! 
   *Do not use the files in the directory /usr/share/myserver/languages
   *if exists a local directory.
   */
	if(File::fileExists("languages"))
	{
		languages_path.assign(getdefaultwd(0, 0) );
    languages_path.append("/languages/");
 
	}
	else
	{
#ifdef PREFIX
		languages_path.assign(PREFIX );
    languages_path.append("/share/myserver/languages/");
#else
    /*! Default PREFIX is /usr/. */
		languages_path.assign("/usr/share/myserver/languages/");
#endif
	}
#endif

#ifdef WIN32
  languages_path.assign( "languages/" );
#endif 

#ifndef WIN32
  /*!
   *Under an *nix environment look for .xml files in the following order.
   *1) myserver executable working directory
   *2) ~/.myserver/
   *3) /etc/myserver/
   *4) default files will be copied in myserver executable working	
   */
	if(File::fileExists("myserver.xml"))
	{
		main_configuration_file.assign("myserver.xml");
	}
	else if(File::fileExists("~/.myserver/myserver.xml"))
	{
		main_configuration_file.assign("~/.myserver/myserver.xml");
	}
	else if(File::fileExists("/etc/myserver/myserver.xml"))
	{
		main_configuration_file.assign("/etc/myserver/myserver.xml");
	}
	else
#endif
	/*! If the myserver.xml files doesn't exist copy it from the default one. */
	if(!File::fileExists("myserver.xml"))
	{
    main_configuration_file.assign("myserver.xml");
		File inputF;
		File outputF;
		ret = inputF.openFile("myserver.xml.default", 
                              FILE_OPEN_READ|FILE_OPEN_IFEXISTS);
		if(ret)
		{
			logPreparePrintError();
			logWriteln("Error loading configuration file\n");
			logEndPrintError();
			return -1;
		}
		ret = outputF.openFile("myserver.xml", FILE_OPEN_WRITE | 
                     FILE_OPEN_ALWAYS);
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
		main_configuration_file.assign("myserver.xml");
  }
	configurationFileManager.open(main_configuration_file.c_str());


	data=configurationFileManager.getValue("VERBOSITY");
	if(data)
	{
		verbosity=(u_long)atoi(data);
	}
	data=configurationFileManager.getValue("LANGUAGE");
	if(data)
	{
    languageFile.assign(languages_path);
    languageFile.append("/");
    languageFile.append(data);
	}
	else
	{
		languageFile.assign("languages/english.xml");
	}

	data=configurationFileManager.getValue("BUFFER_SIZE");
	if(data)
	{
		buffersize=buffersize2= (atol(data) > 81920) ?  atol(data) :  81920 ;
	}
	data=configurationFileManager.getValue("CONNECTION_TIMEOUT");
	if(data)
	{
		connectionTimeout=MYSERVER_SEC((u_long)atol(data));
	}

	data=configurationFileManager.getValue("NTHREADS_STATIC");
	if(data)
	{
		nStaticThreads = atoi(data);
	}
	data = configurationFileManager.getValue("NTHREADS_MAX");
	if(data)
	{
		nMaxThreads = atoi(data);
	}

	/*! Get the max connections number to allow. */
	data = configurationFileManager.getValue("MAX_CONNECTIONS");
	if(data)
	{
		maxConnections=atoi(data);
	}
	/*! Get the max connections number to allow. */
	data = configurationFileManager.getValue("MAX_CONNECTIONS_TO_ACCEPT");
	if(data)
	{
		maxConnectionsToAccept=atoi(data);
	}
	/*! Get the default throttling rate to use on connections. */
	data = configurationFileManager.getValue("THROTTLING_RATE");
	if(data)
	{
		throttlingRate=(u_long)atoi(data);
	}
	/*! Load the server administrator e-mail. */
	data=configurationFileManager.getValue("SERVER_ADMIN");
	if(data)
	{
		serverAdmin.assign(data);
	}

	data=configurationFileManager.getValue("MAX_LOG_FILE_SIZE");
	if(data)
	{
		maxLogFileSize=(u_long)atol(data);
	}
  data = configurationFileManager.getValue("PROCESS_USER_ID");
	if(data)
	{
		uid=atoi(data);
	}
  data = configurationFileManager.getValue("PROCESS_GROUP_ID");
	if(data)
	{
		gid=atoi(data);
	}

	configurationFileManager.close();
	
	if(languageParser.open(languageFile.c_str()))
  {
    string err;
    logPreparePrintError();
    err.assign("Error loading: ");
    err.append(languageFile );
    logWriteln( err.c_str() );     
    logEndPrintError();
    return -1;
  }
	logWriteln(languageParser.getValue("MSG_LANGUAGE"));
	return 0;
	
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
int Server::addConnection(Socket s, MYSERVER_SOCKADDRIN *asock_in)
{

	int ret=1;
	/*!
   *ip is the string containing the address of the remote host 
   *connecting to the server.
   *local_ip is the local addrress used by the connection.
   */
	char ip[MAX_IP_STRING_LEN];
	char local_ip[MAX_IP_STRING_LEN];
	MYSERVER_SOCKADDRIN  localsock_in;
  int dim;
  /*! Remote port used by the client to open the connection. */
	int port;
  /*! Port used by the server to listen. */
	int myport;

	if( s.getHandle() == 0 )
		return 0;
  
  /*!
   *Do not accept this connection if a MAX_CONNECTIONS_TO_ACCEPT limit is defined.
   */
  if(maxConnectionsToAccept && (nConnections >= maxConnectionsToAccept))
    return 0;

  /*!
   *Create a new thread if there are not available threads and
   *we did not reach the limit.
   */
  if((nThreads < nMaxThreads) && (countAvailableThreads() == 0))
  {
    addThread(0);
  }

	memset(&localsock_in,  0,  sizeof(localsock_in));

	dim=sizeof(localsock_in);
	s.getsockname((MYSERVER_SOCKADDR*)&localsock_in, &dim);

  // NOTE: inet_ntop supports IPv6
	strncpy(ip,  inet_ntoa(asock_in->sin_addr),  MAX_IP_STRING_LEN); 

  // NOTE: inet_ntop supports IPv6
	strncpy(local_ip,  inet_ntoa(localsock_in.sin_addr),  MAX_IP_STRING_LEN); 

  /*! Port used by the client. */
	port=ntohs((*asock_in).sin_port);

  /*! Port used by the server. */
	myport=ntohs(localsock_in.sin_port);

	if(!addConnectionToList(s, asock_in, &ip[0], &local_ip[0], port, myport, 1))
	{
		/*! If we report error to add the connection to the thread. */
		ret=0;

    /*! Shutdown the socket both on receive that on send. */
		s.shutdown(2);

    /*! Then close it. */
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
ConnectionPtr Server::addConnectionToList(Socket s, 
                        MYSERVER_SOCKADDRIN* /*asock_in*/, char *ipAddr, 
                        char *localIpAddr, int port, int localPort, int /*id*/)
{
  static u_long connection_ID = 0;
	int doSSLhandshake=0;
	ConnectionPtr new_connection=new Connection;
	if(!new_connection)
	{
		return NULL;
	}
  new_connection->socket = s;
	new_connection->setPort(port);
	new_connection->setTimeout( get_ticks() );
	new_connection->setLocalPort((u_short)localPort);
	new_connection->setIpAddr(ipAddr);
	new_connection->setLocalIpAddr(localIpAddr);
	new_connection->host = (void*)lserver->vhostList->getVHost(0, localIpAddr, 
                                                             (u_short)localPort);

  /*! No vhost for the connection so bail. */
	if(new_connection->host == 0) 
	{
		delete new_connection;
		return 0;
	}
	if(((Vhost*)new_connection->host)->getProtocol() > 1000	)
	{
		doSSLhandshake=1;
	}
	else if(((Vhost*)new_connection->host)->getProtocol() == PROTOCOL_UNKNOWN)
	{	
		DynamicProtocol* dp;
    dp=lserver->getDynProtocol(((Vhost*)(new_connection->host))->getProtocolName());
		if(dp->getOptions() & PROTOCOL_USES_SSL)
			doSSLhandshake=1;	
	}
	
	/*! Do the SSL handshake if required. */
	if(doSSLhandshake)
	{
		int ret =0;
#ifndef DO_NOT_USE_SSL
		SSL_CTX* ctx=((Vhost*)new_connection->host)->getSSLContext();
		new_connection->socket.setSSLContext(ctx);
		ret=new_connection->socket.sslAccept();
		
#endif		
		if(ret<0)
		{
			/*! free the connection on errors. */
			delete new_connection;
			return 0;
		}
	}
	if(new_connection->host==0)
	{
		delete new_connection;
		return 0;
	}
	/*! Update the list. */
  try
  {
    lserver->connections_mutex_lock();
    connection_ID++;
    new_connection->setID(connection_ID);
    new_connection->next = connections;
   	connections=new_connection;
    nConnections++;
    lserver->connections_mutex_unlock();
  }
  catch(...)
  {
    logPreparePrintError();
    logWriteln("Error adding connection to list");
    logEndPrintError();	
    lserver->connections_mutex_unlock();
  };

	/*!
   *If defined maxConnections and the number of active connections 
   *is bigger than it say to the protocol that will parse the connection 
   *to remove it from the active connections list.
   */
	if(maxConnections && (nConnections>maxConnections))
		new_connection->setToRemove(CONNECTION_REMOVE_OverLOAD);

	return new_connection;
}

/*!
 *Delete a connection from the list.
 */
int Server::deleteConnection(ConnectionPtr s, int /*id*/, int doLock)
{
	int ret=0;

	/*!
   *Remove the connection from the active connections list. 
   */
	ConnectionPtr prev=0;

	if(!s)
	{
		return 0;
	}

	/*!
   *Get the access to the connections list.
   */
  if(doLock)
    connections_mutex_lock();

	for(ConnectionPtr i=connections;i;i=i->next )
	{
		if(i == s)
		{
      if(connectionToParse == i)
        connectionToParse=connectionToParse->next;

			if(prev)
				prev->next = i->next;
			else
				connections = i->next;
			ret=1;
			break;
		}
		else
		{
			prev=i;
		}
	}
	nConnections--;

  if(doLock)
    connections_mutex_unlock();

	delete s;
	return ret;
}

/*!
 *Get a connection to parse. Be sure to have the connections mutex
 *access for the caller thread before use this.
 *Using this without the right permissions can cause wrong data 
 *returned to the client.
 */
ConnectionPtr Server::getConnection(int /*id*/)
{
	/*! Do nothing if there are not connections. */
	if(connections==0)
		return 0;
	/*! Stop the thread if the server is pausing.*/
	while(pausing)
	{
		Thread::wait(5);
	}
	if(connectionToParse)
	{
			connectionToParse=connectionToParse->next;
	}
	else
	{
    /*! Link to the first element in the linked list. */
		connectionToParse=connections;
	}
  /*! 
   *Check that we are not in the end of the linked list, in that
   *case link to the first node.
   */
	if(!connectionToParse)
		connectionToParse=connections;

	return connectionToParse;
}

/*!
 *Delete all the active connections.
 */
void Server::clearAllConnections()
{
	ConnectionPtr c;
	ConnectionPtr next;
	connections_mutex_lock();
  try
  {	
    c=connections;
    next=0;
    while(c)
    {
      next=c->next;
      deleteConnection(c, 1, 0);
      c=next;
    }
  }
  catch(...)
  {
    connections_mutex_unlock();
    throw;
  };
	connections_mutex_unlock();
	/*! Reset everything.	*/
	nConnections=0;
	connections=0;
	connectionToParse=0;
}

/*!
 *find a connection passing its socket.
 */
ConnectionPtr Server::findConnectionBySocket(Socket a)
{
	ConnectionPtr c;
	connections_mutex_lock();
	for(c=connections;c;c=c->next )
	{
		if(c->socket==a)
		{
			connections_mutex_unlock();
			return c;
		}
	}
	connections_mutex_unlock();
	return NULL;
}

/*!
 *find a connection in the list by its ID.
 */
ConnectionPtr Server::findConnectionByID(u_long ID)
{
	ConnectionPtr c;
	connections_mutex_lock();
	for(c=connections;c;c=c->next )
	{
		if(c->getID()==ID)
		{
			connections_mutex_unlock();
			return c;
		}
	}
	connections_mutex_unlock();
	return 0;
}

/*!
 *Returns the full path of the binaries directory. 
 *The directory where the file myserver(.exe) is.
 */
const char *Server::getPath()
{
	return path.c_str();
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
	return nThreads;
}

/*!
 *Returns a comma-separated local machine IPs list.
 *For example: 192.168.0.1, 61.62.63.64, 65.66.67.68.69
 */
const char *Server::getAddresses()
{
	return ipAddresses.c_str();
}

/*!
 *Get the dynamic protocol to use for that connection.
 *While built-in protocols have an object per thread, for dynamic
 *protocols there is only one object shared among the threads.
 */
DynamicProtocol* Server::getDynProtocol(const char *protocolName)
{
	return protocols.getDynProtocol(protocolName);
}

/*!
 *Lock connections list access to the caller thread.
 */
int Server::connections_mutex_lock()
{
	connections_mutex->lock();
	return 1;
}

/*!
 *Unlock connections list access.
 */
int Server::connections_mutex_unlock()
{
	connections_mutex->unlock();
	return 1;
}

/*!
 *Get where external protocols are.
 */
const char* Server::getExternalPath()
{
  return externalPath.c_str();
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
#ifndef WIN32
    /*! 
     *Under an *nix environment look for .xml files in the following order.
     *1) myserver executable working directory
     *2) ~/.myserver/
     *3) /etc/myserver/
     *4) default files will be copied in myserver executable working	
     */
    if(File::fileExists("MIMEtypes.xml"))
    {
      mime_configuration_file.assign("MIMEtypes.xml");
    }
    else if(File::fileExists("~/.myserver/MIMEtypes.xml"))
    {
      mime_configuration_file.assign("~/.myserver/MIMEtypes.xml");
    }
    else if(File::fileExists("/etc/myserver/MIMEtypes.xml"))
    {
      mime_configuration_file.assign("/etc/myserver/MIMEtypes.xml");
    }
    else
#endif
      /*! 
       *If the MIMEtypes.xml files doesn't exist copy it 
       *from the default one. 
       */
      if(!File::fileExists("MIMEtypes.xml"))
      {
        File inputF;
        File outputF;
        mime_configuration_file.assign("MIMEtypes.xml");
        ret=inputF.openFile("MIMEtypes.xml.default", FILE_OPEN_READ|
                            FILE_OPEN_IFEXISTS);
        if(ret)
        {
          logPreparePrintError();
          logWriteln(languageParser.getValue("ERR_LOADMIME"));
          logEndPrintError();	
          return -1;
        }
        ret = outputF.openFile("MIMEtypes.xml", FILE_OPEN_WRITE|
                               FILE_OPEN_ALWAYS);
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
        mime_configuration_file.assign("MIMEtypes.xml");
      }

    if(filtersFactory.insert("gzip", Gzip::factory))
    {
      ostringstream stream;
      stream <<  languageParser.getValue("ERR_ERROR") << ": Gzip Filter";
      logPreparePrintError();
      logWriteln(stream.str().c_str());
      logEndPrintError();
    }

    /*! Load the MIME types. */
    logWriteln(languageParser.getValue("MSG_LOADMIME"));
    if(int nMIMEtypes=mimeManager.loadXML(mime_configuration_file.c_str()))
    {
      ostringstream stream;
      stream <<  languageParser.getValue("MSG_MIMERUN") << ": " << nMIMEtypes;
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

#ifndef WIN32
    /*!
     *Under an *nix environment look for .xml files in the following order.
     *1) myserver executable working directory
     *2) ~/.myserver/
     *3) /etc/myserver/
     *4) default files will be copied in myserver executable working	
     */
    if(File::fileExists("virtualhosts.xml"))
    {
      vhost_configuration_file.assign("virtualhosts.xml");
    }
    else if(File::fileExists("~/.myserver/virtualhosts.xml"))
    {
      vhost_configuration_file.assign("~/.myserver/virtualhosts.xml");
    }
    else if(File::fileExists("/etc/myserver/virtualhosts.xml"))
    {
      vhost_configuration_file.assign("/etc/myserver/virtualhosts.xml");
    }
    else
#endif
      /*! 
       *If the virtualhosts.xml file doesn't exist copy it 
       *from the default one. 
       */
      if(!File::fileExists("virtualhosts.xml"))
      {
        File inputF;
        File outputF;
        vhost_configuration_file.assign("virtualhosts.xml");		
        ret = inputF.openFile("virtualhosts.xml.default", FILE_OPEN_READ | 
                              FILE_OPEN_IFEXISTS );
        if(ret)
        {
          logPreparePrintError();
          logWriteln(languageParser.getValue("ERR_LOADMIME"));
          logEndPrintError();	
          return -1;
        }
        ret = outputF.openFile("virtualhosts.xml", 
                               FILE_OPEN_WRITE|FILE_OPEN_ALWAYS);
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
        vhost_configuration_file.assign("virtualhosts.xml");
      }
    vhostList = new VhostManager();
    if(vhostList == 0)
    {
      return -1;
    }
    /*! Load the virtual hosts configuration from the xml file. */
    vhostList->loadXMLConfigurationFile(vhost_configuration_file.c_str(), 
                                        this->getMaxLogFileSize());
    
#ifdef NOT_WIN
    if(File::fileExists("external"))
      externalPath.assign("external");
    else
    {
#ifdef PREFIX
      externalPath.assign(PREFIX);
      externalPath.append("/lib/myserver/external");
 #else
      externalPath.assign("/usr/lib/myserver/external");
#endif
    }

#endif
 
#ifdef WIN32
    externalPath.assign("external/protocols");

#endif

    Http::loadProtocol(&languageParser);
    Https::loadProtocol(&languageParser);
    ControlProtocol::loadProtocol(&languageParser);

    /*! Load external protocols. */
    {
      string protocolsPath;
      protocolsPath.assign(externalPath);
      protocolsPath.append("/protocols");
      if(protocols.loadProtocols(protocolsPath.c_str(), &languageParser, 
                                 "myserver.xml", this))
      {
        ostringstream out;
        logPreparePrintError();
        out << languageParser.getValue("ERR_ERROR") << ": dynamic protocols";
        logWriteln(out.str().c_str());
        logEndPrintError();	  
      }
        
    }

    /*! Load external filters. */
    {
      string filtersPath;
      filtersPath.assign(externalPath);
      filtersPath.append("/filters");
      if(filters.loadFilters(filtersPath.c_str(), &languageParser, this))
      {
        ostringstream out;
        logPreparePrintError();
        out << languageParser.getValue("ERR_ERROR") << ": dynamic protocols";
        logWriteln(out.str().c_str());
        logEndPrintError();	  
      }
      if(filters.registerFilters(&filtersFactory))
      {
        ostringstream out;
        logPreparePrintError();
        out << languageParser.getValue("ERR_ERROR") << ": registering filters";
        logWriteln(out.str().c_str());
        logEndPrintError();	  
      }
    }
    
    logWriteln( languageParser.getValue("MSG_CREATET"));

    for(i=0; i<nStaticThreads; i++)
	  {
      ret = addThread(1);
      if(ret)
        return -1;
    }
    logWriteln(languageParser.getValue("MSG_THREADR"));

    /*! Return 1 if we had an allocation problem.  */
    if(getdefaultwd(path))
      return -1;
    /*!
     *Then we create here all the listens threads. 
     *Check that all the port used for listening have a listen thread.
     */
    logWriteln(languageParser.getValue("MSG_LISTENT"));
    createListenThreads();
    
    /*! 
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
      out << "uid: " << uid;
      logWriteln(out.str().c_str());
      autoRebootEnabled = 0;
    }
    /*!
     *Do a similar thing for the group identifier.
     */
    if(gid)
     {
       ostringstream out;
       if(Process::setgid(gid))
       {
         out << languageParser.getValue("ERR_ERROR") << ": setgid " << gid;
         logPreparePrintError();
         logWriteln(out.str().c_str());
         logEndPrintError();
       }	
       out << "gid: " << gid;
       logWriteln(out.str().c_str()); 
       autoRebootEnabled = 0;
     }
    
    logWriteln(languageParser.getValue("MSG_READY"));
    
    /*!
     *Print this message only if the log outputs to the console screen.
     */
    if(logManager.getType() == LogManager::TYPE_CONSOLE)
      logWriteln(languageParser.getValue("MSG_BREAK"));
    
    /*!
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
  int ret = 0;
  serverReady = 0;
  /*! Reset the toReboot flag. */
  toReboot = 0;

  /*! Do nothing if the reboot is disabled. */
  if(!autoRebootEnabled)
    return 0;
  /*! Do a beep if outputting to console. */
  if(logManager.getType() == LogManager::TYPE_CONSOLE)
  {
    char beep[]={static_cast<char>(0x7), '\0'};
    logManager.write(beep);
  }
  
  logWriteln("Rebooting...");
	if(mustEndServer)
		return 0;
	mustEndServer=1;
	ret = terminate();
  if(ret)
    return ret;
	mustEndServer=0;
  /*! Wait a bit before restart the server. */
	Thread::wait(5000);
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
  toReboot = 1;
}

/*!
 *Get the number of active listening threads.
 */
int Server::getListeningThreadCount()
{
	return listeningThreads;
}

/*!
 *Increase of one the number of active listening threads.
 */
void Server::increaseListeningThreadCount()
{
	connections_mutex_lock();
	++listeningThreads;
	connections_mutex_unlock();
}

/*!
 *Decrease of one the number of active listening threads.
 */
void Server::decreaseListeningThreadCount()
{
	connections_mutex_lock();
	--listeningThreads;
	connections_mutex_unlock();
}

/*!
 *Return the path to the mail configuration file.
 */
const char *Server::getMainConfFile()
{
  return main_configuration_file.c_str();
}

/*!
 *Return the path to the mail configuration file.
 */
const char *Server::getVhostConfFile()
{
  return vhost_configuration_file.c_str();
}

/*!
 *Return the path to the mail configuration file.
 */
const char *Server::getMIMEConfFile()
{
  return mime_configuration_file.c_str();
}

/*!
 *Get the first connection in the linked list.
 *Be sure to have locked connections access before.
 */
ConnectionPtr Server::getConnections()
{
  return connections;
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
 *Return the protocol_manager object.
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
  return languages_path.c_str();
}

/*!
 *Get the current language file.
 */
const char *Server::getLanguageFile()
{
  return languageFile.c_str();
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
	ThreadID ID;

  ClientsThread* newThread = new ClientsThread();

  newThread->setStatic(staticThread);

  if(newThread == 0)
    return -1;
  
  newThread->id=(u_long)(++currentThreadID);

  ret = Thread::create(&ID, &::startClientsThread, 
                                (void *)newThread);
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

  /*!
   *If everything was done correctly add the new thread to the linked list.
   */

	threads_mutex->lock();

  if(threads == 0)
  {
    threads = newThread;
    threads->next = 0;
  }
  else
  {
    newThread->next = threads;
    threads = newThread;
  }
  nThreads++;

	threads_mutex->unlock();

  return 0;
}

/*!
 *Remove a thread.
 *Return zero if a thread was removed.
 */
int Server::removeThread(u_long ID)
{
  int ret_code = 1;
  threads_mutex->lock();
  ClientsThread *thread = threads;
  ClientsThread *prev = 0;
  /*!
   *If there are no threads return an error.
   */
  if(threads == 0)
    return -1;
  while(thread)
  {
    if(thread->id == ID)
    {
      if(prev)
        prev->next = thread->next;
      else
        threads = thread->next;
      thread->stop();
      while(!thread->threadIsStopped)
      {
        Thread::wait(100);
      }
      nThreads--;
      delete thread;
      ret_code = 0;
      break;
    }

    prev = thread;
    thread = thread->next;
  }

	threads_mutex->unlock();
  return ret_code;

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
  ClientsThread* thread;
	threads_mutex->lock();
  thread = threads;
  while(thread)
  {
    if(!thread->isParsing())
      count++;
    thread = thread->next;
  }
	threads_mutex->unlock();
  return count;
}

/*!
 *Write a string to the log file and terminate the line.
 */
int Server::logWriteln(const char* str)
{
  if(!str)
    return 0;
  /*!
   *If the log receiver is not the console output a timestamp.
   */
  if(logManager.getType() != LogManager::TYPE_CONSOLE)
  {
    char time[38];
    int len;
    time[0]='[';
    getRFC822GMTTime(&time[1], 32);
    len = strlen(time);
    time[len+0]=']';
    time[len+1]=' ';
    time[len+2]='-';
    time[len+3]='-';
    time[len+4]=' ';
    time[len+5]='\0';
    if(logManager.write(time))
      return 1;
  }
  return logManager.writeln((char*)str);
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
