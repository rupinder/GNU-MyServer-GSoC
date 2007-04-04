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

#ifndef PROCESS_SERVER_MANAGER_H
#define PROCESS_SERVER_MANAGER_H

#include "../stdafx.h"
#include "../include/utility.h"
#include "../include/sockets.h"
#include "../include/connection.h"
#include "../include/stringutils.h"
#include "../include/thread.h"
#include "../include/mutex.h"
#include "../include/hash_map.h"
#include <string>

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
			SOCKET sock;
			unsigned int value;
		}DESCRIPTOR;

		Socket socket;
		string host;
		Process process; 
		u_short port;
		struct ServerDomain* sd;
		bool isLocal;

		void terminate()
		{
			if(isLocal)
			{
				socket.closesocket();
				process.terminateProcess();
			}
		}
	};

	struct ServerDomain
	{
		string domainName;
		HashMap<string, Server*> servers;
		void (*clear)(Server*);
		ServerDomain()
		{
			clear = 0;
		}
	};
	
	ServerDomain* createDomain(const char* name);
	ServerDomain* getDomain(const char* name);
	void clear();
	Server* getServer(const char* domain, const char* name);
	ProcessServerManager();
	~ProcessServerManager();
	Server* runAndAddServer(const char* domain, const char* name);
	Server* addRemoteServer(const char* domain, const char* name, 
													const char* host, u_short port);
	int connect(Socket* sock, Server* server);
	void setMaxServers(int max){maxServers = max;}
	int getMaxServers(){return maxServers;}
	void setInitialPort(u_short port){initialPort = port;}
	u_short getInitialPort(){return initialPort;}
	void removeServer(const char* domain, const char* name);
	void removeDomain(const char* domain);
	int domainServers(const char* domain);
private:
	int maxServers;
	u_short initialPort;
	int nServers;
  Mutex mutex;
	HashMap<string, ServerDomain*> domains;
  int runServer(Server* server, const char* path, int port = 0);
	void addServer(Server* server, const char* domain, const char* name);
};

#endif
