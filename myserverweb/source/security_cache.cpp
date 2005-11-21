/*
MyServer
Copyright (C) 2005 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "../include/security_cache.h"
#include "../include/security.h"
#include "../include/filemanager.h"

#include <string>

using namespace std;

/*!
 *Constructor for the SecurityCache object.
 */
SecurityCache::SecurityCache()
{
  /*!
   *By default do not store more than 25 nodes.
   */
  limit = 25;

}

/*!
 *Get the error file name from the security file.
 */
int SecurityCache::getErrorFileName(const char *directory, int error, 
                                    const char* sysdirectory, string& out)
{
  string permissionsFile;
  XmlParser *parser;
  if(directory == 0)
  {
    if(sysdirectory == 0)
      return -1;
    directory = sysdirectory;
    sysdirectory = 0;
  }
  permissionsFile.assign(directory);
  permissionsFile.append("/security");

  parser = dictionary.get(permissionsFile);
  /*!
   *If the parser is still present use it.
   */
  if(parser)
  {
    time_t fileModTime;
    /*! If the file was modified reload it. */
    fileModTime=File::getLastModTime(permissionsFile.c_str());
    if((fileModTime != static_cast<time_t>(-1))  && 
       (parser->getLastModTime() != fileModTime))
    {
      parser->close();
      if(parser->open(permissionsFile.c_str()) == -1)
      {
        dictionary.remove(permissionsFile);
        return -1;
      }
    }
    return sm.getErrorFileName(directory, error, out, parser);

  }
  else
  {
    /*! 
     *Create the parser and append at the dictionary.
     */
    parser = new XmlParser();
    if(parser == 0)
    {  
      return -1;
    }
   
    if(!File::fileExists(permissionsFile.c_str()))
    {
      /*!
       *If the security file doesn't exist try with a default one(the one in the system
       *directory).
       */
      if(sysdirectory!=0)
      {
        delete parser;
        return getErrorFileName(sysdirectory, error, 0, out);
      }
      else
      {
        delete parser;
        return -1;
      }
    }
    if(parser->open(permissionsFile.c_str()) == -1)
    {
      delete parser;
      return -1;
    }

    if(dictionary.size() >= limit)
    {
      XmlParser* toremove = dictionary.remove(dictionary.begin());
      if(toremove)
        delete toremove;
    }

		{
			XmlParser* old;	
			old=dictionary.put(permissionsFile, parser);
			if(old)
			{
				delete old;
			}
		}
    return sm.getErrorFileName(directory, error, out, parser);  
  }

  return 0;


}

/*!
 *Destroy the security cache object.
 */
SecurityCache::~SecurityCache()
{
  free();
}

/*!
 *free the memory used by the SecurityCache object.
 */
void SecurityCache::free()
{
	HashMap<string, XmlParser*>::Iterator it = dictionary.begin();
	HashMap<string, XmlParser*>::Iterator end = dictionary.end();

	for (;it != end; it++)
	{
		delete (*it);
	}

  dictionary.clear();
}

/*!
 *Set a new limit on the nodes to keep in memory.
 */
void SecurityCache::setMaxNodes(int newLimit)
{
  /*! Remove all the additional nodes from the dictionary. */
	
	while(newLimit < dictionary.size())
	{
		XmlParser* toremove = dictionary.remove(dictionary.begin());
		if(toremove)
			delete toremove;    
	}
  limit = newLimit;
}

/*!
 *Get the actual limit of open nodes.
 */
int SecurityCache::getMaxNodes()
{
  return limit;
}

/*!
 *Get the permission mask for the specified file and user. If the security file to use
 *is not loaded it will be loaded and added to the cache dictionary for faster future 
 *accesses.
 */
int SecurityCache::getPermissionMask(SecurityToken* st)
{
  XmlParser *parser;
  string permissionsFile;

  if(st->directory == 0)
    return -1;
  if(st->filename == 0)
    return -1;

  permissionsFile.assign(st->directory); 
  permissionsFile.append("/security");

  parser = dictionary.get(permissionsFile.c_str());
  /*!
   *If the parser is still present use it.
   */
  if(parser)
  {
    time_t fileModTime;
    /*! If the file was modified reload it. */
    fileModTime=File::getLastModTime(permissionsFile.c_str());
    if((fileModTime != static_cast<time_t>(-1))  && 
       (parser->getLastModTime() != fileModTime))
    {
      parser->close();
      if(parser->open(permissionsFile.c_str()) == -1)
      {
        dictionary.remove(permissionsFile.c_str());
        return -1;
      }

    }
    return sm.getPermissionMask(st, parser);
  }
  else
  {
    /*! 
     *Create the parser and append at the dictionary.
     */
    parser = new XmlParser();
    if(parser == 0)
    {  
      return -1;
    }
    if(!File::fileExists(permissionsFile.c_str()))
    {
      /*!
       *If the security file doesn't exist try with a default one.
       */
      if(st->sysdirectory!=0)
      {
        delete parser;
        st->directory = st->sysdirectory;
        st->sysdirectory = 0;
        return getPermissionMask(st);
      }
      else
      {
        delete parser;
        return -1;
      }
    }

    if(parser->open(permissionsFile.c_str()) == -1)
    {
      delete parser;
      return -1;
    }

    if(dictionary.size() >= limit)
    {
      XmlParser* toremove = dictionary.remove(dictionary.begin());
      if(toremove)
        delete toremove;
    }

		{
			XmlParser* old = dictionary.put(permissionsFile, parser);
			if(old)
			{
				delete old;
			}
		}
    return sm.getPermissionMask(st, parser);  
  }

  return 0;
}
