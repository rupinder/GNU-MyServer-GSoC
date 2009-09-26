/*
  MyServer
  Copyright (C) 2005, 2007, 2008, 2009 Free Software Foundation, Inc.
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
#include <include/protocol/http/http_response.h>
#include <include/base/string/stringutils.h>
#include <iostream>
#include <sstream>
using namespace std;

/*!
 *Create the object.
 */
HttpResponseHeader::HttpResponseHeader ()
{
  free();
}


/*!
 *Destroy the object.
 */
HttpResponseHeader::~HttpResponseHeader ()
{
  HashMap<string, HttpResponseHeader::Entry*>::Iterator it = other.begin ();
  for (;it != other.end(); it++)
    delete (*it);
}

/*!
 *Reset the object.
 */
void HttpResponseHeader::free()
{
  ver.clear ();
  serverName.clear ();
  contentType.clear ();
  connection.clear ();
  cookie.clear ();
  contentLength.clear ();
  errorType.clear ();

  HashMap<string, HttpResponseHeader::Entry*>::Iterator it = other.begin ();
  for (;it != other.end (); it++)
    delete (*it);

  other.clear ();
}

/*!
 *Get the value of the [name] field.
 */
string* HttpResponseHeader::getValue (const char* name, string* out)
{
  if (!strcmpi (name, "Ver"))
    {
      if (out)
        out->assign (ver.c_str ());
      return &ver;
    }

  if (!strcmpi (name, "Server"))
    {
      if (out)
        out->assign (serverName.c_str ());
      return &ver;
    }

  if (!strcmpi (name, "Content-Type"))
    {
      if (out)
        out->assign (contentType.c_str ());
      return &contentType;
    }

  if (!strcmpi (name, "Connection"))
    {
      if (out)
        out->assign (connection.c_str ());
      return &connection;
    }

  if (!strcmpi (name, "Content-Type"))
    {
      if (out)
        out->assign (contentType.c_str ());
      return &contentType;
    }

  if (!strcmpi (name, "Cookie"))
    {
      if (out)
        out->assign (cookie.c_str ());
      return &cookie;
    }

  if (!strcmpi (name, "Content-Length"))
    {
      if (out)
        out->assign (contentLength.c_str ());
      return &contentLength;
    }

  HttpResponseHeader::Entry *e = other.get(name);
  if (e)
    {
      if (out)
        out->assign (*(e->value));
      return e->value;
    }
  return 0;
}

/*!
 *Set the value of the [name] field to [in].
 */
string* HttpResponseHeader::setValue (const char* name, const char* in)
{
  if (!strcmpi (name, "Ver"))
    {
      ver.assign (in);
      return &ver;
    }

  if (!strcmpi (name, "Server"))
    {
      serverName.assign (in);
      return &serverName;
    }

  if (!strcmpi (name, "Content-Type"))
    {
      contentType.assign (in);
      return &contentType;
    }

  if (!strcmpi (name, "Connection"))
    {
      connection.assign (in);
      return &connection;
    }

  if (!strcmpi (name, "Content-Type"))
    {
      contentType.assign (in);
      return &contentType;
    }

  if (!strcmpi (name, "Cookie"))
    {
      cookie.assign (in);
      return &cookie;
    }

  if (!strcmpi (name, "Content-Length"))
    {
      contentLength.assign (in);
      return &contentLength;
    }

  {
    HttpResponseHeader::Entry *e = other.get(name);
    if (e)
      {
        e->value->assign (in);
        return (e->value);
      }
    else
      {
        e = new HttpResponseHeader::Entry;
        e->name->assign (name);
        e->value->assign (in);
        other.put (*e->name, e);
      }

    return 0;
  }
}

/*!
 *Get the kind of HTTP status code specified by the httpStatus variable.
 *\return the HTTP status kind.
 */
int HttpResponseHeader::getStatusType ()
{
  if (httpStatus < 200)
    return HttpResponseHeader::INFORMATIONAL;

  if (httpStatus < 300)
    return HttpResponseHeader::SUCCESSFUL;

  if (httpStatus < 400)
    return HttpResponseHeader::REDIRECTION;

  if (httpStatus < 500)
    return HttpResponseHeader::CLIENT_ERROR;

  return HttpResponseHeader::SERVER_ERROR;
}
