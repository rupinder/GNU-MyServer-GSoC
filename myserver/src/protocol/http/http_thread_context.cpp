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
#include <sstream>
#include <include/protocol/http/http_thread_context.h>
#include <include/conf/vhost/vhost.h>

/*!
  Get the value for name in the hash dictionary.
  If the key is not present in the hash map then the request
  is propagated to the virtual host, if it is defined.
  \param name The key name to look for in the hash map.
 */
const char* HttpThreadContext::getData (const char *name)
{
  Vhost *vh = (Vhost*)connection->host;

  string *ret = other.get (string (name));

  if (ret)
    return ret->c_str ();
  else
    return vh ? vh->getData (name) : NULL;
}

/*!
  Get the current vhost doc directory for the environvment.
 */
const char *HttpThreadContext::getVhostDir ()
{
  if (vhostDir.length () > 1)
    return vhostDir.c_str ();

  if (connection && connection->host)
    return connection->host->getDocumentRoot ().c_str ();

  return "";
}

/*!
  Get the current vhost sys directory for the environvment.
 */
const char *HttpThreadContext::getVhostSys ()
{
  if (vhostSys.length () > 1)
    return vhostSys.c_str ();

  if (connection && connection->host)
    return connection->host->getSystemRoot ().c_str ();

  return "";
}
