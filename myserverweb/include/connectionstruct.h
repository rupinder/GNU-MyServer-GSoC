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

#ifndef CONNECTIONSTRUCT_H
#define CONNECTIONSTRUCT_H
#include "../include/sockets.h"
#include "../include/utility.h"

/*!
*Here are listed all the protocol supported by the server.
*The protocols > 1000 use SSL.
*/
#define PROTOCOL_HTTP		0
#define PROTOCOL_HTTPS		1001
#define PROTOCOL_FTP		2
#define PROTOCOL_UNKNOWN	3

#define CONNECTION_REMOVE_OVERLOAD 1

typedef u_long CONNECTION_PROTOCOL;
/*!
*This structure is used to describe a connection.
*/
struct CONNECTION
{
public:
	const static int check_value_const=0x20;/*Const value for the CONNECTION structure to check integrity*/
	int parsing;/*The server is parsing this connection*/
	int check_value;/*Check if this is equal to check_value_const to ha a valid structure*/
	char login[20];/*Login name*/
	char password[32];/*Password used to log in*/
	char nTries;/*# of tries for an authorized login*/
	char ipAddr[MAX_IP_STRING_LEN];/*Remote IP address*/
	char localIpAddr[MAX_IP_STRING_LEN];/*Local IP used to connect to*/
	u_short port;/*Remote port used*/
	u_short localPort;/*Local port used to connect to*/
	MYSERVER_SOCKET socket;/*Connection socket*/
	u_long timeout;/*Current timeout for the connection*/
	CONNECTION* next;/*Next CONNECTION in linked list*/
	void *host;/*Pointer to an host structure*/
	int dataRead;/*Data size read in the buffersize2*/
	int toRemove;/*If nonzero the server is saying to the protocol to remove the connection.
	*Protocols can not consider this but is a good idea do it to avoid server overloads. 
	*Reasons to remove the connection are defined at the begin of this page.
	*/

	char connectionBuffer[KB(8)];/*!This buffer must be used only by the ClientsTHREAD class*/
};
typedef CONNECTION*  volatile LPCONNECTION;
#endif
