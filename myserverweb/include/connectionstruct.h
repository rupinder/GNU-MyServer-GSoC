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
#define PROTOCOL_CONTROL		1002

/*! Remove the connection due a high server load.  */
#define CONNECTION_REMOVE_OVERLOAD 1

/*! Remove the connection if the administrator decided this.  */
#define CONNECTION_USER_KILL        2


typedef u_long CONNECTION_PROTOCOL;

/*!
 *Base class to handle a buffer in the connection.
 */
class	protocol_buffer
{
public:
  protocol_buffer();
	virtual ~protocol_buffer();
};



class CONNECTION
{
  /*! IDentifier for the CONNECTION. */
  u_long ID;
	/*! The server is parsing this connection. */
	int parsing;

	/*! Remote port used.  */
	u_short port;

	/*! Login name. */
	char login[20];
	
	/*! Password used to log in. */
	char password[32];

	/*! # of tries for an authorized login. */
	char nTries;

	/*! Remote IP address.  */
	char ipAddr[MAX_IP_STRING_LEN];
	
	/*! Local IP used to connect to.  */
	char localIpAddr[MAX_IP_STRING_LEN];

	/*! Local port used to connect to.  */
	u_short localPort;

	/*! Current timeout for the connection.  */
	u_long timeout;

  /*! Number of bytes ready in the buffer. */
	int dataRead;
	
	/*
   *!If nonzero the server is saying to the protocol to remove the connection.
   *Protocols can not consider this but is a good idea do it to avoid server
   *overloads. 
   *Reasons to remove the connection are defined at the begin of this file.  
   */
	int toRemove;
	
	/*! Force the connection to be parsed.  */
	int forceParsing;
public:
  u_long getID();
  void setID(u_long);

	/*! Pointer to the thread struct that is using the CONNECTION. */
	void *thread;

  void setParsing(int);
  int isParsing();

  u_short getPort();
  void setPort(u_short);
	
  u_short getLocalPort();
  void setLocalPort(u_short);

  char* getLogin();
  void setLogin(char*);

  char* getPassword();
  void setPassword(char*);

	void setnTries(char);
	char getnTries();
	void incnTries();

  char* getipAddr();
  void setipAddr(char*);

  char* getlocalIpAddr();
  void setlocalIpAddr(char*);

	u_long getTimeout();
  void setTimeout(u_long);

	/*! Connection socket.  */
	MYSERVER_SOCKET socket;
	
	/*! Next CONNECTION in linked list.  */
	CONNECTION* next;
	
	/*! Pointer to an host structure.  */
	void *host;
	
  int getDataRead();
  void setDataRead(int);

  int getToRemove();
  void setToRemove(int);

  int getForceParsing();
  void setForceParsing(int);	
	
	/*! This buffer must be used only by the ClientsTHREAD class.  */
	char connectionBuffer[KB(8)];
	
	/*! Buffer for the connecion struct. Used by protocols.  */
	protocol_buffer *protocolBuffer;

  CONNECTION();
  virtual ~CONNECTION();
};
typedef CONNECTION* volatile LPCONNECTION;

#endif
