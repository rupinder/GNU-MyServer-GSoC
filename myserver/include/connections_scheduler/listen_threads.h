/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2007, 2009 Free Software Foundation, Inc.
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
#ifndef LISTEN_THREADS_H
#define LISTEN_THREADS_H

#include "stdafx.h"
#include <include/base/xml/xml_parser.h>
#include <include/base/sync/mutex.h>
#include <include/base/hash_map/hash_map.h>
#include <include/connections_scheduler/connections_scheduler.h>

class ListenThreads
{
public:
	void addListeningThread (u_short port);
	int initialize ();
	int terminate ();
	/*! Initialize the shutdown phase.  */
	void shutdown (){shutdownStatus = true;}
	/*! Is it shutdown phase?  */
	bool isShutdown (){return shutdownStatus;}
	void commitFastReboot ();
	void beginFastReboot ();
	void rollbackFastReboot ();
	ListenThreads (ConnectionsScheduler*, Server*);

private:
	struct SocketInformation
	{
		Socket *ipv4;
		Socket *ipv6;
		u_short port;
		ConnectionsScheduler::ListenerArg laIpv4;
		ConnectionsScheduler::ListenerArg laIpv6;
	};

	bool fastRebooting;
	bool committingFastReboot;
	list<u_short> frPortsToAdd;
	list <SocketInformation*> frPortsToRemove;

	bool shutdownStatus;
	HashMap<u_short, SocketInformation*> usedPorts;
	int createServerAndListener (u_short port);
	void registerListener (SocketInformation*);

  Server *server;
  ConnectionsScheduler *scheduler;
};

#endif
