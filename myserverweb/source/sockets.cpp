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

#include "../stdafx.h"
#include "../include/utility.h"
#include "../include/sockets.h"
extern "C" {
#include <string.h>
#include <stdio.h>
#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#endif
}

#ifdef WIN32
#pragma comment(lib,"wsock32.lib")
#pragma comment(lib,"ws2_32.lib")
#endif

/*
*Source code to wrap the socket library to myServer project.
*/
int ms_startupSocketLib(u_short ver)
{
#ifdef WIN32	
	WSADATA wsaData;
	return WSAStartup(ver, &wsaData);
#endif
	return 0;
}

MYSERVER_SOCKET ms_socket(int af,int type,int protocol)
{
	return	(MYSERVER_SOCKET)socket(af,type,protocol);
}

int ms_bind(MYSERVER_SOCKET s,sockaddr* sa,int namelen)
{
#ifdef WIN32	
	return bind((SOCKET)s,sa,namelen);
#else
	return bind((int)s,sa,namelen);
#endif
}

int ms_listen(MYSERVER_SOCKET s,int max)
{
	return listen(s,max);
}

MYSERVER_SOCKET ms_accept(MYSERVER_SOCKET s,sockaddr* sa,int* sockaddrlen)
{
#ifdef WIN32
	return (MYSERVER_SOCKET)accept(s,sa,sockaddrlen);
#endif
#ifdef __linux__
	unsigned int Connect_Size = *sockaddrlen;
	return (MYSERVER_SOCKET)accept(s,sa,&Connect_Size);
#endif
}

int ms_closesocket(MYSERVER_SOCKET s)
{
#ifdef WIN32
	return closesocket(s);
#else
	return close(s);
#endif
}
MYSERVER_HOSTENT *ms_gethostbyaddr(char* addr,int len,int type)
{
#ifdef WIN32
	HOSTENT *he=gethostbyaddr(addr,len,type);
	return he;
#endif
#ifdef __linux__
	struct hostent * he=gethostbyaddr(addr,len,type);
	return he;
#endif
}
MYSERVER_HOSTENT *ms_gethostbyname(const char *hostname)
{	
	return (MYSERVER_HOSTENT *)gethostbyname(hostname);
}


int ms_shutdown(MYSERVER_SOCKET s,int how)
{
	return shutdown(s,how);
}

int	ms_setsockopt(MYSERVER_SOCKET s,int level,int optname,const char *optval,int optlen)
{
	return setsockopt(s,level, optname,optval,optlen);
}

int ms_send(MYSERVER_SOCKET s,const char* buffer,int len,int flags)
{
	return	send(s,buffer,len,flags);
}

int ms_ioctlsocket(MYSERVER_SOCKET s,long cmd,unsigned long* argp)
{
#ifdef WIN32
	return ioctlsocket(s,cmd,argp);
#else
	return ioctl(s,cmd,argp);
#endif
}

int ms_connect(MYSERVER_SOCKET s,sockaddr* sa,int na)
{
#ifdef WIN32
	return connect((SOCKET)s,sa,na);
#else
	return connect((int)s,sa,na);
#endif
}

int ms_recv(MYSERVER_SOCKET s,char* buffer,int len,int flags)
{
	int err;
	err=recv(s,buffer,len,flags);
#ifdef WIN32
	if(err==SOCKET_ERROR)
		return -1;
	else 
		return err;
#else
	return err;
#endif
}

u_long ms_bytesToRead(MYSERVER_SOCKET c)
{
	u_long nBytesToRead;
	ms_ioctlsocket(c,FIONREAD,&nBytesToRead);
	return nBytesToRead;
}
int ms_gethostname(char *name,int namelen)
{
	return gethostname(name,namelen);
}
