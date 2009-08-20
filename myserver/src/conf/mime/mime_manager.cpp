/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008, 2009 Free Software Foundation, Inc.
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
 * Destroy the object.
 */
MimeRecord::~MimeRecord ()
{
  clear ();
}

/*!
 * Add a filter to the list of filters to apply on this MIME type.
 * Return zero if the filters was not added.
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
 * Copy constructor.
 */
MimeRecord::MimeRecord (MimeRecord& m)
{
  list<string>::iterator i = m.filters.begin ();

  filters.clear ();

  for( ; i != m.filters.end (); i++)
  {
    filters.push_back (*i);
  }

  i = m.extensions.begin ();

  extensions.clear ();

  for( ; i != m.extensions.end (); i++)
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

  HashMap<string, string*>::Iterator it = hashedData.begin ();
  for (;it != hashedData.end (); it++)
  {
    delete (*it);
  }
  hashedData.clear ();


  pathRegex.clear ();
}

/*!
 * Get the value stored in the hash dictionary for the `name' key.
 */
const char* MimeRecord::getHashedData(string &name)
{
  string *str = hashedData.get (name);
  if (str)
    return str->c_str ();

  return NULL;
}

/*!
 * Get the name of the used file.
 */
const char *MimeManager::getFilename ()
{
  return filename.c_str ();
}

/*!
 * Read a MIME record from a XML node.
 */
MimeRecord *MimeManager::readRecord (xmlNodePtr node)
{
  xmlNodePtr lcur = node->children;
  xmlAttr *attrs;

  MimeRecord *rc = new MimeRecord;

  for (attrs = node->properties; attrs; attrs = attrs->next)
  {
    if (!xmlStrcmp (attrs->name, (const xmlChar *)"handler") &&
        attrs->children && attrs->children->content)
      rc->cmdName.assign ((const char*)attrs->children->content);

    if (!xmlStrcmp (attrs->name, (const xmlChar *)"self") &&
        attrs->children && attrs->children->content)
      rc->selfExecuted = xmlStrcmp (attrs->children->content,
                                    (const xmlChar *)"YES");

    if (!xmlStrcmp (attrs->name, (const xmlChar *)"param") &&
        attrs->children && attrs->children->content)
      rc->cgiManager.assign ((const char*)attrs->children->content);
  }

  for ( ;lcur; lcur = lcur->next)
  {
    if (lcur->name && !xmlStrcmp(lcur->name, (const xmlChar *)"EXTENSION"))
    {
      for (attrs = lcur->properties; attrs; attrs = attrs->next)
      {
        if (!xmlStrcmp (attrs->name, (const xmlChar *)"value") &&
            attrs->children && attrs->children->content)
        {
          string ext ((const char*)attrs->children->content);
          rc->extensions.push_back (ext);
        }
      }
    }

    if (lcur->name && !xmlStrcmp(lcur->name, (const xmlChar *)"DEFINE"))
    {
      const char *name = NULL;
      const char *value = NULL;

      for (attrs = lcur->properties; attrs; attrs = attrs->next)
      {
        if (!xmlStrcmp (attrs->name, (const xmlChar *)"name") &&
            attrs->children && attrs->children->content)
          name = (const char*)attrs->children->content;

        if (!xmlStrcmp (attrs->name, (const xmlChar *)"value") &&
            attrs->children && attrs->children->content)
          value = (const char*)attrs->children->content;
      }

      if (name && value)
      {
        string key (name);
        string *v = new string (value);
        rc->hashedData.put(key, v);
      }

    }

    if (lcur->name && !xmlStrcmp(lcur->name, (const xmlChar *)"PATH"))
    {
      for (attrs = lcur->properties; attrs; attrs = attrs->next)
      {
        if (!xmlStrcmp (attrs->name, (const xmlChar *)"regex") &&
            attrs->children && attrs->children->content)
        {
          Regex *r = new Regex;

          if (r->compile ((const char*)attrs->children->content, 0))
            return NULL;

          rc->pathRegex.push_back (r);
        }
      }
    }

    if (lcur->name && !xmlStrcmp(lcur->name, (const xmlChar *)"FILTER"))
    {
      for (attrs = lcur->properties; attrs; attrs = attrs->next)
      {
        if (!xmlStrcmp (attrs->name, (const xmlChar *)"value") &&
            attrs->children && attrs->children->content)
          rc->addFilter ((const char*)attrs->children->content);
      }
    }
  }

  return rc;
}

/*!
 * Load the MIME types from a XML file. Returns the number of
 * MIME types loaded successfully.
 */
