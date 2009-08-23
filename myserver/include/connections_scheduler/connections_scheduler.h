/* -*- mode: c++ -*- */
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

#ifndef CONNECTIONS_SCHEDULER_H
#define CONNECTIONS_SCHEDULER_H

#include "stdafx.h"
#include <include/base/socket/socket.h>
#include <include/connection/connection.h>
#include <include/base/sync/mutex.h>
#include <include/base/sync/event.h>
#include <include/base/sync/semaphore.h>
#include <include/base/hash_map/hash_map.h>
#include <include/base/thread/thread.h>
#include <include/base/socket_pair/socket_pair.h>

#include <list>
#include <queue>

#include <event.h>

using namespace std;

#define PRIORITY_CLASSES 3


class Server;

class ConnectionsSchedulerVisitor
{
public:
  virtual int visitConnection(ConnectionPtr conn, void* param) = 0;
};

class ConnectionsScheduler
{
public:
  struct ListenerArg
  {
    Socket* serverSocket;
    u_short port;
    event ev;
    bool *terminate;
    Mutex* eventsMutex;
    ConnectionsScheduler *scheduler;
    Server* server;
    void reset(Socket* sock, u_short p, Server* ser){serverSocket = sock; port = p; server = ser;}
    ListenerArg(Socket* sock, u_short p, Server* server){reset(sock, p, server);}
    ListenerArg(){reset(NULL, 0, NULL);}
    ListenerArg(ListenerArg* l){serverSocket = l->serverSocket; port = l->port; server = l->server;}
  };

  struct DispatcherArg
  {
    bool terminated;
    bool terminate;
    Mutex* mutex;
    event loopEvent;
    Server* server;
    ConnectionsScheduler* scheduler;
    SocketPair socketPair;
  };

  ConnectionsScheduler(Server* server = NULL);
  ~ConnectionsScheduler();

  void addNewReadyConnection(ConnectionPtr);
  void addReadyConnection(ConnectionPtr);

  void addNewWaitingConnection(ConnectionPtr);
  void addWaitingConnection(ConnectionPtr);

  ConnectionPtr getConnection();
  void release();
  void restart();
  void initialize();
  void listener(struct ListenerArg* );
  void removeListener(struct ListenerArg*);
  u_long getConnectionsNumber();
  void removeConnection(ConnectionPtr connection);
  void terminateConnections();
  void getConnections(list<ConnectionPtr> &out);

  int accept(ConnectionsSchedulerVisitor*, void*);

  void registerConnectionID(ConnectionPtr);

  u_long getNumTotalConnections();

  void newData(short event, SocketHandle handle);

private:
  Server* server;
  void addWaitingConnectionImpl(ConnectionPtr, int lock);
  void addReadyConnectionImpl(ConnectionPtr);
  u_long nTotalConnections;
  ThreadID dispatchedThreadId;
  Semaphore *readySemaphore;
  Mutex readyMutex;
  Mutex connectionsMutex;
  Mutex eventsMutex;
  Mutex eventsSocketMutex;
  queue<ConnectionPtr> *ready;
  HashMap<SocketHandle, ConnectionPtr> connections;
  list<ListenerArg*> listeners;
  u_long currentPriority;
  u_long currentPriorityDone;
  DispatcherArg dispatcherArg;
  bool releasing;
};


#endif
