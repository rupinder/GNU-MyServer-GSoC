/*
*myServer
*Copyright (C) Rocky_10_Balboa
*This library is free software; you can redistribute it and/or
*modify it under the terms of the GNU Library General Public
*License as published by the Free Software Foundation; either
*version 2 of the License, or (at your option) any later version.

*This library is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*Library General Public License for more details.

*You should have received a copy of the GNU Library General Public
*License along with this library; if not, write to the
*Free Software Foundation, Inc., 59 Temple Place - Suite 330,
*Boston, MA  02111-1307, USA.
*/
#pragma once
#include "..\stdafx.h"

#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <uninstd.h>
#endif

typedef unsigned int MYSERVER_SOCKET;
typedef sockaddr_in MYSERVER_SOCKADDRIN;
int ms_startupSocketLib(WORD);
MYSERVER_SOCKET ms_socket(int,int,int);
int ms_bind(MYSERVER_SOCKET,sockaddr*,int);
int ms_listen(MYSERVER_SOCKET,int);
MYSERVER_SOCKET ms_accept(MYSERVER_SOCKET,sockaddr*,int*);
int ms_closesocket(MYSERVER_SOCKET);
int	ms_setsockopt(MYSERVER_SOCKET,int,int,const char*,int);
int ms_shutdown(MYSERVER_SOCKET s,int how);
int ms_ioctlsocket(MYSERVER_SOCKET,long,unsigned long*);
int ms_send(MYSERVER_SOCKET,const char*,int,int);
int ms_connect(MYSERVER_SOCKET,sockaddr*,int);
int ms_recv(MYSERVER_SOCKET,char*,int,int);
u_long bytesToRead(MYSERVER_SOCKET);