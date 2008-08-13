/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007, 2008 Free Software Foundation, Inc.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CONNECTION_H
#define CONNECTION_H

extern "C"
{
#include <sys/time.h>
#include <sys/types.h>
}

#include "../include/socket.h"
#include "../include/protocol_buffer.h"
#include "../include/utility.h"
#include "../include/mem_buff.h"

class Vhost;
class ClientsThread;

#include <string>

using namespace std;

/*! Remove the connection due a high server load.  */
#define CONNECTION_REMOVE_OVERLOAD 1

/*! Remove the connection if the administrator decided this.  */
#define CONNECTION_USER_KILL        2


class Connection;
                                   
typedef  Connection* ConnectionPtr;

typedef int (*continuationPROC)(ConnectionPtr a, char *b1, char *b2,
                                int bs1, int bs2, u_long nbtr, u_long id);


class Connection
{
public:
	int getPriority();
	void setPriority(int);

  u_long getID();
  void setID(u_long);

  void setScheduled(int);
  int isScheduled();
  int allowDelete(bool bWait = false);

  u_short getPort();
  void setPort(u_short);
	
  u_short getLocalPort();
  void setLocalPort(u_short);

  const char* getLogin();
  void setLogin(const char*);

  const char* getPassword();
  void setPassword(const char*);

	void setnTries(char);
	char getnTries();
	void incnTries();

  const char* getIpAddr();
  void setIpAddr(const char*);

  const char* getLocalIpAddr();
  void setLocalIpAddr(const char*);

	u_long getTimeout();
  void setTimeout(u_long);

	/*! Connection socket.  */
	Socket *socket;
	
	/*! Pointer to an host structure.  */
	Vhost *host;
	
  int getToRemove();
  void setToRemove(int);

  int isForceControl();
  void setForceControl(int);	
	
	/*! This buffer must be used only by the ClientsTHREAD class.  */
	MemBuf connectionBuffer;
	
	/*! Buffer for the connection struct. Used by protocols.  */
	ProtocolBuffer *protocolBuffer;

	/*! Set the thread that is currently using the connection.  */
	void setActiveThread(ClientsThread* t){thread = t;}

	/*! Get the thread that is using the connection.  */
	ClientsThread* getActiveThread(){return thread;}

  Connection();
  virtual ~Connection();

  /*! Get the continuation function.  */
  continuationPROC getContinuation(){return continuation;}

  /*! Set a new continuation function.  */
  void setContinuation(continuationPROC newContinuation){continuation = newContinuation;}

  /*! Check if the connection has a continuation.  */
  bool hasContinuation(){return continuation ? true : false;}
protected:
	ClientsThread *thread;

  /*! Continuation function.  */
  continuationPROC continuation;

  /*! Identifier for the connection.  */
  u_long ID;

	/*! The server has scheduled this connection.  */
	int scheduled;

	/*! Remote port used.  */
	u_short port;

	/*! Login name.  */
	string login;
	
	/*! Password used to log in.  */
	string password;

	/*! # of tries for an authorized login.  */
	char nTries;

	/*! Remote IP address.  */
	string ipAddr;
	
	/*! Local IP used to connect to.  */
	string localIpAddr;

	/*! Local port used to connect to.  */
	u_short localPort;

	/*! Current timeout for the connection.  */
	u_long timeout;

	/*
   *!If nonzero the server is saying to the protocol to remove the connection.
   *Protocols can not consider this but is a good idea do it to avoid server
   *overloads. 
   *Reasons to remove the connection are defined at the begin of this file.  
   */
	int toRemove;
	
	/*! Force the connection to be parsed.  */
	int forceControl;

	/*! Connection priority, used by the scheduler.  */
	int priority;

};

#endif
