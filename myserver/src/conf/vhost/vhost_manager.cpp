/*
  MyServer
  Copyright (C) 2007, 2008, 2009 Free Software Foundation, Inc.
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
#include <include/conf/vhost/vhost_manager.h>
#include <include/conf/vhost/vhost.h>
#include <include/conf/mime/xml_mime_handler.h>
#include <include/server/server.h>
#include <include/base/file/files_utility.h>
#include <include/conf/mime/xml_mime_handler.h>

#include <include/conf/xml_conf.h>

VhostManagerHandler::VhostManagerHandler ()
{
}

VhostManagerHandler::~VhostManagerHandler ()
{
}

Vhost* VhostManagerHandler::getVHost (const char*, const char*, u_short)
{
  return NULL;
}

Vhost* VhostManagerHandler::getVHostByNumber (int n)
{
  return 0;
}

int VhostManagerHandler::addVHost (Vhost*)
{
  return 0;
}

/*!
 * C'tor.
 */
VhostManager::VhostManager ()
{
  handler = NULL;
}

/*!
 * Set the handler where delegate all requests.
 *
 *\param handler The new handler.
 */
void VhostManager::setHandler (VhostManagerHandler *handler)
{
  this->handler = handler;
}

/*!
 *Get the vhost for the connection. A return value of 0 means that
 *a valid host was not found.
 *\param host Hostname for the virtual host.
 *\param ip IP address for the virtual host.
 *\param port The port used by the client to connect to the server.
 */
Vhost* VhostManager::getVHost (const char *host, const char *ip, u_short port)
{
  if (handler)
    return handler->getVHost (host, ip, port);

  return NULL;
}


/*!
 *Get a virtual host by its position in the list.
 *Zero based list.
 *\param n The virtual host id.
 */
Vhost* VhostManager::getVHostByNumber (int n)
{
  if (handler)
    return handler->getVHostByNumber (n);

  return NULL;
}

/*!
 * Register a builder function for a vhost manager.
 * \param name manager name.
 * \param builder Builder routine.
 */
void VhostManager::registerBuilder (string &name, MAKE_HANDLER builder)
{
  builders.put (name, builder);
}

/*!
 * Build an handler given its name.
 *
 * \param name handler name.
 * \return an instance of the requested handler type.
 */
VhostManagerHandler *VhostManager::buildHandler (string &name,
                                                 ListenThreads *lt,
                                                 LogManager *lm)
{
  MAKE_HANDLER builder = builders.get (name);

  if (builder)
    return builder (lt, lm);

  return NULL;
}
