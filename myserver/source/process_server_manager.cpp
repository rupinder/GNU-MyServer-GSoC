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
#include "../include/process_server_manager.h"
#include "../include/processes.h"
#include <string>
#include <sstream>
using namespace std;

ProcessServerManager::ProcessServerManager()
{
	mutex.init();
	nServers = 0;
	initialPort = 3333;
	maxServers = 25;
}

ProcessServerManager::~ProcessServerManager()
{
	mutex.destroy();
}

/*!
 *Get a servers process domain by its name.
 *\param name The domain name.
 */
ProcessServerManager::ServerDomain* 
ProcessServerManager::getDomain(const char* name)
{
	ServerDomain* ret;
	mutex.lock();
	ret = domains.get(name);
	mutex.unlock();
	return ret;
}

/*!
 *Create a new servers process domain by its name and return it.
 *\param name The domain name.
 */
ProcessServerManager::ServerDomain* 
ProcessServerManager::createDomain(const char* name)
{
	ServerDomain* ret;

	mutex.lock();

	ret = domains.get(name);
	if(!ret)
	{
		string str(name);
		ret = new ServerDomain();
		domains.put(str, ret);
	}

	mutex.unlock();

	return ret;
}

/*!
 *Clear the used memory.
 */
void ProcessServerManager::clear()
{
	HashMap<string, ServerDomain*>::Iterator it;

	mutex.lock();

	it = domains.begin();

	for(;it != domains.end(); it++)
	{
		ServerDomain *sd = *it;
		HashMap<string, Server*>::Iterator server = sd->servers.begin();

		for(;server != sd->servers.end(); server++)
		{
			if((*it)->clear)
			{
				(*it)->clear(*server);
			}
			(*server)->terminate();
			delete (*server);
		}
		delete (*it);
	}
	domains.clear();
	mutex.unlock();
}

/*!
 *Get a server is running by its domain and name.
 *\param domain The domain name.
 *\param name The server name name.
 */
ProcessServerManager::Server* 
ProcessServerManager::getServer(const char* domain, const char* name)
{
	ServerDomain* sd;
	Server* s;
	mutex.lock();
	sd = domains.get(domain);
	if(sd)
		s = sd->servers.get(name);
	else
		s = 0;
	
	if(s && !s->process.isProcessAlive())
	{
		s->socket.closesocket();
		s->process.terminateProcess();
		if(runServer(s, s->path.c_str(), s->port))
		{
			sd->servers.remove(name);
			delete s;
			s = 0;
		}
	}

	mutex.unlock();
	return s;
}

/*!
 *Add a server to the manager.
 *\param server The server object.
 *\param domain The server's domain.
 *\param name The server's name.
 */
void ProcessServerManager::addServer(ProcessServerManager::Server* server, 
																		 const char* domain, const char* name)
{
	ServerDomain* sd = createDomain(domain);
	Server* old;
	string str(name);

	mutex.lock();

	old = sd->servers.put(str, server);

	if(old)
	{
		if(old->isLocal)
		{
			old->socket.closesocket();
			old->process.terminateProcess();
		}
		delete old;
	}
	mutex.unlock();
}

/*!
 *Add a remote server.
 *\param domain The server's domain name.
 *\param name The server name.
 *\param host The host name to connect to.
 *\param port The port number to use for the connection.
 */
ProcessServerManager::Server* 
ProcessServerManager::addRemoteServer(const char* domain, const char* name, 
																			const char* host, u_short port)
{
	Server* s = new Server;
	s->path.assign(name);
	s->host.assign(host);
	s->port = port;
	s->isLocal = false;

	addServer(s, domain, name);
	return s;
}

/*!
 *Run and add a server to the collection.
 *\param domain The server's domain.
 *\param path The path to the executable.
 */
ProcessServerManager::Server* 
ProcessServerManager::runAndAddServer(const char* domain, 
																			const char* path)
{
	Server* server = new Server;
	if(runServer(server, path))
	{
		delete server;
		return 0;
	}
	addServer(server, domain, path);
	return server;
}

/*!
 *Run a new server.
 *\param server The server object.
 *\param path The path to the executable.
 *\param port The listening port.
 */
