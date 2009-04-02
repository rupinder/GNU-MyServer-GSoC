/*
MyServer
Copyright (C) 2007, 2008, 2009 Free Software Foundation, Inc.
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
#include <include/server/server.h>
#include <include/base/file/files_utility.h>
#include <include/connections_scheduler/listen_threads.h>
#include <include/base/sync/semaphore.h>
#include <include/base/sync/event.h>

extern "C"
{
#ifdef WIN32
  //#include <Ws2tcpip.h>
#include <direct.h>
#endif
#ifndef WIN32
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

#include <set>
#include <algorithm>
#include <string>

using namespace std;

/*!
 *c'tor.
 */
ListenThreads::ListenThreads(ConnectionsScheduler* scheduler, Server* server)
{
  fastRebooting = false;
  committingFastReboot = false;
  this->scheduler = scheduler;
  this->server = server;
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

  if(fastRebooting)
  {
    frPortsToAdd.push_back(port);
    return 0;
  }

  /*
   *Create the server sockets:
   *one server socket for IPv4 and another one for IPv6
   */
  Socket *serverSocketIPv4 = new Socket();
  Socket *serverSocketIPv6 = NULL;

  SocketInformation* si = new SocketInformation;
  si->port = port;

  /*
   *Create the server socket.
   */
  try
  {
    if ( serverSocketIPv4 != NULL )
    {
      server->logWriteln(languageParser->getValue("MSG_SSOCKCREATE"));
      serverSocketIPv4->socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      if (serverSocketIPv4->getHandle() == (FileHandle)INVALID_SOCKET)
      {
        server->logWriteln(languageParser->getValue("ERR_OPENP"), MYSERVER_LOG_MSG_ERROR);
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

#ifndef WIN32
        /*
         *Under the unix environment the application needs some time before
         * create a new socket for the same address.
         *To avoid this behavior we use the current code.
         */
        if(serverSocketIPv4->setsockopt(SOL_SOCKET, SO_REUSEADDR,
                                        (const char *)&optvalReuseAddr,
                                        sizeof(optvalReuseAddr)) < 0)
        {
          server->logWriteln(languageParser->getValue("ERR_ERROR"), MYSERVER_LOG_MSG_ERROR);
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
            server->logWriteln(languageParser->getValue("ERR_BIND"), MYSERVER_LOG_MSG_ERROR);
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
      if ( serverSocketIPv6->getHandle() == (FileHandle)INVALID_SOCKET )
      {
        server->logWriteln(languageParser->getValue("ERR_OPENP"), MYSERVER_LOG_MSG_ERROR);
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
#ifndef WIN32
        /*
         *Under the unix environment the application needs some time before
         * create a new socket for the same address.
         *To avoid this behavior we use the current code.
         */
        if(serverSocketIPv6->setsockopt(SOL_SOCKET, SO_REUSEADDR,
                                        (const char *)&optvalReuseAddr,
                                        sizeof(optvalReuseAddr))<0)
        {
          server->logWriteln(languageParser->getValue("ERR_ERROR"), MYSERVER_LOG_MSG_ERROR);
          delete serverSocketIPv6;
          serverSocketIPv6 = NULL;
          //return 0;allow IPv6
        }

        if(serverSocketIPv6->setsockopt(IPPROTO_IPV6, IPV6_V6ONLY,
                                        (const char *)&optvalReuseAddr,
                                        sizeof(optvalReuseAddr)) < 0)
        {
          server->logWriteln(languageParser->getValue("ERR_ERROR"), MYSERVER_LOG_MSG_ERROR);
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
            server->logWriteln(languageParser->getValue("ERR_BIND"), MYSERVER_LOG_MSG_ERROR);
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
    {
      delete si;
      return 1;
    }

    /*
     *Set connections listen queque to max allowable.
     */
    server->logWriteln(languageParser->getValue("MSG_SLISTEN"));
    if (serverSocketIPv4 != NULL && serverSocketIPv4->listen(SOMAXCONN))
    {
      server->logWriteln(languageParser->getValue("ERR_LISTEN"), MYSERVER_LOG_MSG_ERROR);
      delete serverSocketIPv4;
      serverSocketIPv4 = NULL;
    }

    if (serverSocketIPv6 != NULL && serverSocketIPv6->listen(SOMAXCONN))
    {
      server->logWriteln(languageParser->getValue("ERR_LISTEN"), MYSERVER_LOG_MSG_ERROR);
      delete serverSocketIPv6;
      serverSocketIPv6 = NULL;
    }

    if ( serverSocketIPv4 == NULL && serverSocketIPv6 == NULL )
    {
      delete si;
      return 1;
    }

    portBuff << (u_int)port;

    listenPortMsg.assign(languageParser->getValue("MSG_LISTEN"));
    listenPortMsg.append(": ");
    listenPortMsg.append(portBuff.str());
  
    server->logWriteln(listenPortMsg.c_str());

    si->ipv4 = serverSocketIPv4;
    si->ipv6 = serverSocketIPv6;

    usedPorts.put(port, si);
    
    registerListener(si);

    return 0;
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
  return 1;
}

/*!
 *Register the sockets on the events listener.
 */
void ListenThreads::registerListener(SocketInformation* si)
{

    if(si->ipv4)
    {
      si->laIpv4.reset(si->ipv4, si->port, server);
      scheduler->listener(&(si->laIpv4));
    }

    if(si->ipv6)
    {
      si->laIpv6.reset(si->ipv6, si->port, server);
      scheduler->listener(&(si->laIpv6));
    }
}

/*!
 *Add a listening thread on a specific port.
 *\param port Port to listen on.
 */
void ListenThreads::addListeningThread(u_short port)
{
  if(!(fastRebooting || committingFastReboot))
    if(usedPorts.get(port))
      return;
  createServerAndListener(port);
}

/*!
 *Initialize the listen threads manager.
 *\param parser Xml data to use for error messages.
 */
int ListenThreads::initialize(XmlParser* parser)
{
  languageParser = parser;
  shutdownStatus = false;
  return 0;
}

/*!
 *Complete the fast reboot.
 */
void ListenThreads::commitFastReboot()
{
  /* Contains already present ports.  */
  set<u_short> presentPorts;

  /* Contains all the ports needed after the commit.  */
  set<u_short> newPorts;

  /* Contains ports already present and still needed.  */
  set<u_short> intersection;

  /* Contains ports already present and still needed.  */
  set<u_short> toRemove;

  /* Contains new ports to add.  */
  set<u_short> toAdd;

  fastRebooting = false;
  committingFastReboot = true;

  for(HashMap<u_short, SocketInformation*>::Iterator it = usedPorts.begin(); 
      it != usedPorts.end(); it++)
  {
    presentPorts.insert((*it)->port);
  }

  for(list<u_short>::iterator it = frPortsToAdd.begin(); it != frPortsToAdd.end(); it++)
  {
    newPorts.insert(*it);
  }

  /* intersection = intersection(presentsPorts, newPorts).  */
  set_intersection(presentPorts.begin(), presentPorts.end(), newPorts.begin(), newPorts.end(),
                   insert_iterator<set<u_short> >(intersection, intersection.begin()));


  /* toRemove = presentsPorts - newPorts.  */
  set_difference(presentPorts.begin(), presentPorts.end(), newPorts.begin(), newPorts.end(),
                 insert_iterator<set<u_short> >(toRemove, toRemove.begin()));


  /* toAdd = newPorts - presentsPorts.  */
  set_difference(newPorts.begin(), newPorts.end(), presentPorts.begin(), presentPorts.end(),
                 insert_iterator<set<u_short> >(toAdd, toAdd.begin()));



  /* Ports in intersections need only to be registered on the event listener.  */
  for(set<u_short>::iterator it = intersection.begin(); it != intersection.end(); it++)
  {
    registerListener(usedPorts.get(*it));
  }

  /* Create here the new ports.  */
  for(set<u_short>::iterator it = toAdd.begin(); it != toAdd.end(); it++)
   {
    createServerAndListener(*it);
  }

  /* Enqueue connections to remove to frPortsToRemove and destroy them with terminate.  */
  for(set<u_short>::iterator it = toRemove.begin(); it != toRemove.end(); it++)
  {
    SocketInformation* si;
    si = usedPorts.get(*it);
    frPortsToRemove.push_back(si);
    usedPorts.remove(si->port);
  }

  terminate();

  frPortsToAdd.clear();
  frPortsToRemove.clear();

  committingFastReboot = false;
}


/*!
 *Restore the previous situation without do anything.
 */
void ListenThreads::rollbackFastReboot()
{
  fastRebooting = false;
  committingFastReboot = false;
  
  for(HashMap<u_short, SocketInformation*>::Iterator it = usedPorts.begin(); 
      it != usedPorts.end(); it++)
  {
    registerListener(*it);
  }

  frPortsToAdd.clear();
  frPortsToRemove.clear();
}

/*!
 *Prepare the listen threads manager for a fast reboot.
 */
void ListenThreads::beginFastReboot()
{
  fastRebooting = true;
}


/*!
 *Unload the listen threads manager.
 */
int ListenThreads::terminate()
{
  char buffer[256];

  list <SocketInformation*> sockets;

  list <SocketInformation*>::iterator it;
  list <SocketInformation*>::iterator end;

  if(fastRebooting)
  {
    return 0;
  }
  else if(committingFastReboot)
  {
    it = frPortsToRemove.begin();
    end = frPortsToRemove.end();
  }
  else
  {
    for(HashMap<u_short, SocketInformation*>::Iterator i = usedPorts.begin(); 
        i != usedPorts.end(); i++)
      sockets.push_front(*i);

    it = sockets.begin();
    end = sockets.end();

    shutdown();
  }

  while(it != end)
  {
    for(int t = 0; t < 2; t++)
    {
      Socket* serverSocket;
      int err;

      if(t == 0)
        serverSocket = (*it)->ipv4;
      else
        serverSocket = (*it)->ipv6;
      
      if(!serverSocket)
        continue;

      serverSocket->shutdown(SD_BOTH);
      do
      {
        err = serverSocket->recv(buffer, 256, 0);
      }while(err != -1);

      serverSocket->close();
      delete serverSocket;
    }
    delete (*it);
    it++;
  }

  /* If it is not a fast reboot then clear everything.  */
  if(!(fastRebooting || committingFastReboot))
  {
    usedPorts.clear();
  }
  return 0;
}
