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
	char login[20];	
	char password[32];
	char nTries;	
	char ipAddr[32];
	char localIpAddr[32];
	u_short port;
	u_short localPort;
	MYSERVER_SOCKET socket;	
	u_long timeout;	
	CONNECTION* Next;
	void *host;
};
typedef CONNECTION*  volatile LPCONNECTION;
#endif
