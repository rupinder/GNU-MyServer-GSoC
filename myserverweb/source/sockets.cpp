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
#else
	return 0;
#endif
}
MYSERVER_SOCKET_HANDLE MYSERVER_SOCKET::ms_getHandle()
{
	return socketHandle;
}
int MYSERVER_SOCKET::ms_setHandle(MYSERVER_SOCKET_HANDLE h)
{
	socketHandle=h;
	return 1;
}
int MYSERVER_SOCKET::operator==(MYSERVER_SOCKET s)
{
	return socketHandle==s.socketHandle;
}
int MYSERVER_SOCKET::operator=(MYSERVER_SOCKET s)
{
	socketHandle=s.socketHandle;
	return 1;
}
int MYSERVER_SOCKET::ms_socket(int af,int type,int protocol)
{
	socketHandle=socket(af,type,protocol);
	return	(int)socketHandle;
}

int MYSERVER_SOCKET::ms_bind(MYSERVER_SOCKADDR* sa,int namelen)
{
#ifdef WIN32	
	return bind((SOCKET)socketHandle,sa,namelen);
#endif
#ifdef __linux__
	return bind((int)socketHandle,sa,namelen);
#endif
}

int MYSERVER_SOCKET::ms_listen(int max)
{
#ifdef WIN32
	return listen(socketHandle,max);
#endif
#ifdef __linux__
	return listen((int)socketHandle,max);
#endif
}

MYSERVER_SOCKET MYSERVER_SOCKET::ms_accept(MYSERVER_SOCKADDR* sa,int* sockaddrlen)
{
#ifdef WIN32
	MYSERVER_SOCKET_HANDLE h=accept(socketHandle,sa,sockaddrlen);
	MYSERVER_SOCKET s;
	s.ms_setHandle(h);
	return s;
#endif
#ifdef __linux__
	unsigned int Connect_Size = *sockaddrlen;
	int as = accept((int)socketHandle,sa,&Connect_Size);
	MYSERVER_SOCKET s;
	s.ms_setHandle(as);
	return s;
#endif
}

int MYSERVER_SOCKET::ms_closesocket()
{
#ifdef WIN32
	return closesocket(socketHandle);
#endif
#ifdef __linux__
	return close((int)socketHandle);
#endif
}
MYSERVER_HOSTENT *MYSERVER_SOCKET::ms_gethostbyaddr(char* addr,int len,int type)
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
MYSERVER_HOSTENT *MYSERVER_SOCKET::ms_gethostbyname(const char *hostname)
{	
	return (MYSERVER_HOSTENT *)gethostbyname(hostname);
}


int MYSERVER_SOCKET::ms_shutdown(int how)
{
#ifdef WIN32
	return shutdown(socketHandle,how);
#endif
#ifdef __linux__
	return shutdown((int)socketHandle,how);
#endif
}

int	MYSERVER_SOCKET::ms_setsockopt(int level,int optname,const char *optval,int optlen)
{
	return setsockopt(socketHandle,level, optname,optval,optlen);
}

int MYSERVER_SOCKET::ms_send(const char* buffer,int len,int flags)
{
#ifdef WIN32
	return	send(socketHandle,buffer,len,flags);
#endif
#ifdef __linux__
	return	send((int)socketHandle,buffer,len,flags);
#endif
}

int MYSERVER_SOCKET::ms_ioctlsocket(long cmd,unsigned long* argp)
{
#ifdef WIN32
	return ioctlsocket(socketHandle,cmd,argp);
#endif
#ifdef __linux__
	int int_argp = 0;
	int ret = ioctl((int)socketHandle,cmd,&int_argp);
	*argp = int_argp;
	return ret;

#endif
}

int MYSERVER_SOCKET::ms_connect(MYSERVER_SOCKADDR* sa,int na)
{
#ifdef WIN32
	return connect((SOCKET)socketHandle,sa,na);
#endif
#ifdef __linux__
	return connect((int)socketHandle,sa,na);
#endif
}

int MYSERVER_SOCKET::ms_recv(char* buffer,int len,int flags)
{
	int err;
#ifdef WIN32
	err=recv(socketHandle,buffer,len,flags);
	if(err==SOCKET_ERROR)
		return -1;
	else 
		return err;
#endif
#ifdef __linux__
	err=recv((int)socketHandle,buffer,len,flags);
	if(err == 0)
		err = -1;
	return err;
#endif
}

u_long MYSERVER_SOCKET::ms_bytesToRead()
{
	u_long nBytesToRead;
	ms_ioctlsocket(FIONREAD,&nBytesToRead);
	return nBytesToRead;
}
int MYSERVER_SOCKET::ms_gethostname(char *name,int namelen)
{
	return gethostname(name,namelen);
}
int MYSERVER_SOCKET::ms_getsockname(MYSERVER_SOCKADDR *ad,int *namelen)
{
#ifdef WIN32
	return getsockname(socketHandle,ad,namelen);
#endif
#ifdef __linux__
	unsigned int len = *namelen;
	int ret = getsockname((int)socketHandle,ad,&len);
	*namelen = len;
	return ret;
#endif
}
