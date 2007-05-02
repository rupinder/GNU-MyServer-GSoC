/*
MyServer
Copyright (C) 2007 The MyServer Team
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
#include "../include/server.h"
#include "../include/files_utility.h"
#include "../include/listen_threads.h"
#include "../include/thread.h"

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


#include <string>

using namespace std;

struct ListenThreadArgv
{
	u_long port;
	Socket *serverSocket;
	int SSLsocket;
	ListenThreads *lt;
};


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
	ListenThreadArgv *argv = (ListenThreadArgv*)params;
	Socket *serverSocket = argv->serverSocket;
	ListenThreads* lt = argv->lt;

	MYSERVER_SOCKADDRIN asockIn;
	int asockInLen = 0;
	Socket asock;
  int ret;

	int timeoutValue = 3;

#ifdef __linux__
	timeoutValue = 1;
#endif
#ifdef __HURD__
	timeoutValue = 5;
#endif

	if ( serverSocket == NULL)
	   return 0;

#ifdef NOT_WIN
	// Block SigTerm, SigInt, and SigPipe in threads
	sigset_t sigmask;
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGPIPE);
	sigaddset(&sigmask, SIGINT);
	sigaddset(&sigmask, SIGTERM);
	sigprocmask(SIG_SETMASK, &sigmask, NULL);
#endif

	/* Free the structure used to pass parameters to the new thread.  */
	delete argv;

	if (serverSocket != NULL )
		ret = serverSocket->setNonBlocking(1);

	Server::getInstance()->setProcessPermissions();

	lt->increaseCount();

	while(!Server::getInstance()->stopServer())
	{
		/*
     *Accept connections.
     *A new connection will be added using the Server::addConnection function.
     */
		if(serverSocket->dataOnRead(timeoutValue, 0) == 1 )
		{
			asockInLen = sizeof(sockaddr_in);
			asock = serverSocket->accept(&asockIn,
																	 &asockInLen);
			if(asock.getHandle() != 0 &&
				 asock.getHandle() != (SocketHandle)INVALID_SOCKET)
			{
				Server::getInstance()->addConnection(asock, &asockIn);
			}
		}
	}

	/*!
   *When the flag mustEndServer is 1 end current thread and clean
   *the socket used for listening.
   */
	serverSocket->shutdown(SD_BOTH);
	do
	{
		err = serverSocket->recv(buffer, 256, 0);
	}while(err != -1);

	serverSocket->closesocket();
	delete serverSocket;

	lt->decreaseCount();


	/*
   *Automatically free the current thread.
   */
	Thread::terminate();
	return 0;
}


/*!
 *This function is used to create a socket server and a thread listener
 *for a port.
 */
