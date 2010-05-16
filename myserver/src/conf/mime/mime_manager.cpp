/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008, 2009, 2010 Free
  Software Foundation, Inc.
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
#include <include/conf/mime/mime_manager.h>
#include <include/base/file/file.h>
#include <include/base/string/stringutils.h>
#include <include/server/server.h>
#include <include/base/file/files_utility.h>

#include <string>
#include <algorithm>

#ifdef WIN32
# define MIME_LOWER_CASE
#endif

using namespace std;

MimeRecord::MimeRecord ()
{
  filters.clear ();
  extensions.clear ();
  mimeType.assign ("");
  cgiManager.assign ("");
  cmdName.assign ("");
  selfExecuted = false;
}

/*!
  Destroy the object.
 */
MimeRecord::~MimeRecord ()
{
  clear ();
}

/*!
  Add a filter to the list of filters to apply on this MIME type.
  Return zero if the filters was not added.
 */
int MimeRecord::addFilter (const char* n, bool acceptDuplicate)
{
  if (!acceptDuplicate)
  {
    list<string>::iterator i = filters.begin ();
    for (; i != filters.end (); i++)
    {
      if (!stringcmpi (*i, n))
        return 0;
    }
  }

  filters.push_back (n);
  return 1;
}

/*!
  Copy constructor.
 */
MimeRecord::MimeRecord (MimeRecord& m)
{
  list<string>::iterator i = m.filters.begin ();

  filters.clear ();

  for ( ; i != m.filters.end (); i++)
  {
    filters.push_back (*i);
  }

  i = m.extensions.begin ();

  extensions.clear ();

  for ( ; i != m.extensions.end (); i++)
  {
    filters.push_back (*i);
  }

  selfExecuted = m.selfExecuted;
  mimeType.assign (m.mimeType);
  cmdName.assign (m.cmdName);
  cgiManager.assign (m.cgiManager);
}

/*!
  *Clear the used memory.
  */
void MimeRecord::clear ()
{
  filters.clear ();
  extensions.clear ();
  mimeType.assign ("");
  cmdName.assign ("");
  cgiManager.assign ("");

  for (list<Regex*>::iterator it = pathRegex.begin ();
       it != pathRegex.end ();
       it++)
  {
    delete *it;
  }

  HashMap<string, NodeTree<string>*>::Iterator it = hashedData.begin ();
  for (;it != hashedData.end (); it++)
    delete (*it);
  hashedData.clear ();

  pathRegex.clear ();
}

/*!
  Get the node tree stored in the hash dictionary for the `name' key.
 */
NodeTree<string>* MimeRecord::getNodeTree (string &name)
{
  return hashedData.get (name);
}

/*!
  Get the value stored in the hash dictionary for the `name' key.
 */
const char* MimeRecord::getData (string &name)
{
  NodeTree<string> *str = hashedData.get (name);
  if (str)
    return str->getValue ()->c_str ();

  return NULL;
}

/*!
  Reload using the same configuration file.
 */
u_long MimeManager::reload ()
{
 HashMap<string, MimeManagerHandler*>::Iterator it = handlers.begin ();
  for (;it != handlers.end (); it++)
    (*it)->reload ();

  return 0;
}

MimeManager::~MimeManager ()
{
  clean ();
}

/*!
  Clean the memory allocated by the structure.
 */
void MimeManager::clean ()
{

}

/*!
  Constructor of the class.
 */
MimeManager::MimeManager ()
{
  defHandler = NULL;
}

/*!
  Get the MIME record using the registered handlers.
 */
MimeRecord *MimeManager::getMIME (const char *filename, const char *handler)
{
  if (handler)
    {
      MimeManagerHandler *h = handlers.get (handler);
      if (h)
        return h->getMIME (filename);
    }

  if (defHandler)
    return defHandler->getMIME (filename);

  return NULL;
}

/*!
  Register an external handler under the specified name.

  \param name Handler name.
  \param handler Handler object to register.
 */
void MimeManager::registerHandler (string &name, MimeManagerHandler *handler)
{
  MimeManagerHandler *old = handlers.put (name, handler);
  if (old)
    delete old;
}

/*!
  Set the default handler specifying its name.

  \param name The default handler name.
 */
void MimeManager::setDefaultHandler (string &name)
{
  defHandler = handlers.get (name);
}

/*!
  Register a builder function for a mime manager.
  \param name manager name.
  \param builder Builder routine.
 */
void MimeManager::registerBuilder (string &name, MAKE_HANDLER builder)
{
  builders.put (name, builder);
}

/*!
  Build an handler given its name.

  \param name handler name.
  \return an instance of the requested handler type.
 */
MimeManagerHandler *MimeManager::buildHandler (string &name)
{
  MAKE_HANDLER builder = builders.get (name);

  if (builder)
    return builder ();

  return NULL;
}
