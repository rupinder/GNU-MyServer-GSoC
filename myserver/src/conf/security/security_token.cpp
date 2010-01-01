/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
  Free Software Foundation, Inc.
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

#include <include/conf/security/security_token.h>
#include <include/conf/vhost/vhost.h>
#include <include/server/server.h>

#include <string>
#include <sstream>
#include <memory>

using namespace std;

/*!
 *Create the object.
 */
SecurityToken::SecurityToken ()
{
  reset ();
}

/*!
  Reset every structure member.
*/
void SecurityToken::reset ()
{
  mask = 0;
  done = false;
  server = NULL;
  vhost = NULL;
  mimeRecord = NULL;
  directory = NULL;
  sysdirectory = NULL;
  resource = NULL;
  HashMap<string, NodeTree<string>*>::Iterator it = values.begin ();
  for (;it != values.end (); it++)
    delete (*it);
  values.clear ();
}

/*!
  Get the value for the variable using the specified domains.
  \param key Variable name.
  \param def Default value.
  \param domains Domains where to look.  They are looked in this order:
  \li Security configuration file.
  \li Virtual host configuration file.
  \li Global security file.
  \li Default value.
*/
NodeTree<string>* SecurityToken::getNodeTree (string& key, int domains,
                                              NodeTree<string>* def)
{
  if (domains & MYSERVER_SECURITY_CONF)
    {
      string strName (key);
      NodeTree<string>* ret = values.get (strName);

      if (ret)
        return ret;
    }

  if (mimeRecord && (domains & MYSERVER_MIME_CONF))
    {
      string strName (key);
      NodeTree<string>* ret = mimeRecord->getNodeTree (strName);

      if (ret)
        return ret;
    }

  if (vhost && (domains & MYSERVER_VHOST_CONF))
    {
      NodeTree<string>* ret = vhost->getNodeTree (key);

      if (ret)
        return ret;
    }

  if (server && (domains & MYSERVER_SERVER_CONF))
    {
      NodeTree<string>* ret = server->getNodeTree (key);

      if (ret)
        return ret;
    }

  return def;
}


/*!
  Get the value for the variable using the specified domains.
  \param name Variable name.
  \param def Default value.
  \param domains Domains where to look.  They are looked in this order:
  \li Security configuration file.
  \li Virtual host configuration file.
  \li Global security file.
  \li Default value.
*/
const char* SecurityToken::getData (const char* name, int domains,
                                    const char *def)
{
  if (domains & MYSERVER_SECURITY_CONF)
    {
      string strName (name);
      NodeTree<string> *ret = values.get (strName);

      if (ret)
        return ret->getValue ()->c_str ();
    }

  if (mimeRecord && (domains & MYSERVER_MIME_CONF))
    {
      string strName (name);
      const char *ret = mimeRecord->getData (strName);

      if (ret)
        return ret;
    }

  if (vhost && (domains & MYSERVER_VHOST_CONF))
    {
      const char* ret = vhost->getData (name);

      if (ret)
        return ret;
    }

  if (server && (domains & MYSERVER_SERVER_CONF))
    {
      const char* ret = server->getData (name);

      if (ret)
        return ret;
    }

  return def;
}
