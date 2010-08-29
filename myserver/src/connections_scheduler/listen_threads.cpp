/*
  MyServer
  Copyright (C) 2007, 2008, 2009, 2010 Free Software Foundation, Inc.
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
#include <include/base/file/files_utility.h>
#include <include/connections_scheduler/listen_threads.h>
#include <include/base/sync/semaphore.h>
#include <include/base/sync/event.h>

#ifdef WIN32
# include <direct.h>
#endif
#ifndef WIN32
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


#include <set>
#include <algorithm>
#include <string>

using namespace std;

/*!
  C'tor.
 */
ListenThreads::ListenThreads (ConnectionsScheduler* scheduler, Server* server)
{
  fastRebooting = false;
  committingFastReboot = false;
  this->scheduler = scheduler;
  this->server = server;
}

/*!
  This function is used to create a socket server and a thread listener
  for a port.
 */
int ListenThreads::createServerAndListener (u_short port)
{
  string listenPortMsg;

  if (fastRebooting)
    {
      frPortsToAdd.push_back (port);
      return 0;
    }

  /*
    Create the server sockets:
    one server socket for IPv4 and another one for IPv6
   */
  Socket *serverSocketIPv4 = new Socket ();
  Socket *serverSocketIPv6 = NULL;

  SocketInformation* si = new SocketInformation;
  si->port = port;

  /*
    Create the server socket.
   */
  try
    {
      MYSERVER_SOCKADDR_STORAGE sockServerSocketIPv4 = { 0 };
      sockaddr_in* sai = (sockaddr_in *) &sockServerSocketIPv4;
      serverSocketIPv4->socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
      sai->sin_family = AF_INET;
      sai->sin_addr.s_addr = htonl (INADDR_ANY);
      sai->sin_port = htons ((u_short) port);
      serverSocketIPv4->reuseAddress (true);
      server->log (MYSERVER_LOG_MSG_INFO, _("Binding the port"));
      serverSocketIPv4->bind (&sockServerSocketIPv4, sizeof (sockaddr_in));
      server->log (MYSERVER_LOG_MSG_INFO, _("Port was bound"));

      serverSocketIPv4->listen (SOMAXCONN);
    }
  catch (exception & e)
    {
      server->log (MYSERVER_LOG_MSG_ERROR,
                   _E ("Error while creating the server socket : %e"), &e);
      delete serverSocketIPv4;
      serverSocketIPv4 = NULL;
    }

#if HAVE_IPV6
  serverSocketIPv6 = new Socket ();
  try
    {
      MYSERVER_SOCKADDR_STORAGE sockServerSocketIPv6 = { 0 };
      sockaddr_in6 *sai = (sockaddr_in6 *)(&sockServerSocketIPv6);
      serverSocketIPv6->socket (AF_INET6, SOCK_STREAM, IPPROTO_TCP);
      sai->sin6_family = AF_INET6;
      sai->sin6_addr = in6addr_any;
      sai->sin6_port = htons ((u_short) port);
      serverSocketIPv6->reuseAddress (true);

# ifndef WIN32
      int one = 1;
      serverSocketIPv6->setsockopt (IPPROTO_IPV6, IPV6_V6ONLY,
                                    (const char *) &one,
                                    sizeof (one));
# endif
      server->log (MYSERVER_LOG_MSG_INFO, _("Binding the port"));
      serverSocketIPv6->bind (&sockServerSocketIPv6,  sizeof (sockaddr_in6));
      server->log (MYSERVER_LOG_MSG_INFO, _("Port was bound"));
      serverSocketIPv6->listen (SOMAXCONN);
    }
  catch (exception & e)
    {
      server->log (MYSERVER_LOG_MSG_ERROR,
                   _("Error while creating the IPv6 server socket : %e"), &e);
      delete serverSocketIPv6;
      serverSocketIPv6 = NULL;
    }
#endif

  if (serverSocketIPv4 == NULL && serverSocketIPv6 == NULL)
    {
      delete si;
      return 1;
    }

  if (serverSocketIPv4 == NULL && serverSocketIPv6 == NULL)
    {
      delete si;
      return 1;
    }

  server->log (MYSERVER_LOG_MSG_INFO, _("Listening on the port: %i"), port);

  si->ipv4 = serverSocketIPv4;
  si->ipv6 = serverSocketIPv6;

  usedPorts.put (port, si);

  registerListener (si);

  return 0;
}

/*!
  Register the sockets on the events listener.
 */
void ListenThreads::registerListener (SocketInformation* si)
{
  if (si->ipv4)
    {
      si->laIpv4.reset (si->ipv4, si->port, server);
      scheduler->listener (&(si->laIpv4));
    }

  if (si->ipv6)
    {
      si->laIpv6.reset (si->ipv6, si->port, server);
      scheduler->listener (&(si->laIpv6));
    }
}

