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

#include "../include/connections_scheduler.h"
#include "../include/server.h"

static void do_nothing(int fd, short ev, void *arg)
{
	static timeval tv = {0, 500000};
	if(ev == EV_TIMEOUT)
	{
		event_add((event*) arg, &tv);
	}
}

#ifdef WIN32
static unsigned int __stdcall dispatcher(void* p)
#else
static void* dispatcher(void* p)
#endif
{
	ConnectionsScheduler::DispatcherArg *da = (ConnectionsScheduler::DispatcherArg*)p;

	da->mutex->lock();
	if(!da->terminated)
	{
 		da->mutex->unlock();
		return 0;
	}
	da->terminated = false;
	da->terminate = false;
	da->mutex->unlock();


	while(!da->terminate)
  {		
		int res;

		res = event_dispatch();

		if(res == 1)
		{
			Thread::wait(1);
		}
	}
	
	da->mutex->lock();
	da->terminated = true;
	da->mutex->unlock();

	return 0;
}

static void new_data_handler(int fd, short event, void *arg)
{
	if(event == EV_TIMEOUT)
	{
		Server::getInstance()->getConnectionsScheduler()->lockConnectionsList();
		
		if(!((ConnectionPtr)arg)->isParsing())
			Server::getInstance()->deleteConnection((ConnectionPtr)arg, 0, 0);

		Server::getInstance()->getConnectionsScheduler()->unlockConnectionsList();
	}
	else if(event == EV_READ)
	{
		Server::getInstance()->getConnectionsScheduler()->lockConnectionsList();

		Server::getInstance()->getConnectionsScheduler()->addReadyConnection((ConnectionPtr)arg, 0);

		Server::getInstance()->getConnectionsScheduler()->unlockConnectionsList();
	}
}

static void listener_handler(int fd, short event, void *arg)
{
	static timeval tv = {5, 0};
	ConnectionsScheduler::ListenerArg* s = (ConnectionsScheduler::ListenerArg*)arg;

	if(event == EV_TIMEOUT)
	{
		event_add (&(s->ev), &tv);
	}
	else if(event == EV_READ)
	{
		MYSERVER_SOCKADDRIN asockIn;
		int asockInLen = 0;
		Socket asock;

		asockInLen = sizeof(sockaddr_in);
		asock = s->serverSocket->accept(&asockIn, &asockInLen);
		if(asock.getHandle() != 0 &&
			 asock.getHandle() != (SocketHandle)INVALID_SOCKET)
		{
			Server::getInstance()->addConnection(asock, &asockIn);
		}

		event_add (&(s->ev), &tv);
	}
}

/*!
 *Add a listener socket to the event queue.
 *This is used to renew the event after the listener thread is notified.
 *
 *\param sock Listening socket.
 *\param la Structure containing an Event to be notified on new data.
 */
void ConnectionsScheduler::listener(ConnectionsScheduler::ListenerArg *la)
{
	ConnectionsScheduler::ListenerArg *arg = new ConnectionsScheduler::ListenerArg(la);
	static timeval tv = {3, 0};

	event_set(&(arg->ev), la->serverSocket->getHandle(), EV_READ | EV_TIMEOUT, 
						listener_handler, arg);

	arg->terminate = &dispatcherArg.terminate;
	arg->scheduler = this;
	arg->eventsMutex = &eventsMutex;
	la->serverSocket->setNonBlocking(1);

	listeners.push_front(arg);

	eventsMutex.lock();
	event_add(&(arg->ev), &tv);
	eventsMutex.unlock();
}

/*!
 *Remove a listener thread from the list.
 */
void ConnectionsScheduler::removeListener(ConnectionsScheduler::ListenerArg* la)
{
	eventsMutex.lock();
   	event_del(&(la->ev));
	listeners.remove(la);
	eventsMutex.unlock();
}

/*!
 *C'tor.
 */
ConnectionsScheduler::ConnectionsScheduler()
{
	readyMutex.init();
	eventsMutex.init();
	connectionsMutex.init();
	readySemaphore = new Semaphore(0);
	currentPriority = 0;
	currentPriorityDone = 0;
}


/*!
 *Restart the scheduler.
 */
void ConnectionsScheduler::restart()
{
	readyMutex.init();
	connectionsMutex.init();
	eventsMutex.init();
	listeners.clear();

	if(readySemaphore)
		delete readySemaphore;

	readySemaphore = new Semaphore(0);

	initialize();
}

/*!
 *Static initialization.
 */
void ConnectionsScheduler::initialize()
{
	static timeval tv = {1, 0};

    event_init();

	event_set(&timeoutEv, 0, EV_TIMEOUT, do_nothing, &timeoutEv);
	event_add(&timeoutEv, &tv);


	dispatcherArg.terminated = true;
	dispatcherArg.mutex = &eventsMutex;

	if(Thread::create(&dispatchedThreadId, dispatcher, &dispatcherArg))
	 {
		 Server::getInstance()->logLockAccess();
		 Server::getInstance()->logPreparePrintError();
		 Server::getInstance()->logWriteln("Error initializing dispatcher thread.");
		 Server::getInstance()->logEndPrintError();
		 Server::getInstance()->logUnlockAccess();
		 dispatchedThreadId = 0;
	 }
}

