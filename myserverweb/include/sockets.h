/*
*myServer
*Copyright (C) 2002 Giuseppe Scrivano
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
#include <netdb.h>
#include <stdio.h>
}
#endif

typedef unsigned int MYSERVER_SOCKET;
typedef struct sockaddr_in MYSERVER_SOCKADDRIN;
typedef struct sockaddr MYSERVER_SOCKADDR;
typedef struct hostent MYSERVER_HOSTENT;
int ms_startupSocketLib(u_short);
MYSERVER_HOSTENT *ms_gethostbyname(const char*);
MYSERVER_SOCKET ms_socket(int,int,int);
int ms_bind(MYSERVER_SOCKET,MYSERVER_SOCKADDR*,int);
int ms_listen(MYSERVER_SOCKET,int);
MYSERVER_SOCKET ms_accept(MYSERVER_SOCKET,MYSERVER_SOCKADDR*,int*);
int ms_closesocket(MYSERVER_SOCKET);
int ms_setsockopt(MYSERVER_SOCKET,int,int,const char*,int);
int ms_shutdown(MYSERVER_SOCKET s,int how);
int ms_ioctlsocket(MYSERVER_SOCKET,long,unsigned long*);
int ms_send(MYSERVER_SOCKET,const char*,int,int);
int ms_connect(MYSERVER_SOCKET,MYSERVER_SOCKADDR*,int);
int ms_recv(MYSERVER_SOCKET,char*,int,int);
int ms_gethostname(char*,int);
u_long ms_bytesToRead(MYSERVER_SOCKET);
int ms_getsockname(MYSERVER_SOCKET,MYSERVER_SOCKADDR*,int*);
#endif
