/*
  MyServer
  Copyright (C) 2005, 2006, 2008, 2009, 2010 Free Software Foundation,
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

#include <include/conf/security/security_cache.h>
#include <include/conf/security/security_manager.h>
#include <include/conf/security/auth_domain.h>
#include <include/base/file/file.h>
#include <include/base/file/files_utility.h>

#include <string>

using namespace std;

/*
  Constructor for the SecurityCache object.
*/
SecurityCache::SecurityCache ()
{
  /*
    By default do not store more than 25 nodes.
  */
  limit = 25;
}

/*!
  Destroy the security cache object.
 */
SecurityCache::~SecurityCache ()
{
  free ();
}

/*!
  Free the memory used by the SecurityCache object.
*/
void SecurityCache::free ()
{
  HashMap<string, CacheNode *>::Iterator it = dictionary.begin ();

  for (;it != dictionary.end (); it++)
    delete (*it);


  dictionary.clear ();

  list<CacheNode *>::iterator it2;
  for (it2 = toRemove.begin (); it2 != toRemove.end (); it2++)
    delete *it2;

  toRemove.clear ();
}

/*!
  Set a new limit on the nodes to keep in memory.
  \param newLimit Number of files to cache.  it is adjusted
  to be >= 1.
*/
void SecurityCache::setMaxNodes (int newLimit)
{
  if (newLimit <= 0)
    newLimit = 1;

  limit = newLimit;
}

/*!
  Get the security file to use starting from the file location, returns
  zero on success.
  \param dir The directory we need a security parser for.
  \param sys The system directory.
  \param out Output string where put the security file path.
  \param secFileName The security file name.
*/
int SecurityCache::getSecurityFile (const string& dir,
                                    const string& sys,
                                    string& out,
                                    const char* secFileName)
{
  int found = 0;
  string secFile;
  string file (dir);

  int i = file.length () - 1;

  while (i && dir[i] == '/')
    file.erase (i--, 1);

  secFile.assign (dir);
  secFile.append ("/");
  secFile.append (secFileName);

  /* The security file exists in the directory.  */
  if (FilesUtility::nodeExists (secFile))
    {
      out.assign (secFile);
      return 0;
    }


  /* Go upper in the tree till we find a security file.  */
  for (;;)
    {
      if (found || !file.length ())
        break;

      for (i = file.length () - 1; i; i--)
        if (file[i] == '/')
          {
            file.erase (i, file.length () - i);
            break;
          }

      /*
        Top of the tree, check if the security file is present in the
        system directory.  Return an error if it is not.
      */
      if (i == 0)
        {
          out.assign (sys);
          out.append ("/");
          out.append (secFileName);
          return !FilesUtility::nodeExists (out);
        }

      secFile.assign (file);
      secFile.append ("/");
      secFile.append (secFileName);

      found = FilesUtility::nodeExists (secFile);
    }

  out.assign (secFile);
  return 0;
}


/*!
  Get the actual limit of open nodes.
*/
int SecurityCache::getMaxNodes ()
{
  return limit;
}

/*!
  Open the XML parser associated to the file.
  \param dir The path where start looking.
  \param sys The system directory.
  \param useXpath Specify if XPath will be used on the file.
  \param secFileName The security file name.
  \param maxSize The maximum file size allowed for the security file.
*/
SecurityCache::CacheNode *SecurityCache::getParser (const string &dir,
                                                    const string &sys,
                                                    bool useXpath,
                                                    const char* secFileName,
                                                    u_long maxSize)
{
  XmlParser* parser;
  string file;
  CacheNode* node;

  if (getSecurityFile (dir, sys, file, secFileName))
    return NULL;

  mutex.lock ();
  try
    {
      node = dictionary.get (file);
      if (node)
        node->addRef ();
    }
  catch (...)
    {}
  mutex.unlock ();

  /* If the parser is already present and satisfy XPath then use it.  */
  if (node && (!useXpath || node->parser.isXpathEnabled ()))
    {
      time_t fileModTime;
      fileModTime = FilesUtility::getLastModTime (file.c_str ());
      parser = &(node->parser);
      if ((fileModTime != static_cast<time_t>(-1))  &&
          (parser->getLastModTime () != fileModTime))
        {
          parser->close ();

          /* FIXME:  Don't open the file twice, once to check
            and the second time to parse.  */
          if (maxSize)
            {
              File parserFile;
              if (parserFile.openFile (file.c_str (), File::READ))
                return NULL;

              if (parserFile.getFileSize () > maxSize)
                return NULL;

              parserFile.close ();
            }

          if (parser->open (file.c_str (), useXpath) == -1)
            return NULL;
        }
    }
  else
    {
      /* Create the parser and add it to the dictionary.  */
      CacheNode* old;
      node = new CacheNode (this);
      node->key = file;
      if (node->parser.open (file.c_str (), useXpath) == -1)
        {
          delete node;
          return NULL;
        }

      old = dictionary.put (file, node);
      if (old)
        toRemove.push_back (old);
    }

  return node;
}

void SecurityCache::mayDrop (CacheNode *cn)
{
  mutex.lock ();
  try
    {
      list<CacheNode *>::iterator it;
      for (it = toRemove.begin (); it != toRemove.end (); it++)
        {
          CacheNode *cn = *it;
          if (cn->getRef () == 0)
            {
              delete cn;
              toRemove.erase (it);
            }
        }

      if (dictionary.size () >= limit)
        {
          dictionary.remove (cn->key);
          toRemove.push_back (cn);
        }
    }
  catch (...)
    {}
  mutex.unlock ();
}

void SecurityCache::CacheNode::decRef ()
{
  if (--ref == 0)
    cache->mayDrop (this);
}
