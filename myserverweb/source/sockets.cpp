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

#include "..\stdafx.h"
#include "..\include\utility.h"
#include "..\include\sockets.h"
#include <string.h>

/*
*Source code to wrap the socket library to myServer project.
*/

int ms_startupSocketLib(WORD ver)
{
#ifdef WIN32	
	WSADATA wsaData;
	return WSAStartup(ver, &wsaData);
#endif
}

MYSERVER_SOCKET ms_socket(int af,int type,int protocol)
{
#ifdef WIN32
	return	(MYSERVER_SOCKET)socket(af,type,protocol);
#endif
}

int ms_bind(MYSERVER_SOCKET s,sockaddr* sa,int namelen)
{
#ifdef WIN32	
	return bind((SOCKET)s,sa,namelen);
#endif
}

int ms_listen(MYSERVER_SOCKET s,int max)
{
#ifdef WIN32
	return listen(s,max);
#endif
}

MYSERVER_SOCKET ms_accept(MYSERVER_SOCKET s,sockaddr* sa,int* sockaddrlen)
{
#ifdef WIN32
	return (MYSERVER_SOCKET)accept(s,sa,sockaddrlen);
#endif
}

int ms_closesocket(MYSERVER_SOCKET s)
{
#ifdef WIN32
	return closesocket(s);
#endif
}

int ms_shutdown(MYSERVER_SOCKET s,int how)
{
#ifdef WIN32
	return shutdown(s,how);
#endif
}

int	ms_setsockopt(MYSERVER_SOCKET s,int level,int optname,const char *optval,int optlen)
{
#ifdef WIN32
	return setsockopt(s,level, optname,optval,optlen);
#endif
}

int ms_send(MYSERVER_SOCKET s,const char* buffer,int len,int flags)
{
#ifdef WIN32
	return	send(s,buffer,len,flags);
#endif
}

int ms_ioctlsocket(MYSERVER_SOCKET s,long cmd,unsigned long* argp)
{
#ifdef WIN32
	return ioctlsocket(s,cmd,argp);
#endif
}

int ms_connect(MYSERVER_SOCKET s,sockaddr* sa,int na)
{
#ifdef WIN32
	return connect((SOCKET)s,sa,na);
#endif
}

int ms_recv(MYSERVER_SOCKET s,char* buffer,int len,int flags)
{
#ifdef WIN32
	return recv(s,buffer,len,flags);
#endif
}

DWORD bytesToRead(MYSERVER_SOCKET c)
{
#ifdef WIN32
	DWORD nBytesToRead;
	ms_ioctlsocket(c,FIONREAD,&nBytesToRead);
	return nBytesToRead;
#endif
}