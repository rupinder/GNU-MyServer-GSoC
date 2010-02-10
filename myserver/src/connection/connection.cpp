/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007, 2008, 2009, 2010 Free Software
Foundation, Inc.
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

#include "myserver.h"
#include <include/connection/connection.h>
#include <include/conf/vhost/vhost.h>

/*!
 * Initialize the structure.
 */
void Connection::init ()
{
  thread = 0;
  scheduled = 0;
  login = new string ();
  password = new string ();
  ipAddr = new string ();
  localIpAddr = new string ();
  nTries = 0;
  port = 0;
  localPort = 0;
  timeout = 0;
  host = 0;
  toRemove = 0;
  forceControl = 0;
  protocolBuffer = NULL;
  socket = NULL;
  priority = -1;
  continuation = NULL;
	connectionBuffer = new MemBuf ();
}

/*!
 * Destroy the object.
 */
void Connection::destroy ()
{
  if (socket)
  {
    socket->shutdown (SHUT_RDWR);
    socket->close ();
    delete socket;
    socket = NULL;
  }

  if (login)
    delete login;

  if (password)
    delete password;

  if (ipAddr)
    delete ipAddr;

  if (localIpAddr)
    delete localIpAddr;

  if (connectionBuffer)
    delete connectionBuffer;

  if (protocolBuffer)
    delete protocolBuffer;

  /*! Remove the reference for the vhost. */
  if (host)
    ((Vhost*)host)->removeRef ();

  login = NULL;
  password = NULL;
  ipAddr = NULL;
  localIpAddr = NULL;
  protocolBuffer = NULL;
  connectionBuffer = NULL;

  host = NULL;
}

/*!
 *Return the IDentifier for the connection.
 */
u_long Connection::getID ()
{
  return ID;
}

/*!
 *Set the IDentifier for the connection.
 *\param nID The new ID.
 */
void Connection::setID (u_long nID)
{
  ID = nID;
}

/*!
 *Set if the connection is scheduled by the server.
 *\param np The new scheduled state.
 */
void Connection::setScheduled (int np)
{
  scheduled = np;
}

/*!
 *Return if the connection is scheduled.
 */
int Connection::isScheduled ()
{
  return scheduled;
}

/*!
 *Return if the connection may be deleted by the server.
 */
int Connection::allowDelete (bool bWait/*= false*/)
{
  if (isScheduled ())
     return 0;

  if (protocolBuffer != NULL)
    return protocolBuffer->allowDelete (bWait);

  return 1;
}

/*!
 *Get the port used by the connection.
 */
u_short Connection::getPort ()
{
  return port;
}

/*!
 *Set the port used by the connection.
 *\param newPort The new port.
 */
void Connection::setPort (u_short newPort)
{
  port = newPort;
}

/*!
 *Get the login name used by the connection user.
 */
const char* Connection::getLogin ()
{
  return login->c_str ();
}

/*!
 *Set the login name for the connection user.
 *\param loginName The login name.
 */
void Connection::setLogin (const char* loginName)
{
  login->assign (loginName);
}

/*!
 *Set the # of attempts to authenticate the user.
 *\arg n The new number of tries.
 */
void Connection::setnTries (char n)
{
  nTries = n;
}

/*!
 *Get the attempts number to authenticate the user.
 */
char Connection::getnTries ()
{
  return nTries;
}
/*!
 *Increment by 1 the # of attempts to authenticate the user.
 */
void Connection::incnTries ()
{
  nTries++;
}

/*!
 *Get the IP address of the client.
 */
const char* Connection::getIpAddr ()
{
  return ipAddr->c_str ();
}

/*!
 *Set the IP address of the client.
 *\param na The new IP address.
 */
void Connection::setIpAddr (const char* na)
{
  ipAddr->assign (na);
}

/*!
 *Get the IP address of the local interface used to connect to.
 */
const char* Connection::getLocalIpAddr ()
{
  return localIpAddr->c_str ();
}

/*!
 *Set the IP address of the local interface used to connect to.
 *\param na The new local IP address.
 */
void Connection::setLocalIpAddr (const char* na)
{
  localIpAddr->assign (na);
}

/*!
 *Get the local port used to connect to.
 */
u_short Connection::getLocalPort ()
{
  return localPort;
}

/*!
 *Set the local port used to connect to.
 *\param np The new local port.
 */
void Connection::setLocalPort (u_short np)
{
  localPort = np;
}

/*!
 *Get the timeout to use with the connection.
 */
u_long Connection::getTimeout ()
{
  return timeout;
}

/*!
 *Set the timeout to use with the connection.
 *\param nTimeout The new timeout value.
 */
void Connection::setTimeout (u_long nTimeout)
{
  timeout = nTimeout;
}

/*!
 *Return if the connection must be removed and why.
 */
int Connection::getToRemove ()
{
  return toRemove;
}

/*!
 *Set the reason to remove the connection.
 *\param r Set if the connection/connection.has to be removed.
 */
void Connection::setToRemove (int r)
{
  toRemove = r;
}

/*!
 *Get if the connection is forced to be parsed.
 */
int Connection::isForceControl ()
{
  return forceControl;
}
/*!
 *Force the control of this connection on next server loop.
 *\param fp The new force control value even if there is new data.
 */
void Connection::setForceControl (int fp)
{
  forceControl = fp;
}

/*!
 *Return the password submitted by the user.
 */
const char* Connection::getPassword ()
{
  return password->c_str ();
}

/*!
 *Set the password for the user.
 *\param p The new password.
 */
void Connection::setPassword (const char* p)
{
  password->assign (p);
}

/*!
 *Get the connection priority.
 */
int Connection::getPriority ()
{
  return priority;
}

/*!
 *Set the connection priority.
 *\param p The new priority.
 */
void Connection::setPriority (int p)
{
  priority = p;
}
