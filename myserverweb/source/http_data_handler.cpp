/*
*MyServer
*Copyright (C) 2005 The MyServer Team
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
#include "../include/http_headers.h"
#include "../include/http.h"
#include "../include/http_data_handler.h"

/*!
 *Send a file to the client using the HTTP protocol.
 */
int HttpDataHandler::send(HttpThreadContext*/* td*/, ConnectionPtr /*s*/, 
                          const char* /*filenamePath*/, const char* /*exec*/,
                          int /*onlyHeader*/)
{
  return 0;
}

/*!
 *Constructor for the class.
 */
HttpDataHandler::HttpDataHandler()
{

}

/*!
 *Destroy the object.
 */
HttpDataHandler::~HttpDataHandler()
{

}

/*!
 *Load the static elements.
 */
int HttpDataHandler::load(XmlParser* /*confFile*/)
{
  return 0;
}

/*!
 *Unload the static elements.
 */
int HttpDataHandler::unload()
{
  return 0;
}
