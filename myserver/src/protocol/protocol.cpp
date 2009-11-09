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

#include "stdafx.h"
#include <include/protocol/protocol.h>
#include <include/server/clients_thread.h>
#include <include/server/server.h>

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
 * Returns the name of the protocol. If an out buffer is
 * defined fullfill it with the name too.
 */
const char* Protocol::getName ()
{
  return "";
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
