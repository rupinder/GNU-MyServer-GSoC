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
*Source code to wrap the socket library
*/

int ms_startupSocketLib(WORD ver)
{
	WSADATA wsaData;
	return WSAStartup(ver, &wsaData);
}

MYSERVER_SOCKET ms_socket(int af,int type,int protocol)
{
	return	(MYSERVER_SOCKET)socket(af,type,protocol);
}

int ms_bind(MYSERVER_SOCKET s,sockaddr* sa,int namelen)
{
	return bind((SOCKET)s,sa,namelen);

}

int ms_listen(MYSERVER_SOCKET s,int max)
{
	return listen(s,max);

}

MYSERVER_SOCKET ms_accept(MYSERVER_SOCKET s,sockaddr* sa,int* sockaddrlen)
{
	return (MYSERVER_SOCKET)accept(s,sa,sockaddrlen);
}

int ms_closesocket(MYSERVER_SOCKET s)
{
	return closesocket(s);
}

int	ms_setsockopt(MYSERVER_SOCKET s,int level,int optname,const char *optval,int optlen)
{
	return setsockopt(s,level, optname,optval,optlen);
}

int ms_shutdown(MYSERVER_SOCKET s,int how)
{
	return shutdown(s,how);
}

int ms_send(MYSERVER_SOCKET s,const char* buffer,int len,int flags)
{
	return	send(s,buffer,len,flags);
}

int ms_ioctlsocket(MYSERVER_SOCKET s,long cmd,unsigned long* argp)
{
	return ioctlsocket(s,cmd,argp);
}

int ms_recv(MYSERVER_SOCKET s,char* buffer,int len,int flags)
{
	return recv(s,buffer,len,flags);
}