/*!
 *D'tor.
 */
ConnectionsScheduler::~ConnectionsScheduler()
{
	readyMutex.destroy();
	eventsMutex.destroy();
	connectionsMutex.destroy();
	delete readySemaphore;
}

/*!
 *Add a connection to ready connections queue.
 */
void ConnectionsScheduler::addReadyConnection(ConnectionPtr c, int doLock)
{
	u_long priority = std::max((u_long)(PRIORITY_CLASSES - 1), c->getPriority());

	if(doLock)
		connectionsMutex.lock();
	c->setParsing(1);

	if(doLock)
		connectionsMutex.unlock();

	readyMutex.lock();
	ready[priority].push(c);
	readyMutex.unlock();

	Server::getInstance()->checkThreadsNumber();
	readySemaphore->unlock();
}

/*!
 *Add a connection to waiting connections queue.
 */
void ConnectionsScheduler::addWaitingConnection(ConnectionPtr c, int doLock)
{
	static timeval tv = {10, 0};
	SocketHandle handle = c->socket->getHandle();

	tv.tv_sec = Server::getInstance()->getTimeout() / 1000;

	connectionsMutex.lock();
	c->setParsing(0);
	connections.put(handle, c);
	connectionsMutex.unlock();

	event_set(c->getEvent(), handle, EV_READ | EV_TIMEOUT, new_data_handler, c);

	if(doLock)
		eventsMutex.lock();

	event_add(c->getEvent(), &tv);

	if(doLock)
		eventsMutex.unlock();
}

/*!
 *Get a connection from the active connections queue.
 */
ConnectionPtr ConnectionsScheduler::getConnection()
{
	u_long start = getTicks();
	readySemaphore->lock();
	readyMutex.lock();

	for(int i = 0; i < PRIORITY_CLASSES; i++)
	{
		while(currentPriorityDone <= currentPriority && ready[currentPriority].size())
		{
			ConnectionPtr ret = ready[currentPriority].front();
			ready[currentPriority].pop();

			readyMutex.unlock();
			return ret;
		}

		currentPriority = (currentPriority + 1) % PRIORITY_CLASSES;
		currentPriorityDone = 0;
	}

	readyMutex.unlock();

	return 0;
}

/*!
 *Release all the blocking calls.
 */
void ConnectionsScheduler::release()
{
	dispatcherArg.terminate = true;

	event_loopexit(NULL);
	for(u_long i = 0; i < Server::getInstance()->getNumThreads()*5; i++)
	{
		readySemaphore->unlock();
    }
	if(dispatchedThreadId)
		Thread::join(dispatchedThreadId);

	eventsMutex.lock();

	list<ListenerArg*>::iterator it = listeners.begin();

	while(it != listeners.end())
	{
		event_del(&((*it)->ev));
		delete (*it);
		it++;
	}
	listeners.clear();
	
	eventsMutex.unlock();
}

/*!
 *Fullfill a list with all the connections.
 *\param out A list that will receive all the connections alive on the
 *server.
 */
void ConnectionsScheduler::getConnections(list<ConnectionPtr> &out)
{
	out.clear();

	HashMap<SocketHandle, ConnectionPtr>::Iterator it = connections.begin();
	for(; it != connections.end(); it++)
		out.push_back(*it);
}

/*!
 *Release the connection access.
 */
void ConnectionsScheduler::unlockConnectionsList()
{
	connectionsMutex.unlock();
}

/*!
 *Acquire the access to the connection mutex.
 */
void ConnectionsScheduler::lockConnectionsList()
{
	connectionsMutex.lock();
}

/*!
 *Get the alive connections number.
 */
int ConnectionsScheduler::getConnectionsNumber()
{
	return connections.size();
}

/*!
 *Remove a connection from the connections set.
 */
void ConnectionsScheduler::removeConnection(ConnectionPtr connection)
{
	connections.remove(connection->socket->getHandle());
}

/*!
 *Terminate an active connection.
 */
void ConnectionsScheduler::terminateConnections()
{
	int i;
  try
  {
		connectionsMutex.lock();

		HashMap<SocketHandle, ConnectionPtr>::Iterator it = connections.begin();
		for(; it != connections.end(); it++)
    {
			(*it)->socket->closesocket();
    }
  }
  catch(...)
  {
		connectionsMutex.unlock();
    throw;
  };

	connections.clear();

	connectionsMutex.unlock();

	readyMutex.lock();
	for(i = 0; i < PRIORITY_CLASSES; i++)
		while(ready[i].size())
			ready[i].pop();

	readyMutex.unlock();
}