int ProcessServerManager::runServer(ProcessServerManager::Server* server, 
																		const char* path, int port)
{
  StartProcInfo spi;
  MYSERVER_SOCKADDRIN serverSockAddrIn;
  server->host.assign("localhost");
	server->isLocal = true;

	if(nServers > maxServers)
		return 1;

	if(port)
		server->port = port;
	else
		server->port = (initialPort + nServers++);



  server->socket.socket(AF_INET,SOCK_STREAM,0);
  if(server->socket.getHandle() != (SocketHandle)INVALID_SOCKET)
  {
	  ((sockaddr_in *)(&serverSockAddrIn))->sin_family = AF_INET;

	  ((sockaddr_in *)(&serverSockAddrIn))->sin_addr.s_addr = 
			htonl(INADDR_LOOPBACK);
	  ((sockaddr_in *)(&serverSockAddrIn))->sin_port = 
			htons(server->port);
	  if ( !server->socket.bind(&serverSockAddrIn,
															sizeof(sockaddr_in)) )
	  {
		  if( !server->socket.listen(SOMAXCONN) )
			{
				string tmpCgiPath;
				string moreArg;
				int subString = path[0] == '"';
				int i;
				int len = strlen(path);
				for(i = 1; i < len; i++)
				{
					if(!subString && path[i] == ' ')
						break;
					if(path[i] == '"' && path[i - 1] != '\\')
						subString = !subString;
				}

				if(i < len)
				{
					string tmpString(path);
					int begin = tmpString[0] == '"' ? 1 : 0;
					int end   = tmpString[i] == '"' ? i + 1 : i ;
					tmpCgiPath.assign(tmpString.substr(begin, end));
					moreArg.assign(tmpString.substr(i, len));  
				}
				else
				{
					int begin = (path[0] == '"') ? 1 : 0;
					int end   = (path[len] == '"') ? len-1 : len;
					tmpCgiPath.assign(&path[begin], end-begin);
					moreArg.assign("");
				}


			  server->DESCRIPTOR.fileHandle = server->socket.getHandle();
			  spi.envString = 0;
			  spi.stdIn = (FileHandle)server->DESCRIPTOR.fileHandle;
			  spi.cmd.assign(tmpCgiPath);
				spi.arg.assign(moreArg);
			  spi.cmdLine.assign(path);
			  server->path.assign(path);

			  spi.stdOut = spi.stdError =(FileHandle) -1;
			  if(server->process.execConcurrentProcess(&spi) == -1)
				{
					server->socket.closesocket();
					//return 1; allow IPv6
			  }
			  
		  }
		  else
		  {
				server->socket.closesocket();
				//return 1; allow IPv6
		}
	  }
	  else
		{
			server->socket.closesocket();
			//return 1; allow IPv6
		}
  }
  else
  {
#if HAVE_IPV6 && 0
		server->socket.socket(AF_INET6, SOCK_STREAM, 0);
		if(server->socket.getHandle() != (SocketHandle)INVALID_SOCKET)
		{
			((sockaddr_in6 *)(&serverSockAddrIn))->sin6_family = AF_INET6;

			/*! The FastCGI server accepts connections only by the localhost.  */
			((sockaddr_in6 *)(&serverSockAddrIn))->sin6_addr=in6addr_any;
			((sockaddr_in6 *)(&serverSockAddrIn))->sin6_port=htons(server->port);
			if(server->socket.bind(&serverSockAddrIn,
														 sizeof(sockaddr_in6)))
			{
				server->socket.closesocket();
				return 1;
			}
			if(server->socket.listen(SOMAXCONN))
		  {
				server->socket.closesocket();
				return 1;
			}
			server->DESCRIPTOR.fileHandle = server->socket.getHandle();
			spi.envString = 0;
			spi.stdIn = (FileHandle)server->DESCRIPTOR.fileHandle;
			spi.cmd.assign(path);
			spi.cmdLine.assign(path);
			server->path.assign(path);

			spi.stdOut = spi.stdError =(FileHandle) -1;

			if(server->process.execConcurrentProcess(&spi) == -1)
			{
				server->socket.closesocket();
				return 1;
			}
		}
#endif
	}
  return 0;
}

/*!
 *Get a client socket in the fCGI context structure
 *\param sock The socket to connect.
 *\param server The server to connect to.
 */
int ProcessServerManager::connect(Socket* sock,
																	ProcessServerManager::Server* server )
{
	MYSERVER_SOCKADDRIN serverSock = { 0 };
	socklen_t nLength = sizeof(MYSERVER_SOCKADDRIN);
	getsockname(server->socket.getHandle(), (sockaddr *)&serverSock, &nLength);
	if(serverSock.ss_family == AF_INET)
	{
		/*! Try to create the socket.  */
		if(sock->socket(AF_INET, SOCK_STREAM, 0) == -1)
			return -1;
		
		/*! If the socket was created try to connect.  */
		if(sock->connect(server->host.c_str(), server->port) == -1)
		{
			sock->closesocket();
			return -1;
		}

	}
#if ( HAVE_IPV6 && false )/* IPv6 communication not implemented yet by php.  */
	else if ( serverSock.ss_family == AF_INET6 )
	{
		/*! Try to create the socket.  */
		if(sock->socket(AF_INET6, SOCK_STREAM, 0) == -1)
			return -1;
		/*! If the socket was created try to connect.  */
		if(sock->connect(server->host, server->port) == -1)
		{
			sock->closesocket();
			return -1;
		}
	}
#endif // HAVE_IPV6
	sock->setNonBlocking(1);

	return 1;
}
