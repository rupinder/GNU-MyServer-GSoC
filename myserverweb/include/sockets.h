/*
*MyServer
*Copyright (C) 2002 The MyServer Team
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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef SOCKETS_H
#define SOCKETS_H

#include "../stdafx.h"

#ifdef WIN32
#ifndef SOCKETLIBINCLUDED
#include <winsock2.h>
#define SOCKETLIBINCLUDED
#endif
#endif
#ifdef __linux__
extern "C" {
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
}
#define SOCKET int
#define INVALID_SOCKET -1
#endif

typedef unsigned int MYSERVER_SOCKET_HANDLE;
typedef struct sockaddr_in MYSERVER_SOCKADDRIN;
typedef struct sockaddr MYSERVER_SOCKADDR;
typedef struct hostent MYSERVER_HOSTENT;
int startupSocketLib(u_short);
class MYSERVER_SOCKET
{
private:
	MYSERVER_SOCKET_HANDLE socketHandle;
public:
	MYSERVER_SOCKET_HANDLE getHandle();
	int setHandle(MYSERVER_SOCKET_HANDLE);
	static MYSERVER_HOSTENT *gethostbyaddr(char* addr,int len,int type);
	static MYSERVER_HOSTENT *gethostbyname(const char*);
	static int gethostname(char*,int);
	int socket(int,int,int);
	int bind(MYSERVER_SOCKADDR*,int);
	int listen(int);
	MYSERVER_SOCKET();
	MYSERVER_SOCKET(MYSERVER_SOCKET_HANDLE);
	MYSERVER_SOCKET accept(MYSERVER_SOCKADDR*,int*);
	int closesocket();
	int setsockopt(int,int,const char*,int);
	int shutdown(int how);
	int ioctlsocket(long,unsigned long*);
	int send(const char*,int,int);
	int connect(MYSERVER_SOCKADDR*,int);
	int recv(char*,int,int);
	u_long bytesToRead();
	int operator==(MYSERVER_SOCKET);
	int operator=(MYSERVER_SOCKET);
	int getsockname(MYSERVER_SOCKADDR*,int*);
};
#endif
