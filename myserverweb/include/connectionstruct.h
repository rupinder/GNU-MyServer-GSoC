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
#ifndef CONNECTIONSTRUCT_H
#define CONNECTIONSTRUCT_H
#include "../include/sockets.h"
/*
*Here are listed all the protocol supported by the server.
*/
#define PROTOCOL_HTTP		0
#define PROTOCOL_FTP		1

typedef u_long CONNECTION_PROTOCOL;
/*
*This structure is used to describe a connection.
*/
struct CONNECTION
{
public:
	CONNECTION_PROTOCOL protocol;
	char login[20];	
	char password[32];
	char nTries;	
	char ipAddr[32];
	char localIpAddr[32];
	u_short port;
	MYSERVER_SOCKET socket;	
	u_long timeout;	
	CONNECTION* Next;
};
typedef CONNECTION*  volatile LPCONNECTION;
#endif
