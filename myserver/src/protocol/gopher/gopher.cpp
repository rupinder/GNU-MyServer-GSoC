/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2002-2009 Free Software Foundation, Inc.
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

#include <include/protocol/gopher/gopher.h>
#include <include/protocol/gopher/gopher_content.h>
#include <include/protocol/gopher/engine.h>
#include <include/server/server.h>
#include <include/server/clients_thread.h>
#include <include/base/string/securestr.h>

#include <iostream>

/*!
 * Constructor for the Gopher class
 */

Gopher::Gopher ()
{
}

/*!
 * Destructor for the Gopher class.
 */
Gopher::~Gopher ()
{
}

/*!
 * Service method
 */

char *Gopher::registerNameImpl (char* out, int len)
{
  if (out)
    myserver_strlcpy (out, "GOPHER", len);

  return (char*) "GOPHER";
}

/*!
 * The loading unloading routine
 */

int Gopher::loadProtocolStatic ()
{
}

int Gopher::unLoadProtocolStatic ()
{
  return 1;
}

/*!
 * Main protocol implementation stuff
 */

int Gopher::controlConnection (ConnectionPtr pConnection,
                               char *b1,
                               char *b2,
                               u_long bs1,
                               u_long bs2,
                               u_long nbtr,
                               u_long id)
{
  char buffer[256];
  u_long nbr;
  GopherEngine g;

  if (pConnection == NULL)
    return ClientsThread::DELETE_CONNECTION;

  Server *server = Server::getInstance ();
  if (server == NULL)
    return ClientsThread::DELETE_CONNECTION;

  string command (b1);
  string::size_type loc = command.find ('\n', 0);
  while( loc == string::npos )
    {
      pConnection->socket->read (buffer, sizeof (buffer), &nbr);
      command += buffer;
      loc = command.find ('\n', 0);
    }

  GopherContent &Gu = g.incoming (GopherRequest (command.substr (0, loc - 1),
                                            pConnection), pConnection->host);
  reply (pConnection,Gu);
}

void Gopher::reply (ConnectionPtr pConnection,
                    GopherContent &data)
{
  data.toProtocol (pConnection->socket);
  pConnection->socket->close ();
}