int ListenThreads::createServerAndListener(u_short port)
{
	int optvalReuseAddr = 1;
  ostringstream portBuff;
  string listenPortMsg;
	ThreadID threadIdIPv4 = 0;
	ThreadID threadIdIPv6 = 0;
  ListenThreadArgv* argv;
	Server* server = Server::getInstance();
	/*
	 *Create the server sockets:
	 *one server socket for IPv4 and another one for IPv6
	 */
	Socket *serverSocketIPv4 = new Socket();
	Socket *serverSocketIPv6 = NULL;

	/*
   *Create the server socket.
   */
  try
	{
		if ( serverSocketIPv4 != NULL )
		{
			server->logWriteln(languageParser->getValue("MSG_SSOCKCREATE"));
			serverSocketIPv4->socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (serverSocketIPv4->getHandle() == (SocketHandle)INVALID_SOCKET)
			{
				server->logPreparePrintError();
				server->logWriteln(languageParser->getValue("ERR_OPENP"));
				server->logEndPrintError();
				delete serverSocketIPv4;
				serverSocketIPv4 = NULL;
			}
			else
			{
				MYSERVER_SOCKADDR_STORAGE sockServerSocketIPv4 = { 0 };
				server->logWriteln(languageParser->getValue("MSG_SSOCKRUN"));
				((sockaddr_in*)(&sockServerSocketIPv4))->sin_family = AF_INET;
				((sockaddr_in*)(&sockServerSocketIPv4))->sin_addr.s_addr = 
					htonl(INADDR_ANY);
				((sockaddr_in*)(&sockServerSocketIPv4))->sin_port = 
					htons((u_short)port);

#ifdef NOT_WIN
				/*
				 *Under the unix environment the application needs some time before
				 * create a new socket for the same address.
				 *To avoid this behavior we use the current code.
				 */
				if(serverSocketIPv4->setsockopt(SOL_SOCKET, SO_REUSEADDR,
																				(const char *)&optvalReuseAddr,
																				sizeof(optvalReuseAddr)) < 0)
				{
					server->logPreparePrintError();
					server->logWriteln(languageParser->getValue("ERR_ERROR"));
					server->logEndPrintError();
					delete serverSocketIPv4;
					serverSocketIPv4 = NULL;
					//return 0; allow IPv6
				}
#endif
				if( serverSocketIPv4 != NULL )
				{
					/*
					 *Bind the port.
					 */
					server->logWriteln(languageParser->getValue("MSG_BIND_PORT"));
					
					if (serverSocketIPv4->bind(&sockServerSocketIPv4,
																		 sizeof(sockaddr_in)) != 0)
					{
						server->logPreparePrintError();
						server->logWriteln(languageParser->getValue("ERR_BIND"));
						server->logEndPrintError();
						delete serverSocketIPv4;
						serverSocketIPv4 = NULL;
					}
					else
						server->logWriteln(languageParser->getValue("MSG_PORT_BOUND"));
				}
			}
		}

#if ( HAVE_IPV6 )
		serverSocketIPv6 = new Socket();

		if (serverSocketIPv6 != NULL)
		{
			server->logWriteln(languageParser->getValue("MSG_SSOCKCREATE"));
			serverSocketIPv6->socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
			if ( serverSocketIPv6->getHandle() == (SocketHandle)INVALID_SOCKET )
			{
				server->logPreparePrintError();
				server->logWriteln(languageParser->getValue("ERR_OPENP"));
				server->logEndPrintError();
				delete serverSocketIPv6;
				serverSocketIPv6 = NULL;
			}
			else
			{
				MYSERVER_SOCKADDR_STORAGE sockServerSocketIPv6 = { 0 };
				server->logWriteln(languageParser->getValue("MSG_SSOCKRUN"));
				((sockaddr_in6*)(&sockServerSocketIPv6))->sin6_family = AF_INET6;
				((sockaddr_in6*)(&sockServerSocketIPv6))->sin6_addr = in6addr_any;
				((sockaddr_in6*)(&sockServerSocketIPv6))->sin6_port = 
					htons((u_short)port);
#ifdef NOT_WIN
				/*
				 *Under the unix environment the application needs some time before
				 * create a new socket for the same address.
				 *To avoid this behavior we use the current code.
				 */
				if(serverSocketIPv6->setsockopt(SOL_SOCKET, SO_REUSEADDR,
																				(const char *)&optvalReuseAddr,
																				sizeof(optvalReuseAddr))<0)
				{
					server->logPreparePrintError();
					server->logWriteln(languageParser->getValue("ERR_ERROR"));
					server->logEndPrintError();
					delete serverSocketIPv6;
					serverSocketIPv6 = NULL;
					//return 0;allow IPv6
				}

				if(serverSocketIPv6->setsockopt(IPPROTO_IPV6, IPV6_V6ONLY,
																				(const char *)&optvalReuseAddr,
																				sizeof(optvalReuseAddr)) < 0)
				{
					server->logPreparePrintError();
					server->logWriteln(languageParser->getValue("ERR_ERROR"));
					server->logEndPrintError();
					delete serverSocketIPv6;
					serverSocketIPv6 = NULL;
					//return 0;allow IPv6
				}
#endif
				if(serverSocketIPv6 != NULL )
				{
					/*
					 *Bind the port.
					 */
					server->logWriteln(languageParser->getValue("MSG_BIND_PORT"));
					
					if ( serverSocketIPv6->bind(&sockServerSocketIPv6,
																			sizeof(sockaddr_in6)) != 0)
					{
						server->logPreparePrintError();
						server->logWriteln(languageParser->getValue("ERR_BIND"));
						server->logEndPrintError();
						delete serverSocketIPv6;
						serverSocketIPv6 = NULL;
					}
					else
						server->logWriteln(languageParser->getValue("MSG_PORT_BOUND"));

				}
			}
		}
#endif // HAVE_IPV6
		
		if ( serverSocketIPv4 == NULL && serverSocketIPv6 == NULL )
			return 0;

	/*
	 *Set connections listen queque to max allowable.
	 */
		server->logWriteln(languageParser->getValue("MSG_SLISTEN"));
		if (serverSocketIPv4 != NULL && serverSocketIPv4->listen(SOMAXCONN))
		{
      server->logPreparePrintError();
      server->logWriteln(languageParser->getValue("ERR_LISTEN"));
      server->logEndPrintError();
      delete serverSocketIPv4;
      serverSocketIPv4 = NULL;
		}

		if (serverSocketIPv6 != NULL && serverSocketIPv6->listen(SOMAXCONN))
		{
      server->logPreparePrintError();
      server->logWriteln(languageParser->getValue("ERR_LISTEN"));
      server->logEndPrintError();
      delete serverSocketIPv6;
      serverSocketIPv6 = NULL;
		}

		if ( serverSocketIPv4 == NULL && serverSocketIPv6 == NULL )
			return 0;

		portBuff << (u_int)port;

		listenPortMsg.assign(languageParser->getValue("MSG_LISTEN"));
		listenPortMsg.append(": ");
		listenPortMsg.append(portBuff.str());
	
		server->logWriteln(listenPortMsg.c_str());

		server->logWriteln(languageParser->getValue("MSG_LISTENT"));

		/*
		 *Create the listen threads.
		 */
		if(serverSocketIPv4)
		{
			argv = new ListenThreadArgv;
			argv->port = port;
			argv->lt = this;
			argv->serverSocket = serverSocketIPv4;
			Thread::create(&threadIdIPv4, &::listenServer,  (void *)(argv));
		
			if(!threadIdIPv4)
				delete argv;
		}

		if(serverSocketIPv6)
		{
			argv = new ListenThreadArgv;
			argv->port = port;
			argv->lt = this;
			argv->serverSocket = serverSocketIPv6;
			Thread::create(&threadIdIPv6, &::listenServer,  (void *)(argv));

			if(!threadIdIPv6)
				delete argv;
		}

		if(threadIdIPv4 || threadIdIPv6)
			server->logWriteln(languageParser->getValue("MSG_LISTENTR"));

    return threadIdIPv4 || threadIdIPv6;
  }
  catch( bad_alloc &ba)
  {
    ostringstream s;
    s << "Error: Bad allocation " << ba.what();
    server->logWriteln(s.str().c_str());
  }
  catch( exception &e)
  {
    ostringstream s;
    s << "Error :" << e.what();
    server->logWriteln(s.str().c_str());
  };
  return 0;
}

/*!
 *Increase the listening threads counter.
 */
void ListenThreads::increaseCount()
{
	countMutex.lock();
	count++;
	countMutex.unlock();
}

/*!
 *Decrease the listening threads counter.
 */
void ListenThreads::decreaseCount()
{
	countMutex.lock();
	count--;
	countMutex.unlock();
}


/*!
 *Add a listening thread on a specific port.
 *\param port Port to listen on.
 */
void ListenThreads::addListeningThread(u_short port)
{
	if(usedPorts.get(port))
		return;
	usedPorts.put(port, true);
	createServerAndListener(port);
}

/*!
 *Initialize the listen threads manager.
 *\param parser Xml data to use for error messages.
 */
int ListenThreads::initialize(XmlParser* parser)
{
	countMutex.init();
	languageParser = parser;
	usedPorts.clear();
	return 0;
}

/*!
 *Unload the listen threads manager.
 */
int ListenThreads::terminate()
{
	usedPorts.clear();
	countMutex.destroy();
	return 0;
}