u_long MimeManager::loadXML (const char *fn)
{
  XmlParser parser;
  u_long ret = 0;

  if(parser.open (fn))
  {
    return -1;
  }

  filename.assign (fn);

  ret = loadXML (&parser);

  parser.close ();

  return ret;
}

/*!
 * Load the MIME types from a XML parser object. Returns the number
 * of MIME types loaded successfully.
 */
u_long MimeManager::loadXML (XmlParser* parser)
{
  xmlNodePtr node;
  xmlDocPtr doc;

  clearRecords ();

  doc = parser->getDoc ();
  node = doc->children->children;

  for (; node; node = node->next)
  {
    if (xmlStrcmp (node->name, (const xmlChar *)"MIME"))
      continue;

    MimeRecord *rc = readRecord (node);

    if (rc)
      addRecord (rc);
  }

  /*! Store the loaded status. */
  loaded = true;

  return getNumMIMELoaded ();
}

/*!
 * Destroy the object.
 */
MimeManager::~MimeManager ()
{
  clean();
}

/*!
 * Clean the memory allocated by the structure.
 */
void MimeManager::clean ()
{
  if(loaded)
  {
    loaded = false;
    filename.assign ("");
    clearRecords ();
  }
}

/*!
 * Constructor of the class.
 */
MimeManager::MimeManager ()
{
  loaded = false;
}

/*!
 * Add a new record.
 * \return Return the position for the new record.
 */
int MimeManager::addRecord (MimeRecord *mr)
{
  u_long position = records.size ();

  records.push_back (mr);

  for (list<string>::iterator it = mr->extensions.begin ();
       it != mr->extensions.end (); it++)
    {
      string &ext = *it;

#ifdef MIME_LOWER_CASE
      transform (ext.begin(), ext.end(), ext.begin(), ::tolower);
#endif
      extIndex.put (ext, position);
    }

  for (list<Regex*>::iterator it = mr->pathRegex.begin ();
       it != mr->pathRegex.end (); it++)
    {
      PathRegex *pr = new PathRegex;
      pr->regex = *it;
      pr->record = position;

      pathRegex.push_back (pr);
    }

  return position;
}

/*!
 * Remove all the stored records.
 */
void MimeManager::clearRecords ()
{
  vector <MimeRecord*>::iterator i = records.begin ();

  while (i != records.end ())
    {
      MimeRecord *r = *i;
      if (r)
        delete r;
      i++;
    }

  for (list<PathRegex*>::iterator it = pathRegex.begin ();
       it != pathRegex.end (); it++)
    delete *it;


  for (HashMap<string, MimeManagerHandler*>::Iterator it = handlers.begin ();
       it != handlers.end (); it++)
    delete *it;

  pathRegex.clear ();
  records.clear ();

  extIndex.clear ();

  /* The first record is not used to store information.  */
  records.push_back (NULL);
}

/*!
 * Get the MIME type to use on the specified file.
 * \param filename Find the MIME type for this file.
 * \param handler If specified, indicate an external handler to use
 * if the mime cannot be found locally.
 */
MimeRecord *MimeManager::getMIME (string const &filename, const char *handler)
{
  return getMIME (filename.c_str (), handler);
}

/*!
 * Get the MIME type to use on the specified file.
 * \param filename Find the MIME type for this file.
 * \param handler If specified, indicate an external handler to use
 * if the mime cannot be found locally.
 */
MimeRecord *MimeManager::getMIME (const char *filename, const char *handler)
{
  string ext;
  u_long pos = 0;

  for (list<PathRegex*>::iterator it = pathRegex.begin ();
       it != pathRegex.end ();
       it++)
  {
    PathRegex *pr = *it;
    regmatch_t pm;

    if (pr->regex->exec (filename, 1, &pm, 0) == 0)
    {
      pos = pr->record;
      break;
    }
  }

  if (pos == 0)
    {
      FilesUtility::getFileExt (ext, filename);
      pos = extIndex.get (ext.c_str ());
    }

  if (pos)
    return records [pos];

  if (handler)
    {
      MimeManagerHandler *h = handlers.get (handler);
      if (h)
        return h->getMIME (filename);
    }

  return NULL;
}

/*!
 * Register an external handler under the specified name.
 *
 * \param name Handler name.
 * \param handler Handler object to register.
 */
void MimeManager::registerHandler (string &name, MimeManagerHandler *handler)
{
  MimeManagerHandler *old = handlers.put (name, handler);
  if (old)
    delete old;
}

/*!
 * Returns the number of MIME types loaded.
 */
u_long MimeManager::getNumMIMELoaded ()
{
  return records.size () -1;
}

/*!
 * Check if the MIME manager is loaded.
 */
bool MimeManager::isLoaded ()
{
  return loaded;
}
