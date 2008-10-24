/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008 Free Software Foundation, Inc.
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
#define MIME_LOWER_CASE
#endif

using namespace std;

MimeRecord::MimeRecord()
{
  filters.clear();
  extension.assign(""); 
  mimeType.assign("");
  cgiManager.assign("");
  cmdName.assign("");
}

/*!
 *Destroy the object.
 */
MimeRecord::~MimeRecord()
{
  clear();
}

/*!
 *Add a filter to the list of filters to apply on this MIME type.
 *Return zero if the filters was not added.
 */
int MimeRecord::addFilter(const char* n, bool acceptDuplicate) 
{
  if(!acceptDuplicate)
  {
    list<string>::iterator i = filters.begin();
    for( ; i != filters.end() ;i++ )
    {
      if(!stringcmpi(*i, n))
        return 0;
    }
  }
  filters.push_back(n);
  return 1;
}

/*!
 *Copy constructor.
 */
MimeRecord::MimeRecord(MimeRecord& m)
{
  list<string>::iterator i = m.filters.begin();

  filters.clear(); 

  for( ; i != m.filters.end(); i++)
  {
    filters.push_back(*i);
  }
  extension.assign(m.extension); 
  mimeType.assign(m.mimeType);
  cmdName.assign(m.cmdName);
  cgiManager.assign(m.cgiManager);
} 

/*!
  *Clear the used memory.
  */
void MimeRecord::clear()
{
  filters.clear();
  extension.assign(""); 
  mimeType.assign(""); 
  cmdName.assign("");
  cgiManager.assign("");
}  

/*!
 *Get the name of the file opened by the class.
 */
const char *MimeManager::getFilename()
{
  if(filename == 0)
    return "";
  return filename->c_str();
}

/*!
 *Read a MIME record from a XML node.
 */
MimeRecord *MimeManager::readRecord (xmlNodePtr node)
{
  xmlNodePtr lcur = node->children;
  xmlAttr *attrs;

  if(xmlStrcmp(node->name, (const xmlChar *)"MIME"))
    return NULL;


  MimeRecord *rc = new MimeRecord;

  for (attrs = node->properties; attrs; attrs = attrs->next)
  {
    if (!xmlStrcmp (attrs->name, (const xmlChar *)"handler") && 
        attrs->children && attrs->children->content)
      rc->cmdName.assign ((const char*)attrs->children->content);
    
    if (!xmlStrcmp (attrs->name, (const xmlChar *)"param") && 
        attrs->children && attrs->children->content)
      rc->cgiManager.assign ((const char*)attrs->children->content);
  }

  for( ;lcur; lcur = lcur->next)
  {
    if(lcur->name && !xmlStrcmp(lcur->name, (const xmlChar *)"EXTENSION"))
    {
      for (attrs = lcur->properties; attrs; attrs = attrs->next)
      {
        if (!xmlStrcmp (attrs->name, (const xmlChar *)"value") && 
            attrs->children && attrs->children->content)
          rc->extension.assign ((const char*)attrs->children->content);
      }
    }

    if(lcur->name && !xmlStrcmp(lcur->name, (const xmlChar *)"FILTER"))
    {
      if(lcur->children->content)
        rc->addFilter((const char*)lcur->children->content);
    }
  }

  return rc;
}

/*!
 *Load the MIME types from a XML file. Returns the number of
 *MIME types loaded successfully.
 */
int MimeManager::loadXML(const char *fn)
{
  XmlParser parser;
  xmlNodePtr node;
  xmlDocPtr doc;

  if(!fn)
    return -1;

  if(filename)
    delete filename;

  filename = new string(fn);

  if(parser.open (fn))
  {
    return -1;
  }

  removeAllRecords();

  /* The first record is not used to store information.  */
  records.push_back (NULL);

  doc = parser.getDoc();
  node = doc->children->children;

  for(; node; node = node->next )
  {
    MimeRecord *rc = readRecord (node);

    if (rc)
      addRecord (rc);
  }
  parser.close();
  
  /*! Store the loaded status. */
  loaded = true;

  return getNumMIMELoaded ();
}

/*!
 *Destroy the object.
 */
MimeManager::~MimeManager()
{
  clean();
}

/*!
 *Clean the memory allocated by the structure.
 */
void MimeManager::clean()
{
  if(loaded)
  {
    loaded = false;
    if(filename) 
      delete filename;
    filename = 0;
    removeAllRecords();
  }
}

/*!
 *Constructor of the class.
 */
MimeManager::MimeManager()
{
  filename = 0;
  loaded = false;
}

/*!
 *Add a new record.
 *\return Return the position for the new record.
 */
int MimeManager::addRecord (MimeRecord *mr)
{
  u_long position = records.size ();

#ifdef MIME_LOWER_CASE
  transform (mr->extension.begin(), mr->extension.end(), mr->extension.begin(), ::tolower);
#endif
  
  records.push_back (mr);

  extIndex.put (mr->extension, position);
  
  return position;
}

/*!
 *Remove all records from the linked list.
 */
void MimeManager::removeAllRecords ()
{
  vector <MimeRecord*>::iterator i = records.begin ();

  while (i != records.end ())
  {
    MimeRecord *r = *i;

    if (r)
      delete r;
    
    i++;
  }

  records.clear ();

  extIndex.clear ();
}

/*!
 *Get a pointer to a MIME record by its extension.
 */
MimeRecord *MimeManager::getMIME (const char *ext)
{
  string str (ext);
  return getMIME (str);
}

/*!
 *Get a pointer to an existing record passing its extension.
 */
MimeRecord *MimeManager::getMIME (string const &ext)
{
  u_long pos = extIndex.get (ext.c_str ());

  if (pos)
    return records [pos];

  return NULL;
}

/*!
 *Returns the number of MIME types loaded.
 */
u_long MimeManager::getNumMIMELoaded ()
{
  return records.size () -1;
}

/*!
 *Check if the MIME manager is loaded.
 */
bool MimeManager::isLoaded ()
{
  return loaded;
}
