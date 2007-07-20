/*
MyServer
Copyright (C) 2007 The MyServer Team
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

#include "../stdafx.h"
#include "../include/sockets.h"
#include "../include/connection.h"
#include "../include/mutex.h"
#include "../include/event.h"
#include "../include/semaphore.h"
#include "../include/hash_map.h"
#include "../include/thread.h"

#include <list>
#include <queue>

using namespace std;

#define PRIORITY_CLASSES 3

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
		void reset(Socket* sock, u_short p){serverSocket = sock; port = p;}
		ListenerArg(Socket* sock, u_short p){reset(sock, p);}
		ListenerArg(){reset(0, 0);}
		ListenerArg(ListenerArg* l){serverSocket = l->serverSocket; port = l->port;}
	};

	struct DispatcherArg
	{
		bool terminated; 
		bool terminate;
		Mutex* mutex;
	};

  ConnectionsScheduler();
  ~ConnectionsScheduler();

	void addReadyConnection(ConnectionPtr, int = 1);
	void addWaitingConnection(ConnectionPtr, int = 1);

	ConnectionPtr getConnection();
	void release();
	void restart();
	void initialize();
	void listener(struct ListenerArg* );
	void removeListener(struct ListenerArg*);
	int getConnectionsNumber();
	void removeConnection(ConnectionPtr connection);
	void terminateConnections();

	void lockConnectionsList();
	void getConnections(list<ConnectionPtr> &out);
	void unlockConnectionsList();
private:
	event timeoutEv;
	ThreadID dispatchedThreadId;
	Semaphore *readySemaphore;
	Mutex readyMutex;
	Mutex connectionsMutex;
	Mutex eventsMutex;
	queue<ConnectionPtr> ready[PRIORITY_CLASSES];
	HashMap<SocketHandle, ConnectionPtr> connections;
	list<ListenerArg*> listeners;
	u_long currentPriority;
	u_long currentPriorityDone;
	DispatcherArg dispatcherArg;
};
                                   

#endif
