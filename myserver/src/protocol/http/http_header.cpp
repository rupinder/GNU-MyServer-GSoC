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
#include <include/protocol/http/http_header.h>

#include <string>
#include <algorithm>

using namespace std;

/*!
  Get the value of the [name] field.
 */
string* HttpHeader::getValue (const char *name, string *out)
{
  string key (name);
  transform (key.begin (), key.end (), key.begin (), ::tolower);

  Entry *e = other.get (key);
  if (e)
    {
      if (out)
        out->assign (e->value);
      return &(e->value);
    }

  return NULL;
}

/*!
  Set the value of the [name] field to [in].
 */
string* HttpHeader::setValue (const char *name, const char *in)
{
  string key (name);
  transform (key.begin (), key.end (), key.begin (), ::tolower);

  Entry *e = other.get (key);
  if (e)
    {
      e->value.assign (in);
      return &(e->value);
    }
  else
    {
      e = new Entry;
      e->name.assign (name);
      e->value.assign (in);
      other.put (key, e);
    }

  return NULL;
}
