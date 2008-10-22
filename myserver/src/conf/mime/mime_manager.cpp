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
#include <include/base/xml/xml_parser.h>
#include <include/server/server.h>
#include <include/base/file/files_utility.h>

#include <string>
#include <algorithm>

#ifdef WIN32
#define MIME_LOWER_CASE
#endif

using namespace std;

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
int MimeRecord::addFilter(const char* n, int acceptDuplicate) 
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
 *Load the MIME types from a XML file. Returns the number of
 *MIME types loaded successfully.
 */
int MimeManager::loadXML(const char *fn)
{
  XmlParser parser;
  xmlNodePtr node;
  xmlDocPtr doc;
  int retSize;
  if(!fn)
    return -1;

  mutex.lock();

  if(filename)
    delete filename;

  filename = new string(fn);

  if(data)
    delete data;

  data = new HashMap<string, MimeRecord*>();

  if(parser.open (fn))
  {
    mutex.unlock ();
    return -1;
  }

  removeAllRecords();

  doc = parser.getDoc();
  node = doc->children->children;

  for(; node; node = node->next )
  {
    xmlNodePtr lcur = node->children;
    xmlAttr *attrs;
    MimeRecord rc;
    if(xmlStrcmp(node->name, (const xmlChar *)"MIME"))
      continue;

    rc.clear();

    for (attrs = node->properties; attrs; attrs = attrs->next)
    {
      if (!xmlStrcmp (attrs->name, (const xmlChar *)"handler") && 
          attrs->children && attrs->children->content)
        rc.cmdName.assign ((const char*)attrs->children->content);

      if (!xmlStrcmp (attrs->name, (const xmlChar *)"param") && 
          attrs->children && attrs->children->content)
        rc.cgiManager.assign ((const char*)attrs->children->content);
    }

    for( ;lcur; lcur = lcur->next)
    {
      if(lcur->name && !xmlStrcmp(lcur->name, (const xmlChar *)"EXTENSION"))
      {
        for (attrs = lcur->properties; attrs; attrs = attrs->next)
        {
          if (!xmlStrcmp (attrs->name, (const xmlChar *)"value") && 
              attrs->children && attrs->children->content)
            rc.extension.assign ((const char*)attrs->children->content);
        }
      }

      if(lcur->name && !xmlStrcmp(lcur->name, (const xmlChar *)"FILTER"))
      {
        if(lcur->children->content)
          rc.addFilter((const char*)lcur->children->content);
      }
    }

    if(addRecord(rc))
    {
      clean();
      mutex.unlock ();
      return 0;
    }
  }
  parser.close();
  
  /*! Store the loaded status. */
  loaded = 1;

  retSize = data->size();

  mutex.unlock ();

  return retSize;
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
    mutex.lock ();
    loaded = 0;
    if(filename) 
      delete filename;
    filename = 0;
    removeAllRecords();
    delete data;
    data = 0;
    mutex.unlock ();
  }
}

/*!
 *Constructor of the class.
 */
MimeManager::MimeManager()
{
  data = 0;
  filename = 0;
  loaded = 0;
  mutex.init ();
}

/*!
 *Add a new record. Returns zero on success.
 */
int MimeManager::addRecord(MimeRecord& mr)
{
  /*!
   *If the MIME type already exists remove it.
   */
  MimeRecord *nmr = 0;
  try
  {
    MimeRecord *old;

#ifdef MIME_LOWER_CASE
    transform(mr.extension.begin(), mr.extension.end(), mr.extension.begin(), ::tolower);
#endif
    nmr = new MimeRecord(mr);
    if(!nmr)  
      return 1;

    old = data->put(nmr->extension, nmr);
    if(old)
    {
      string error;
      error.assign("Warning: multiple MIME types registered for the extension " );
      error.append(nmr->extension);

      Server::getInstance()->logWriteln(error.c_str(), ERROR);
      delete old;
    }
  }
  catch(...)
  {
    if(nmr)
      delete nmr;
    return 1;
  };
  
  return 0;
}

/*!
 *Remove a record by the extension of the MIME type.
 */
void MimeManager::removeRecord(const string& ext)
{
  MimeRecord *rec = data->remove(ext.c_str());
  if(rec)
    delete rec;
}

/*!
 *Remove all records from the linked list.
 */
void MimeManager::removeAllRecords()
{
  HashMap<string, MimeRecord*>::Iterator it = data->begin();
  for(; it != data->end(); it++)
  {
    MimeRecord *rec = *it;
    if(rec)
      delete rec;
  }

  data->clear();
}

/*!
 *Get a pointer to an existing record passing its extension.
 */
MimeRecord *MimeManager::getMIME(const char *ext)
{
  string str (ext);
  return getMIME (str);
}

/*!
 *Get a pointer to an existing record passing its extension.
 */
MimeRecord *MimeManager::getMIME(string const &ext)
{
  MimeRecord* mr;

  mutex.lock ();

  mr = data ? data->get(ext.c_str()) : 0;

  mutex.unlock ();

  return mr;
}

/*!
 *Returns the number of MIME types loaded.
 */
u_long MimeManager::getNumMIMELoaded()
{
  u_long ret;

  mutex.lock ();

  ret = data ? data->size() : 0;

  mutex.unlock ();

  return ret;
}

/*!
 *Check if the MIME manager is loaded.
 */
int MimeManager::isLoaded()
{
  return loaded;
}
