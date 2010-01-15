/*
  MyServer
  Copyright (C) 2009, 2010 Free Software Foundation, Inc.
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
#include <include/protocol/url.h>


/*!
 *Build an URL object.
 *\param url The url string.
 *\param port the default port to use in case it is not specified.
 */
Url::Url (string &url, u_short port)
{
  parse (url, port);
}

/*!
 *Build an URL object.
 *\param url The url string.
 *\param port the default port to use in case it is not specified.
 */
Url::Url (const char* url, u_short port)
{
  string urlStr (url);
  parse (urlStr, port);
}

/*!
 *Get the protocol part of the URL.
 */
string& Url::getProtocol ()
{
  return protocol;
}


/*!
 *Get the query part of the URL.
 */
string& Url::getQuery ()
{
  return query;
}

/*!
 *Get the credentials part of the URL.
 */
string& Url::getCredentials ()
{
  return credentials;
}

/*!
 *Get the port part of the URL.
 */
u_short Url::getPort ()
{
  return port;
}

/*!
 *Get the resource part of the URL.
 */
string& Url::getResource ()
{
  return resource;
}

/*!
 *Get the host part of the URL.
 */
string& Url::getHost ()
{
  return host;
}

/*!
 *Parse the URL.  Internal function.
 */
void Url::parse (string &url, u_short defPort)
{
  size_t portLoc;
  size_t firstSlash;
  size_t protoEnd = url.find ("://");
  size_t credentialStart;
  if (protoEnd == string::npos)
    return;

  protocol = url.substr (0, protoEnd);

  portLoc = url.find (':', protoEnd + 3);
  firstSlash = url.find ('/', portLoc == string::npos ? protoEnd + 3 :
                                                        portLoc + 1);

  credentialStart = url.find ('@', protoEnd + 3);
  size_t offsetCredentials = credentialStart != string::npos  ? credentialStart + 1
                                                              : protoEnd + 3;

  if (credentialStart != string::npos)
    credentials = url.substr (protoEnd + 3, offsetCredentials - protoEnd - 4);
  else
    credentials = "";

  /* Check if the port is specified.  */
  if (portLoc != string::npos)
    {
      host = url.substr (offsetCredentials, portLoc - offsetCredentials);
      port = atoi (url.substr (portLoc + 1, firstSlash - portLoc - 1).c_str ());
    }
  else
    {
      host = url.substr (offsetCredentials, firstSlash - offsetCredentials);
      port = defPort;
    }

  size_t queryStart = url.find ('?', firstSlash + 1);

  if (queryStart == string::npos)
    {
      resource = url.substr (firstSlash + 1);
      query = "";
    }
  else
    {
      resource = url.substr (firstSlash + 1, queryStart - firstSlash - 1);
      query = url.substr (queryStart + 1);
    }
}
