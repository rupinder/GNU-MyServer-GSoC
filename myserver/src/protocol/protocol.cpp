/*
MyServer
Copyright (C) 2002, 2003, 2004, 2008, 2009 Free Software Foundation, Inc.
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

#include <include/protocol/protocol.h>

/*!
 * Load the protocol. Called once at runtime.
 */
int Protocol::loadProtocol ()
{
  return 1;
}

/*!
 * Unload the protocol. Called once.
 */
int Protocol::unLoadProtocol ()
{
  return 1;
}

/*!
 * Control the connection.
 */
int Protocol::controlConnection (ConnectionPtr /*a*/,char* /*b1*/,
                                 char* /*b2*/,int /*bs1*/,int /*bs2*/,
                                 u_long /*nbtr*/,u_long /*id*/)
{
  return 0;
}

/*!
 * Returns the name of the protocol. If an out buffer is 
 * defined fullfill it with the name too.
 */
char* Protocol::registerName (char* /*out*/,int /*len*/)
{
  return 0;
}

/*!
 * Constructor for the class protocol.
 */
Protocol::Protocol ()
{
  protocolOptions = 0;
  protocolPrefix.assign ("");
}

/*!
 * Destroy the object.
 */
Protocol::~Protocol ()
{

}
