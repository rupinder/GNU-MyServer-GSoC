/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
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
*Protocols with an ID > 1000 use SSL.
*/
#define PROTOCOL_UNKNOWN	0
#define PROTOCOL_HTTP		1
#define PROTOCOL_HTTPS		1001
#define PROTOCOL_FTP		2

/*! Remove the connection due a high server load.  */
#define CONNECTION_REMOVE_OVERLOAD 1

typedef u_long CONNECTION_PROTOCOL;

/*!
 *This structure is used to describe a connection.
 */
struct CONNECTION
{
public:
	/*! Pointer to the thread struct that is using the CONNECTION.  */
	void *thread;
	
	/*! Const value for the CONNECTION structure to check integrity.  */
	const static int check_value_const=0x20;
	
	/*! The server is parsing this connection.  */
	int parsing;
	
	/*! Check if this is equal to check_value_const to ha a valid structure.  */
	int check_value;
	
	/*! Login name.  */
	char login[20];
	
	/*! Password used to log in.  */
	char password[32];
	
	/*! # of tries for an authorized login. */
	char nTries;
	
	/*! Remote IP address.  */
	char ipAddr[MAX_IP_STRING_LEN];
	
	/*! Local IP used to connect to.  */
	char localIpAddr[MAX_IP_STRING_LEN];
	
	/*! Remote port used.  */
	u_short port;
	
	/*! Local port used to connect to.  */
	u_short localPort;
	
	/*! Connection socket.  */
	MYSERVER_SOCKET socket;
	
	/*! Current timeout for the connection.  */
	u_long timeout;
	
	/*! Next CONNECTION in linked list.  */
	CONNECTION* next;
	
	/*! Pointer to an host structure.  */
	void *host;
	
	/*! Data size read in the buffersize2.  */
	int dataRead;
	
	/*! If nonzero the server is saying to the protocol to remove the connection.
   *Protocols can not consider this but is a good idea do it to avoid server
   * overloads. 
   *Reasons to remove the connection are defined at the begin of this file.  */
	int toRemove;
	
	/*! Force the connection to be parsed.  */
	int forceParsing;
	
	/*! This buffer must be used only by the ClientsTHREAD class.  */
	char connectionBuffer[KB(8)];
	
	/*! Buffer for the connecion struct. Used by protocols.  */
	char *protocolBuffer;
};
typedef CONNECTION* volatile LPCONNECTION;
#endif
