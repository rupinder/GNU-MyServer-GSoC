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

#ifndef PROCESS_SERVER_MANAGER_H
# define PROCESS_SERVER_MANAGER_H

# include "stdafx.h"
# include <include/base/utility.h>
# include <include/base/socket/socket.h>
# include <include/connection/connection.h>
# include <include/base/string/stringutils.h>
# include <include/base/thread/thread.h>
# include <include/base/sync/mutex.h>
# include <include/base/hash_map/hash_map.h>
# include <string>

# include <vector>
using namespace std;

class ProcessServerManager
{
public:
	struct Server
	{
		/*! Server executable path.  */
		string path;

		union
		{
			unsigned long fileHandle;
			SocketHandle sock;
			unsigned int value;
		}DESCRIPTOR;

		Socket socket;
		string host;
		Process process;
		u_short port;
		struct ServerDomain *sd;
		bool isLocal;

		void terminate()
		{
			if(isLocal)
			{
				socket.close();
				process.terminateProcess();
			}
		}
	};

	struct ServerDomain
	{
		string domainName;
		HashMap<string, vector<Server*> *> servers;
		void (*clear)(Server*);
		ServerDomain()
		{
			clear = 0;
		}
	};

	ServerDomain *createDomain(const char *name);
	ServerDomain *getDomain(const char *name);
	void clear();
	Server *getServer(const char *domain, const char *name, int seed = 0);
	ProcessServerManager();
	~ProcessServerManager();
	int connect(Socket *sock, Server *server);
	void setMaxServers(int max){maxServers = max;}
	int getMaxServers(){return maxServers;}
	void removeDomain(const char *domain);
	int domainServers(const char *domain);
	void load();
	Server *runAndAddServer(const char *domain, const char *name,
                          const char *chroot = NULL, int uid = 0,
                          int gid = 0, u_short port = 0);
	Server *addRemoteServer(const char *domain, const char *name,
													const char *host, u_short port);
private:
	int maxServers;
	int nServers;
  Mutex mutex;
	HashMap<string, ServerDomain*> domains;
  int runServer (Server *server, const char *path, u_short port = 0,
                 const char *chroot = NULL, int uid = 0, int gid = 0);
	void addServer (Server *server, const char *domain, const char *name);
};

#endif
