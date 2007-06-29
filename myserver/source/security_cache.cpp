/*
MyServer
Copyright (C) 2005, 2006 The MyServer Team
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

#include "../include/security_cache.h"
#include "../include/security.h"
#include "../include/file.h"
#include "../include/files_utility.h"

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

	for (;it != dictionary.end(); it++)
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
 *Get the security file to use starting from the file location, returns
 *zero on success.
 *\param dir The directory we need a security parser for.
 *\param sys The system directory.
 *\param out Output string where put the security file path. 
 */
int SecurityCache::getSecurityFile(const char* dir, const char* sys, 
																	 string& out)
{
	int found = 0;
	string file(dir);
	string secFile;
	int i = file.length() - 1;

	while(i && file[i] == '/')
		file.erase(i--, 1);

	secFile.assign(file);
	secFile.append("/security");

	/* The security file exists in the directory.  */
	if(FilesUtility::fileExists(secFile))
  {
		out.assign(secFile);
		return 0;
	}
	
	/* Go upper in the tree till we find a security file.  */
	do
	{
		for(i = file.length() - 1; i; i--)
			if(file[i] == '/')
			{
				file.erase(i, file.length() - i);
				break;
			}

		/* 
		 *Top of the tree, check if the security file is present in the 
		 *system directory, returns an error if it is not.
		 */
		if(i == 0)
		{
			out.assign(sys);
			out.append("/security");
			return !FilesUtility::fileExists(out);
		}
		secFile.assign(file);
		secFile.append("/security");

	}
	while(!(found = FilesUtility::fileExists(secFile)));

	out.assign(secFile);
	return 0;
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
	string permissionFile;
  if(st->directory == 0)
    return -1;
  if(st->filename == 0)
    return -1;

	if(getSecurityFile(st->directory, st->sysdirectory, permissionFile))
		return -1;

  parser = dictionary.get(permissionFile.c_str());

  /*!
   *If the parser is already present use it.
   */
  if(parser)
  {
    time_t fileModTime;
    /*! If the file was modified reload it. */
    fileModTime = FilesUtility::getLastModTime(permissionFile.c_str());
    if((fileModTime != static_cast<time_t>(-1))  && 
       (parser->getLastModTime() != fileModTime))
    {
      parser->close();
      if(parser->open(permissionFile.c_str()) == -1)
      {
        dictionary.remove(permissionFile.c_str());
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
		XmlParser* old;
    parser = new XmlParser();
    if(parser == 0)
    {  
      return -1;
    }

    if(dictionary.size() >= limit)
    {
      XmlParser* toremove = dictionary.remove(dictionary.begin());
      if(toremove)
        delete toremove;
    }

		if(parser->open(permissionFile.c_str()) == -1)
			return -1;

		old = dictionary.put(permissionFile, parser);
		if(old)
		{
			delete old;
		}
		return sm.getPermissionMask(st, parser);  
  }

  return 0;
}

/*!
 *Get the error file name from the security file.
 */
int SecurityCache::getErrorFileName(const char *directory, int error, 
                                    const char* sysdirectory, string& out)
{
  XmlParser *parser;
	string permissionFile;
	out.assign("");
  if(directory == 0)
    return -1;

	if(getSecurityFile(directory, sysdirectory, permissionFile))
		return -1;

  parser = dictionary.get(permissionFile.c_str());

  /*!
   *If the parser is already present use it.
   */
  if(parser)
  {
    time_t fileModTime;
    /*! If the file was modified reload it. */
    fileModTime = FilesUtility::getLastModTime(permissionFile.c_str());
    if((fileModTime != static_cast<time_t>(-1))  && 
       (parser->getLastModTime() != fileModTime))
    {
      parser->close();
      if(parser->open(permissionFile.c_str()) == -1)
      {
        dictionary.remove(permissionFile.c_str());
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
		XmlParser* old;
    parser = new XmlParser();
    if(parser == 0)
    {  
      return -1;
    }

    if(dictionary.size() >= limit)
    {
      XmlParser* toremove = dictionary.remove(dictionary.begin());
      if(toremove)
        delete toremove;
    }

		if(parser->open(permissionFile.c_str()) == -1)
			return -1;

		old = dictionary.put(permissionFile, parser);
		if(old)
		{
			delete old;
		}
    return sm.getErrorFileName(directory, error, out, parser);  
  }

  return 0;
}