/*!
  Add a listening thread on a specific port.
  \param port Port to listen on.
 */
void ListenThreads::addListeningThread (u_short port)
{
  if (!(fastRebooting || committingFastReboot))
    if (usedPorts.get (port))
      return;
  createServerAndListener (port);
}

/*!
  Initialize the listen threads manager.
  \param parser Xml data to use for error messages.
 */
int ListenThreads::initialize ()
{
  shutdownStatus = false;
  return 0;
}

/*!
  Complete the fast reboot.
 */
void ListenThreads::commitFastReboot ()
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

  for (HashMap<u_short, SocketInformation*>::Iterator it = usedPorts.begin ();
      it != usedPorts.end (); it++)
    presentPorts.insert ((*it)->port);

  for (list<u_short>::iterator it = frPortsToAdd.begin ();
      it != frPortsToAdd.end (); it++)
    newPorts.insert (*it);

  /* intersection = intersection (presentsPorts, newPorts).  */
  set_intersection (presentPorts.begin (), presentPorts.end (),
                    newPorts.begin (), newPorts.end (),
                    insert_iterator<set<u_short> >(intersection,
                                                   intersection.begin ()));


  /* toRemove = presentsPorts - newPorts.  */
  set_difference (presentPorts.begin (), presentPorts.end (),
                  newPorts.begin (), newPorts.end (),
                  insert_iterator<set<u_short> >(toRemove, toRemove.begin ()));


  /* toAdd = newPorts - presentsPorts.  */
  set_difference (newPorts.begin (), newPorts.end (), presentPorts.begin (),
    presentPorts.end (), insert_iterator<set<u_short> >(toAdd, toAdd.begin ()));

  /* Ports in intersections need only to be registered on the event listener.  */
  for (set<u_short>::iterator it = intersection.begin ();
       it != intersection.end (); it++)
    registerListener (usedPorts.get (*it));

  /* Create here the new ports.  */
  for (set<u_short>::iterator it = toAdd.begin (); it != toAdd.end (); it++)
    createServerAndListener (*it);

  /* Enqueue connections to remove to frPortsToRemove and destroy them with terminate.  */
  for (set<u_short>::iterator it = toRemove.begin (); it != toRemove.end (); it++)
    {
      SocketInformation* si;
      si = usedPorts.get (*it);
      frPortsToRemove.push_back (si);
      usedPorts.remove (si->port);
    }

  terminate ();

  frPortsToAdd.clear ();
  frPortsToRemove.clear ();

  committingFastReboot = false;
}


/*!
  Restore the previous situation without do anything.
 */
void ListenThreads::rollbackFastReboot ()
{
  fastRebooting = false;
  committingFastReboot = false;

  for (HashMap<u_short, SocketInformation*>::Iterator it = usedPorts.begin ();
      it != usedPorts.end (); it++)
    registerListener (*it);

  frPortsToAdd.clear ();
  frPortsToRemove.clear ();
}

/*!
  Prepare the listen threads manager for a fast reboot.
 */
void ListenThreads::beginFastReboot ()
{
  fastRebooting = true;
}

/*!
  Unload the listen threads manager.
 */
int ListenThreads::terminate ()
{
  char buffer[256];

  list <SocketInformation*> sockets;

  list <SocketInformation*>::iterator it;
  list <SocketInformation*>::iterator end;

  if (fastRebooting)
    return 0;
  else if (committingFastReboot)
    {
      it = frPortsToRemove.begin ();
      end = frPortsToRemove.end ();
    }
  else
    {
      for (HashMap<u_short, SocketInformation*>::Iterator i = usedPorts.begin ();
           i != usedPorts.end (); i++)
      sockets.push_front (*i);
      it = sockets.begin ();
      end = sockets.end ();
      shutdown ();
    }

  while (it != end)
    {
      for (int t = 0; t < 2; t++)
        {
          Socket* serverSocket;
          int err;

          if (t == 0)
            serverSocket = (*it)->ipv4;
          else
            serverSocket = (*it)->ipv6;

          if (!serverSocket)
            continue;

          serverSocket->shutdown (SHUT_RDWR);
          for (;;)
            {
              try
                {
                  err = serverSocket->recv (buffer, 256, 0);
                }
              catch (...)
                {
                  break;
                }
            }

          serverSocket->close ();
          delete serverSocket;
        }
      delete (*it);
      it++;
    }

  /* If it is not a fast reboot then clear everything.  */
  if (!(fastRebooting || committingFastReboot))
    usedPorts.clear ();

  return 0;
}
