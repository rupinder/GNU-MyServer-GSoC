/*
MyServer
Copyright (C) 2002, 2003, 2004 The MyServer Team
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "../include/connection.h"
#include "../include/vhosts.h"

/*!
 *Create the buffer.
 */
ProtocolBuffer::ProtocolBuffer()
{

}

/*!
 *Destroy the protocol buffer object.
 */
ProtocolBuffer::~ProtocolBuffer()
{

}


/*!
 *Constructor for the Connection class.
 */
Connection::Connection()
{
  thread = 0;
  parsing = 0;
  login.assign("");
  password.assign("");
  nTries = 0;
	ipAddr[0]='\0';
  localIpAddr[0]='\0';
  port = 0;
	localPort = 0;
  timeout = 0;
  next = 0;
  host = 0;
	dataRead = 0;
  toRemove = 0;
  forceParsing = 0;
  connectionBuffer[0] = '\0';
  protocolBuffer = 0;

}

/*!
 *Destroy the object.
 */
Connection::~Connection()
{
	socket.shutdown(SD_BOTH);
	char buffer[256];
	int buffersize=256;
  int err;
	do
	{
		err=socket.recv(buffer, buffersize, 0);
	}while((err!=-1) && err);
	socket.closesocket();

  if(protocolBuffer)
    delete  protocolBuffer;

  /*! Remove the reference for the vhost. */
  if(host)
    ((Vhost*)host)->removeRef();
}

/*!
 *Return the IDentifier for the connection.
 */
u_long Connection::getID()
{
  return ID;
}

/*!
 *Set the IDentifier for the connection.
 *\param nID The new ID. 
 */
void Connection::setID(u_long nID)
{
  ID = nID;
}

/*!
 *Set the parsing state.
 *\param np The new parsing state.
 */
void Connection::setParsing(int np)
{
  parsing = np;
}

/*!
 *Return if the connection is currently parsed.
 */
int Connection::isParsing()
{
  return parsing;
}

/*!
 *Get the port used by the connection.
 */
u_short Connection::getPort()
{
  return port;
}

/*!
 *Set the port used by the connection.
 *\param newPort The new port.
 */
void Connection::setPort(u_short newPort)
{
  port = newPort;
}

/*!
 *Get the login name used by the connection user.
 */
const char* Connection::getLogin()
{
  return login.c_str();
}

/*!
 *Set the login name for the connection user.
 *\param loginName The login name. 
 */
void Connection::setLogin(const char* loginName)
{
  login.assign(loginName);
}

/*!
 *Set the # of attempts to authenticate the user.
 *\arg n The new number of tries.
 */
void Connection::setnTries(char n)
{
  nTries = n;
}

/*!
*Get the # of attempts to authenticate the user.
 */
char Connection::getnTries()
{
  return nTries;
}
/*!
 *Increment by 1 the # of attempts to authenticate the user.
 */
void Connection::incnTries()
{
  nTries++;
}

/*!
 *Get the IP address of the client.
 */
const char* Connection::getIpAddr()
{
  return ipAddr;
}

/*!
 *Set the IP address of the client.
 *\param na The new IP address.
 */
void Connection::setIpAddr(const char* na)
{
  strncpy(ipAddr, na, MAX_IP_STRING_LEN);
}

/*!
 *Get the IP address of the local interface used to connect to.
 */
const char* Connection::getLocalIpAddr()
{
  return localIpAddr;
}

/*!
 *Set the IP address of the local interface used to connect to.
 *\param na The new local IP address.
 */
void Connection::setLocalIpAddr(const char* na)
{
  strncpy(localIpAddr, na, MAX_IP_STRING_LEN);
}

/*!
 *Get the local port used to connect to.
 */
u_short Connection::getLocalPort()
{
  return localPort;
}

/*!
 *Set the local port used to connect to.
 *\param np The new local port. 
 */
void Connection::setLocalPort(u_short np)
{
  localPort = np;
}

/*!
 *Get the timeout to use with the connection.
 */
u_long Connection::getTimeout()
{
  return timeout;
}

/*!
 *Set the timeout to use with the connection.
 *\param nTimeout The new timeout value. 
 */
void Connection::setTimeout(u_long nTimeout)
{
  timeout = nTimeout;
}

/*!
 *Return the number of bytes read.
 */
int Connection::getDataRead()
{
  return dataRead;
}

/*!
 *Set the number of bytes read.
 *\param dr The new data read value. 
 */
void Connection::setDataRead(int dr)
{
  dataRead = dr;
}

/*!
 *Return if the connection must be removed and why.
 */
int Connection::getToRemove()
{
  return toRemove;
}

/*!
 *Set the reason to remove the connection.
 *\param r Set if the connection has to be removed. 
 */
void Connection::setToRemove(int r)
{
  toRemove = r;
}

/*!
 *Get if the connection is forced to be parsed.
 */
int Connection::getForceParsing()
{
  return forceParsing;
}
/*!
 *Force the parsing of this connection on next server loop.
 *\param fp The new force parsing value even if there is new data. 
 */
void Connection::setForceParsing(int fp)
{
  forceParsing = fp;
}

/*!
 *Return the password submitted by the user.
 */
const char* Connection::getPassword()
{
  return password.c_str();
}

/*!
 *Set the password for the user.
 *\param p The new password.
 */
void Connection::setPassword(const char* p)
{
  password.assign(p);
}
