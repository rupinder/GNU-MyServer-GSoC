/*
MyServer
Copyright (C) 2002, 2003, 2004, 2005 The MyServer Team
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
#include "../include/server.h"

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

const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;

/*!
 *At startup the server instance is null.
 */
Server* Server::instance = 0;
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
    memset(serverName, 0, HOST_NAME_MAX+1);
    Socket::gethostname(serverName, HOST_NAME_MAX);

    buffer.assign(languageParser.getValue("MSG_GETNAME"));
    buffer.append(" ");
    buffer.append(serverName);
    logWriteln(buffer.c_str());

    /*!
     *find the IP addresses of the local machine.
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

    myserver_main_conf = File::getLastModTime(mainConfigurationFile->c_str());
    myserver_hosts_conf = File::getLastModTime(vhostConfigurationFile->c_str());
    myserver_mime_conf = File::getLastModTime(mimeConfigurationFile->c_str());

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
            File::getLastModTime(mainConfigurationFile->c_str());
          time_t myserver_hosts_conf_now=
            File::getLastModTime(vhostConfigurationFile->c_str());
          time_t myserver_mime_conf_now=
            File::getLastModTime(mimeConfigurationFile->c_str());

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
       *events before call it.
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
  /*!
   *We don't need to do anything.
   */
  ClientsThread *thread;
  ClientsThread *prev;
  int prev_threads_count;
  if(nThreads == nStaticThreads)
    return 0;

  threadsMutex->lock();
  thread = threads;
  prev = 0;
  prev_threads_count = nThreads;
  while(thread)
  {
    if(nThreads == nStaticThreads)
    {
      threadsMutex->unlock();
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
  threadsMutex->unlock();

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
 *for a port.
 */
int Server::createServerAndListener(u_short port)
{
	int optvalReuseAddr=1;
  ostringstream port_buff;
  string listen_port_msg;
	ThreadID threadId=0;
  listenThreadArgv* argv;
//	MYSERVER_SOCKADDRIN sock_inserverSocket;
  	/*!
	*Create the server sockets:
	*one server socket for IPv4 and another one for IPv6
	*/
	Socket *serverSocketIPv4 = new Socket();
	Socket *serverSocketIPv6 = NULL;
//	Socket *serverSocket;
	/*!
   *Create the server socket.
   */
  try
  {
	if ( serverSocketIPv4 != NULL )
	{
		logWriteln(languageParser.getValue("MSG_SSOCKCREATE"));
		serverSocketIPv4->socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if ( serverSocketIPv4->getHandle()==(SocketHandle)INVALID_SOCKET )
		{
			logPreparePrintError();
			logWriteln(languageParser.getValue("ERR_OPENP"));
			logEndPrintError();
			delete serverSocketIPv4;
			serverSocketIPv4 = NULL;
		}
		else
		{
    		logWriteln(languageParser.getValue("MSG_SSOCKRUN"));
			MYSERVER_SOCKADDR_STORAGE sock_inserverSocketIPv4 = { 0 };
			((sockaddr_in*)(&sock_inserverSocketIPv4))->sin_family=AF_INET;
			((sockaddr_in*)(&sock_inserverSocketIPv4))->sin_addr.s_addr=htonl(INADDR_LOOPBACK);//INADDR_ANY;
			((sockaddr_in*)(&sock_inserverSocketIPv4))->sin_port=htons((u_short)port);

#ifdef NOT_WIN
			/*!
			 *Under the unix environment the application needs some time before
			 * create a new socket for the same address.
			 *To avoid this behavior we use the current code.
			 */
			if(serverSocketIPv4->setsockopt(SOL_SOCKET, SO_REUSEADDR,
										(const char *)&optvalReuseAddr,
										sizeof(optvalReuseAddr))<0)
			{
			  logPreparePrintError();
			  logWriteln(languageParser.getValue("ERR_ERROR"));
			  logEndPrintError();
    			delete serverSocketIPv4;
    			serverSocketIPv4 = NULL;
			  //return 0; allow IPv6
			}
#endif
			/*!
			 *Bind the port.
			 */
			logWriteln(languageParser.getValue("MSG_BIND_PORT"));

			if(serverSocketIPv4 != NULL &&
			serverSocketIPv4->bind(&sock_inserverSocketIPv4,
								  sizeof(sockaddr_in))!=0)
			{
			  logPreparePrintError();
			  logWriteln(languageParser.getValue("ERR_BIND"));
			  logEndPrintError();
			delete serverSocketIPv4;
			serverSocketIPv4 = NULL;
			}
			else
			{
				logWriteln(languageParser.getValue("MSG_PORT_BOUND"));
			}
		}
	}

//    logWriteln(languageParser.getValue("MSG_SSOCKCREATE"));
//    serverSocket = new Socket();
//    if(!serverSocket)
//      return 0;
//    serverSocket->socket(AF_INET, SOCK_STREAM, 0);
//    if(serverSocket->getHandle()==(SocketHandle)INVALID_SOCKET)
//    {
//      logPreparePrintError();
//      logWriteln(languageParser.getValue("ERR_OPENP"));
//      logEndPrintError();
//      return 0;
//    }
//    logWriteln(languageParser.getValue("MSG_SSOCKRUN"));
//    sock_inserverSocket.sin_family=AF_INET;
//    sock_inserverSocket.sin_addr.s_addr=htonl(INADDR_ANY);
//    sock_inserverSocket.sin_port=htons(port);

#if ( IPv6_OS )
	serverSocketIPv6 = new Socket();
	
	if ( serverSocketIPv6 != NULL )
	{
		logWriteln(languageParser.getValue("MSG_SSOCKCREATE"));
		serverSocketIPv6->socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
		if ( serverSocketIPv6->getHandle()==(SocketHandle)INVALID_SOCKET )
		{
			logPreparePrintError();
			logWriteln(languageParser.getValue("ERR_OPENP"));
			logEndPrintError();
			delete serverSocketIPv6;
			serverSocketIPv6 = NULL;
		}
		else
		{
    		logWriteln(languageParser.getValue("MSG_SSOCKRUN"));
			MYSERVER_SOCKADDR_STORAGE sock_inserverSocketIPv6 = { 0 };
			((sockaddr_in6*)(&sock_inserverSocketIPv6))->sin6_family=AF_INET6;
			((sockaddr_in6*)(&sock_inserverSocketIPv6))->sin6_addr=in6addr_any;
			((sockaddr_in6*)(&sock_inserverSocketIPv6))->sin6_port=htons((u_short)port);
#ifdef NOT_WIN
			/*!
			 *Under the unix environment the application needs some time before
			 * create a new socket for the same address.
			 *To avoid this behavior we use the current code.
			 */
			if(serverSocketIPv6->setsockopt(SOL_SOCKET, SO_REUSEADDR,
										(const char *)&optvalReuseAddr,
										sizeof(optvalReuseAddr))<0)
			{
			  logPreparePrintError();
			  logWriteln(languageParser.getValue("ERR_ERROR"));
			  logEndPrintError();
			delete serverSocketIPv6;
			serverSocketIPv6 = NULL;
			  //return 0;allow IPv6
			}
			/*!TODO: Andu: please check out this comment and correct it if it's not true.
			 *Under the unix environment the IPv6 sockets also listen on IPv4 addreses
			 *so before binding assure we'll only use this socket for IPv6 communication
			 *because we already have a socket for IPv4 communication.
			 */
			if(serverSocketIPv6->setsockopt(IPPROTO_IPV6, IPV6_V6ONLY,
										(const char *)&optvalReuseAddr,
										sizeof(optvalReuseAddr))<0)
			{
			  logPreparePrintError();
			  logWriteln(languageParser.getValue("ERR_ERROR"));
			  logEndPrintError();
			delete serverSocketIPv6;
			serverSocketIPv6 = NULL;
			  //return 0;allow IPv6
			}
#endif
			/*!
			 *Bind the port.
			 */
			logWriteln(languageParser.getValue("MSG_BIND_PORT"));

			if(serverSocketIPv6 != NULL &&
			serverSocketIPv6->bind(&sock_inserverSocketIPv6,
								  sizeof(sockaddr_in6))!=0)
			{
			  logPreparePrintError();
			  logWriteln(languageParser.getValue("ERR_BIND"));
			  logEndPrintError();
			delete serverSocketIPv6;
			serverSocketIPv6 = NULL;
			}
			else
			{
				logWriteln(languageParser.getValue("MSG_PORT_BOUND"));
			}
		}
	}
#endif // IPv6_OS

    /*!
     *Set connections listen queque to max allowable.
     */
    logWriteln( languageParser.getValue("MSG_SLISTEN"));
//    if (serverSocket->listen(SOMAXCONN))
//    {
//      logPreparePrintError();
//      logWriteln(languageParser.getValue("ERR_LISTEN"));
//      logEndPrintError();
//      return 0;
//    }
	if ( serverSocketIPv4 == NULL && serverSocketIPv6 == NULL )
		return 0;
	if (serverSocketIPv4 != NULL && serverSocketIPv4->listen(SOMAXCONN))
	{
      logPreparePrintError();
      logWriteln(languageParser.getValue("ERR_LISTEN"));
      logEndPrintError();
      delete serverSocketIPv4;
      serverSocketIPv4 = NULL;
      //return 0;
	}
	if (serverSocketIPv6 != NULL && serverSocketIPv6->listen(SOMAXCONN))
	{
      logPreparePrintError();
      logWriteln(languageParser.getValue("ERR_LISTEN"));
      logEndPrintError();
      delete serverSocketIPv6;
      serverSocketIPv6 = NULL;
      //return 0;
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
//    argv->serverSocket=serverSocket;
	argv->serverSocketIPv4=serverSocketIPv4;
	argv->serverSocketIPv6=serverSocketIPv6;

    Thread::create(&threadId, &::listenServer,  (void *)(argv));
    return (threadId) ? 1 : 0 ;
  }
  catch( bad_alloc &ba)
  {
    ostringstream s;
    s << "Error: Bad allocation " << ba.what();
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
  list<Vhost*>::iterator i = vhostList->getVHostList()->begin();
	for( ;i != vhostList->getVHostList()->end(); i++)
	{
		int needThread=1;
		list<Vhost*>::iterator j = vhostList->getVHostList()->begin();

    /*! No port was specified. */
		if((*i)->getPort()==0)
			continue;

		for( ;j != i; j++)
		{
      /*!
       *If there is still a thread listening on the specified
       *port do not create the thread here.
       */
			if((*i)->getPort() == (*j)->getPort())
			{
				needThread=0;
				break;
      }
		}

		if(needThread)
		{
		  if(!createServerAndListener((*i)->getPort()))
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
//	Socket *serverSocket=argv->serverSocket;
	Socket *serverSocketIPv4 =argv->serverSocketIPv4;
	Socket *serverSocketIPv6 =argv->serverSocketIPv6;
	
	if ( serverSocketIPv4 == NULL && serverSocketIPv6 == NULL )
	   return 0;
	
	MYSERVER_SOCKADDRIN asock_in;
	int asock_inLen = 0;
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

//	ret = serverSocket->setNonBlocking(1);
	if ( serverSocketIPv4 != NULL )
  	   ret = serverSocketIPv4->setNonBlocking(1);
  	if ( serverSocketIPv6 != NULL )
  	   ret = serverSocketIPv6->setNonBlocking(1);

	Server::getInstance()->increaseListeningThreadCount();

  if(Server::getInstance()->getUid() | Server::getInstance()->getGid())
  {
    int uid = Server::getInstance()->getUid();
    int gid = Server::getInstance()->getGid();

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
      Vhost* vh = Server::getInstance()->vhostList->getVHostByNumber(i);
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
        Server::getInstance()->logPreparePrintError();
        Server::getInstance()->logWriteln(str);
        Server::getInstance()->logEndPrintError();
      }

      err = File::chown(vh->getWarningsLogFileName(), uid, gid);
      if(err)
      {
        string str;
        str.assign("Error changing owner for: ");
        str.append(vh->getWarningsLogFileName());
        Server::getInstance()->logPreparePrintError();
        Server::getInstance()->logWriteln(str);
        Server::getInstance()->logEndPrintError();
      }

    }

  }

  if(Server::getInstance()->getUid() && Process::setuid(Server::getInstance()->getUid()))
  {
    ostringstream out;
    out << Server::getInstance()->getLanguageParser()->getValue("ERR_ERROR")
        << ": setuid " << Server::getInstance()->getUid();
    Server::getInstance()->logPreparePrintError();
    Server::getInstance()->logWriteln(out.str().c_str());
    Server::getInstance()->logEndPrintError();
    Thread::terminate();
    return 0;

  }
  if(Server::getInstance()->getGid() && Process::setgid(Server::getInstance()->getGid()))
  {
    ostringstream out;
    out << Server::getInstance()->getLanguageParser()->getValue("ERR_ERROR")
        << ": setgid "  << Server::getInstance()->getGid();
    Server::getInstance()->logPreparePrintError();
    Server::getInstance()->logWriteln(out.str().c_str());
    Server::getInstance()->logEndPrintError();
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
		if ( serverSocketIPv4 != NULL )
		{
			if(serverSocketIPv4->dataOnRead(timeoutValue, 0) == 0 )
			{
				Thread::wait(10);
				//continue; maybe IPv6
			}
			else
			{
//				asock = serverSocket->accept((struct sockaddr*)&asock_in,
//										 &asock_inLen);
//				if(asock.getHandle()==0)
//					continue;
//				if(asock.getHandle()==(SocketHandle)INVALID_SOCKET)
//					continue;
//				asock.setServerSocket(serverSocket);
				asock_inLen = sizeof(sockaddr_in);
				asock = serverSocketIPv4->accept(&asock_in,
										 &asock_inLen);
				if(asock.getHandle() != 0 && asock.getHandle() != (SocketHandle)INVALID_SOCKET)
				{
 					 //Andu: do we need 'setServerSocket' anymore?
					//asock.setServerSocket(serverSocketIPv4);
					Server::getInstance()->addConnection(asock, &asock_in);
				}
			}
		}
		if ( serverSocketIPv6 != NULL )
		{
			if(serverSocketIPv6->dataOnRead(timeoutValue, 0) == 0 )
			{
				Thread::wait(10);
				continue;
			}
			asock_inLen = sizeof(sockaddr_in6);
			asock = serverSocketIPv6->accept(&asock_in,
									 &asock_inLen);
			if(asock.getHandle()==0)
				continue;
			if(asock.getHandle()==(SocketHandle)INVALID_SOCKET)
				continue;
			//Andu: do we need 'setServerSocket' anymore?
			//asock.setServerSocket(serverSocketIPv6);
			Server::getInstance()->addConnection(asock, &asock_in);
		}
	}

	/*!
   *When the flag mustEndServer is 1 end current thread and clean
   *the socket used for listening.
   */
//	serverSocket->shutdown( SD_BOTH);
	if ( serverSocketIPv4 != NULL )
	{
		serverSocketIPv4->shutdown(SD_BOTH);
		do
		{
			err=serverSocketIPv4->recv(buffer, 256, 0);
		}while(err!=-1);
		serverSocketIPv4->closesocket();
		delete serverSocketIPv4;
	}
  	argv->serverSocketIPv4 = NULL;

	if ( serverSocketIPv6 != NULL )
	{
		serverSocketIPv6->shutdown(SD_BOTH);
		do
		{
			err=serverSocketIPv6->recv(buffer, 256, 0);
		}while(err!=-1);
		serverSocketIPv6->closesocket();
		delete serverSocketIPv6;
	}
	argv->serverSocketIPv6 = NULL;
	
	Server::getInstance()->decreaseListeningThreadCount();
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

	while(Server::getInstance()->getListeningThreadCount())
	{
		Thread::wait(1000);
	}

	/* Clear the home directories data.  */
	homeDir.clear();

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
	freeHashedData();

	if(languagesPath)
		delete languagesPath;
	languagesPath = 0;

  if(languageFile)
		delete languageFile;
	languageFile = 0;

	delete vhostList;

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
  delete threadsMutex;

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
	return serverAdmin ? serverAdmin->c_str() : "";
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
  threadsMutex = new Mutex();

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
	if(serverAdmin)
		delete serverAdmin;
	serverAdmin = 0;
	autoRebootEnabled = 1;
	languagesPath = new string();
#ifndef WIN32
	/*!
   *Do not use the files in the directory /usr/share/myserver/languages
   *if exists a local directory.
   */
	if(File::fileExists("languages"))
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
    /*! Default PREFIX is /usr/. */
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
  /*!
   *Under an *nix environment look for .xml files in the following order.
   *1) myserver executable working directory
   *2) ~/.myserver/
   *3) /etc/myserver/
   *4) default files will be copied in myserver executable working
   */
	if(File::fileExists("myserver.xml"))
	{
		mainConfigurationFile->assign("myserver.xml");
	}
	else if(File::fileExists("~/.myserver/myserver.xml"))
	{
		mainConfigurationFile->assign("~/.myserver/myserver.xml");
	}
	else if(File::fileExists("/etc/myserver/myserver.xml"))
	{
		mainConfigurationFile->assign("/etc/myserver/myserver.xml");
	}
	else
#endif
	/*! If the myserver.xml files doesn't exist copy it from the default one. */
	if(!File::fileExists("myserver.xml"))
	{
    mainConfigurationFile->assign("myserver.xml");
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
		mainConfigurationFile->assign("myserver.xml");
  }
	configurationFileManager.open(mainConfigurationFile->c_str());


	data=configurationFileManager.getValue("VERBOSITY");
	if(data)
	{
		verbosity=(u_long)atoi(data);
	}
	data=configurationFileManager.getValue("LANGUAGE");
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
		if(serverAdmin == 0)
			serverAdmin = new string();
		serverAdmin->assign(data);
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
				old=hashedData.put(key, value);
        if(old)
        {
          delete old;
        }
      }
    }
  }

	configurationFileManager.close();

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
	if ( asock_in == NULL || 
	(asock_in->ss_family != AF_INET && asock_in->ss_family != AF_INET6) )
		return ret;
	/*!
   *ip is the string containing the address of the remote host
   *connecting to the server.
   *local_ip is the local addrress used by the connection.
   */
	//Andu: we can use MAX_IP_STRING_LEN only because we use NI_NUMERICHOST 
	//in getnameinfo call; Otherwise we should have used NI_MAXHOST
	char ip[MAX_IP_STRING_LEN];
	memset(ip,  0,  MAX_IP_STRING_LEN);
	char local_ip[MAX_IP_STRING_LEN];
	memset(local_ip,  0,  MAX_IP_STRING_LEN);
  /*! Remote port used by the client to open the connection. */
	u_short port;
  /*! Port used by the server to listen. */
	u_short myport;

	if( s.getHandle() == 0 )
		return 0;

  /*!
   *Do not accept this connection if a MAX_CONNECTIONS_TO_ACCEPT limit is defined.
   */
  if(maxConnectionsToAccept && (nConnections >= maxConnectionsToAccept))
    return 0;

  /*!
   *Create a new thread if there are not available threads and
   *we had not reach the limit.
   */
  if((nThreads < nMaxThreads) && (countAvailableThreads() == 0))
  {
    addThread(0);
  }

	MYSERVER_SOCKADDRIN  localsock_in = { 0 };
	int dim;
//	memset(&localsock_in, 0, sizeof(localsock_in));


#if ( IPv6_OS )
	if ( asock_in->ss_family == AF_INET )
	    ret = getnameinfo(reinterpret_cast<const sockaddr *>(asock_in), sizeof(sockaddr_in),
	        ip, MAX_IP_STRING_LEN, NULL, 0, NI_NUMERICHOST);
     else// AF_INET6
	    ret = getnameinfo(reinterpret_cast<const sockaddr *>(asock_in), sizeof(sockaddr_in6),
	        ip, MAX_IP_STRING_LEN, NULL, 0, NI_NUMERICHOST);
    if ( ret != 0 )
    {
	   return ret;
	 }
	if ( asock_in->ss_family == AF_INET )
		dim=sizeof(sockaddr_in);
	else
		dim=sizeof(sockaddr_in6);
	s.getsockname((MYSERVER_SOCKADDR*)&localsock_in, &dim);
	
	 if ( asock_in->ss_family == AF_INET )
	    ret = getnameinfo(reinterpret_cast<const sockaddr *>(&localsock_in), sizeof(sockaddr_in),
	        local_ip, MAX_IP_STRING_LEN, NULL, 0, NI_NUMERICHOST);
     else// AF_INET6
	    ret = getnameinfo(reinterpret_cast<const sockaddr *>(&localsock_in), sizeof(sockaddr_in6),
	        local_ip, MAX_IP_STRING_LEN, NULL, 0, NI_NUMERICHOST);
    if ( ret != 0 )
    {
	   return ret;
	 }
#else// !IPv6_OS
	dim=sizeof(localsock_in);
	s.getsockname((MYSERVER_SOCKADDR*)&localsock_in, &dim);
  // NOTE: inet_ntop supports IPv6
	strncpy(ip,  inet_ntoa(((sockaddr_in *)asock_in)->sin_addr),  MAX_IP_STRING_LEN);

  // NOTE: inet_ntop supports IPv6
	strncpy(local_ip,  inet_ntoa(((sockaddr_in *)&localsock_in)->sin_addr),  MAX_IP_STRING_LEN);
#endif//IPv6_OS

  /*! Port used by the client. */
  	if ( asock_in->ss_family == AF_INET )
  		port=ntohs(((sockaddr_in *)(asock_in))->sin_port);
	else// if ( asock_in->ss_family == AF_INET6 )
		port=ntohs(((sockaddr_in6 *)(asock_in))->sin6_port);
	//else not permited anymore
	//	return 0;

  /*! Port used by the server. */
  if ( localsock_in.ss_family == AF_INET )
	myport=ntohs(((sockaddr_in *)(&localsock_in))->sin_port);
	else// if ( localsock_in.ss_family == AF_INET6 )
		myport=ntohs(((sockaddr_in6 *)(&localsock_in))->sin6_port);
	//else not permited anymore
	//	return 0;
	
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
                                    MYSERVER_SOCKADDRIN* /*asock_in*/,
                                    char *ipAddr, char *localIpAddr,
                                    u_short port, u_short localPort, int /*id*/)
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
	new_connection->setTimeout( getTicks() );
	new_connection->setLocalPort(localPort);
	new_connection->setIpAddr(ipAddr);
	new_connection->setLocalIpAddr(localIpAddr);
	new_connection->host = Server::getInstance()->vhostList->getVHost(0,
																					                   localIpAddr,
                                                             localPort);

  /*! No vhost for the connection so bail. */
	if(new_connection->host == 0)
	{
		delete new_connection;
		return 0;
	}
	if(new_connection->host->getProtocol() > 1000	)
	{
		doSSLhandshake=1;
	}
	else if(new_connection->host->getProtocol() == PROTOCOL_UNKNOWN)
	{
		DynamicProtocol* dp;
    dp=Server::getInstance()->getDynProtocol(new_connection->host->getProtocolName());
		if(dp->getOptions() & PROTOCOL_USES_SSL)
			doSSLhandshake=1;
	}

	/*! Do the SSL handshake if required. */
	if(doSSLhandshake)
	{
		int ret =0;
#ifndef DO_NOT_USE_SSL
		SSL_CTX* ctx = new_connection->host->getSSLContext();
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
    Server::getInstance()->connections_mutex_lock();
    connection_ID++;
    new_connection->setID(connection_ID);
    new_connection->next = connections;
   	connections=new_connection;
    nConnections++;
    Server::getInstance()->connections_mutex_unlock();
  }
  catch(...)
  {
    logPreparePrintError();
    logWriteln("Error: adding connection to list");
    logEndPrintError();
    Server::getInstance()->connections_mutex_unlock();
  };

	/*!
   *If defined maxConnections and the number of active connections
   *is bigger than it say to the protocol that will parse the connection
   *to remove it from the active connections list.
   */
	if(maxConnections && (nConnections>maxConnections))
		new_connection->setToRemove(CONNECTION_REMOVE_OVERLOAD);

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
	return path ? path->c_str() : "";
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
		HashMap<string, string*>::Iterator end = hashedData.end();
		for (;it != end; it++)
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
    /*!
     *Under an *nix environment look for .xml files in the following order.
     *1) myserver executable working directory
     *2) ~/.myserver/
     *3) /etc/myserver/
     *4) default files will be copied in myserver executable working
     */
    if(File::fileExists("MIMEtypes.xml"))
    {
      mimeConfigurationFile->assign("MIMEtypes.xml");
    }
    else if(File::fileExists("~/.myserver/MIMEtypes.xml"))
    {
      mimeConfigurationFile->assign("~/.myserver/MIMEtypes.xml");
    }
    else if(File::fileExists("/etc/myserver/MIMEtypes.xml"))
    {
      mimeConfigurationFile->assign("/etc/myserver/MIMEtypes.xml");
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
        mimeConfigurationFile->assign("MIMEtypes.xml");
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

    /*! Load the MIME types. */
    logWriteln(languageParser.getValue("MSG_LOADMIME"));
		if(mimeManager)
			delete mimeManager;
		mimeManager = new MimeManager();
    if(int nMIMEtypes=mimeManager->loadXML(mimeConfigurationFile->c_str()))
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

		vhostConfigurationFile = new string();

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
      vhostConfigurationFile->assign("virtualhosts.xml");
    }
    else if(File::fileExists("~/.myserver/virtualhosts.xml"))
    {
      vhostConfigurationFile->assign("~/.myserver/virtualhosts.xml");
    }
    else if(File::fileExists("/etc/myserver/virtualhosts.xml"))
    {
      vhostConfigurationFile->assign("/etc/myserver/virtualhosts.xml");
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
        vhostConfigurationFile->assign("virtualhosts.xml");
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
        vhostConfigurationFile->assign("virtualhosts.xml");
      }
    vhostList = new VhostManager();
    if(vhostList == 0)
    {
      return -1;
    }
    /*! Load the virtual hosts configuration from the xml file. */
    vhostList->loadXMLConfigurationFile(vhostConfigurationFile->c_str(),
                                        this->getMaxLogFileSize());

		if(externalPath)
			delete externalPath;
		externalPath = new string();
#ifdef NOT_WIN
    if(File::fileExists("external"))
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

    Http::loadProtocol(&languageParser);
    Https::loadProtocol(&languageParser);
    ControlProtocol::loadProtocol(&languageParser);

		/* Load the home directories configuration.  */
		homeDir.load();

    /* Load external protocols.  */
    {
      string protocolsPath;
      protocolsPath.assign(*externalPath);
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
      filtersPath.assign(*externalPath);
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

		if(path == 0)
			path = new string();

    /*! Return 1 if we had an allocation problem.  */
    if(getdefaultwd(*path))
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
  return mainConfigurationFile
		? mainConfigurationFile->c_str() : 0;
}

/*!
 *Return the path to the mail configuration file.
 */
const char *Server::getVhostConfFile()
{
  return vhostConfigurationFile
		? vhostConfigurationFile->c_str() : 0;
}

/*!
 *Return the path to the mail configuration file.
 */
const char *Server::getMIMEConfFile()
{
  return mimeConfigurationFile
		? mimeConfigurationFile->c_str() : "";
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
	ThreadID ID;

  ClientsThread* newThread = new ClientsThread();

  if(newThread == 0)
    return -1;

  newThread->setStatic(staticThread);

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
	threadsMutex->lock();

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

	threadsMutex->unlock();

  return 0;
}

/*!
 *Remove a thread.
 *Return zero if a thread was removed.
 */
int Server::removeThread(u_long ID)
{
  int ret_code = 1;
  threadsMutex->lock();
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

	threadsMutex->unlock();
  return ret_code;

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
  ClientsThread* thread;
	threadsMutex->lock();
  thread = threads;
  while(thread)
  {
    if(!thread->isParsing())
      count++;
    thread = thread->next;
  }
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
