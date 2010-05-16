/*
  MyServer
  Copyright (C) 2005, 2006, 2007, 2009, 2010 Free Software Foundation,
  Inc.
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
#include <include/protocol/http/http_request.h>
#include <include/base/string/stringutils.h>
#include <iostream>
#include <sstream>
#include <algorithm>

using namespace std;

/*!
  Create the object.
 */
HttpRequestHeader::HttpRequestHeader ()
{
  free ();
}

/*!
  Destroy the object.
 */
HttpRequestHeader::~HttpRequestHeader ()
{
  HashMap<string, HttpRequestHeader::Entry*>::Iterator it = other.begin ();
  for (; it != other.end (); it++)
    delete (*it);
}

/*!
  Free the structure.
 */
void HttpRequestHeader::free ()
{
  ver.clear ();
  cmd.clear ();
  auth.clear ();
  contentLength.clear ();
  uri.clear ();
  uriOpts.clear ();
  uriOptsPtr = NULL;

  {
    HashMap<string, HttpRequestHeader::Entry*>::Iterator it = other.begin ();
    for (; it != other.end (); it++)
      delete (*it);
  }
  other.clear ();
  rangeType.clear ();
  rangeByteBegin = 0;
  rangeByteEnd = 0;
  uriEndsWithSlash = 0;
  digestRealm[0] = '\0';
  digestOpaque[0] = '\0';
  digestNonce[0] = '\0';
  digestCnonce[0] = '\0';
  digestUri[0] = '\0';
  digestMethod[0] = '\0';
  digestUsername[0] = '\0';
  digestResponse[0] = '\0';
  digestQop[0] = '\0';
  digestNc[0] = '\0';
}

/*!
  Check if this request is keep-alive.
 */
bool HttpRequestHeader::isKeepAlive ()
{
  Entry *connection = other.get ("connection");
  if (connection)
    return (! stringcmpi (connection->value,
                          "keep-alive"));

  return false;
}

/*!
  Get the value of the [name] field.
 */
string* HttpRequestHeader::getValue (const char* name, string* out)
{
  if (! strcasecmp (name, "cmd"))
    {
      if (out)
        out->assign (cmd.c_str ());
      return &cmd;
    }

  if (! strcasecmp (name, "ver"))
    {
      if (out)
        out->assign (ver.c_str ());
      return &ver;
    }

  if (! strcasecmp (name, "uri"))
    {
      if (out)
        out->assign (uri.c_str ());
      return &uri;
    }

  if (! strcasecmp (name, "uriOpts"))
    {
      if (out)
        out->assign (uriOpts.c_str ());
      return &uriOpts;
    }

  if (! strcasecmp (name, "Authorization"))
    {
      if (out)
        out->assign (auth.c_str ());
      return &auth;
    }

  if (! strcasecmp (name, "Content-length"))
    {
      if (out)
        out->assign (contentLength.c_str ());
      return &contentLength;
    }

  if (! strcasecmp (name, "rangeType"))
    {
      if (out)
        out->assign (rangeType.c_str ());
      return &rangeType;
    }

  if (! strcasecmp (name, "rangeByteBegin"))
    {
      ostringstream s;
      s << rangeByteBegin;
      if (out)
        out->assign (s.str ());
      return 0;
    }

  if (! strcasecmp (name, "rangeByteEnd"))
    {
      ostringstream s;
      s << rangeByteEnd;
      if (out)
        out->assign (s.str ());
      return 0;
    }

  return HttpHeader::getValue (name, out);
}


/*!
  Set the value of the [name] field to [in].
 */
string* HttpRequestHeader::setValue (const char *name, const char *in)
{
  if (! strcasecmp (name, "cmd"))
    {
      cmd.assign (in);
      return &cmd;
    }

  if (! strcasecmp (name, "ver"))
    {
      ver.assign (in);
      return &ver;
    }

  if (! strcasecmp (name, "uri"))
    {
      uri.assign (in);
      return &uri;
    }

  if (! strcasecmp (name, "uriOpts"))
    {
      uriOpts.assign (in);
      return &uriOpts;
    }

  if (! strcasecmp (name, "Authorization"))
    {
      auth.assign (in);
      return &auth;
    }

  if (! strcasecmp (name, "Content-length"))
    {
      contentLength.assign (in);
      return &contentLength;
    }

  if (! strcasecmp (name, "rangeType"))
    {
      rangeType.assign (in);
      return &rangeType;
    }

  if (! strcasecmp (name, "rangeByteBegin"))
    {
      rangeByteBegin = atoi (in);
      return 0;
    }

  if (! strcasecmp (name, "rangeByteEnd"))
    {
      rangeByteEnd = atoi (in);
      return 0;
    }

  return HttpHeader::setValue (name, in);
}
