/*
*MyServer
*Copyright (C) 2005 The MyServer Team
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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "../include/security_cache.h"
#include "../include/security.h"
#include "../include/filemanager.h"

/*!
 *Constructor for the SecurityCache object.
 */
SecurityCache::SecurityCache()
{
  /*!
   *By default store 25 nodes.
   */
  limit = 25;

}

/*!
 *Get the error file name from the security file.
 */
int SecurityCache::getErrorFileName(char *directory, int error, 
                                     char* sysdirectory, char** out)
{
  int permissionsFileLen = strlen(directory)+10;
  char* permissionsFile = new char[permissionsFileLen];
  XmlParser *parser;
  if(permissionsFile == 0)
    return 0;
  sprintf(permissionsFile,"%s/security",directory);

  parser =(XmlParser*)dictionary.getData(permissionsFile);
  /*!
   *If the parser is still present use it.
   */
  if(parser)
  {
    u_long fileModTime;
    /*! If the file was modified reload it. */
    fileModTime=File::getLastModTime(permissionsFile);
    if((fileModTime != (u_long)-1)  && (parser->getLastModTime() != fileModTime))
    {
      parser->close();
      if(parser->open(permissionsFile) == -1)
      {
        dictionary.removeNode(permissionsFile);
        delete [] permissionsFile;
        return 0;
      }
    }
    delete [] permissionsFile;
    return ::getErrorFileName(directory, error, out, parser);

  }
  else
  {
    /*! 
     *Create the parser and append at the dictionary.
     */
    parser = new XmlParser();
    if(parser == 0)
    {  
      delete [] permissionsFile;
      return 0;
    }
   
    if(!File::fileExists(permissionsFile))
    {
      /*!
       *If the security file doesn't exist try with a default one.
       */
      if(sysdirectory!=0)
      {
        delete parser;
        delete [] permissionsFile;
        return getErrorFileName(directory, error, 0, out);
      }
      else
      {
        delete parser;
        delete [] permissionsFile;
        return 0;
      }
    }
    if(parser->open(permissionsFile) == -1)
    {
      delete parser;
      delete [] permissionsFile;
      return 0;
    }
    if(dictionary.nodesNumber() >= limit)
    {
      XmlParser* toremove =(XmlParser*) dictionary.removeNodeAt(1);
      if(toremove)
        delete toremove;
    }
    if(dictionary.append(permissionsFile, (void*)parser) == 0)
    {
      delete parser;
      delete [] permissionsFile;
      return 0;
    }
    delete [] permissionsFile;
    return ::getErrorFileName(directory, error, out, parser);  
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
 *Free the memory used by the SecurityCache object.
 */
void SecurityCache::free()
{
  int i;
  for(i=1; i <= dictionary.nodesNumber(); i++ )
  {
    XmlParser *el =(XmlParser*) dictionary.getData(i);
    if(el)
      delete el;
  }
  dictionary.free();
}

/*!
 *Set a limit on the nodes to keep in memory.
 */
void SecurityCache::setMaxNodes(int new_limit)
{
  limit = new_limit;
}

/*!
 *Get the actual limit of open nodes.
 */
int SecurityCache::getMaxNodes()
{
  return limit;
}

/*!
 *Get the permission mask for the specified file and user.
 */
int SecurityCache::getPermissionMask(char* user, char* password,char* directory,
                                      char* filename,char *sysdirectory,
                                      char *password2,char* auth_type,
                                      int len_auth,int *permission2)
{
  int permissionsFileLen = strlen(directory)+10;
  char* permissionsFile = new char[permissionsFileLen];
  XmlParser *parser;
  u_long filenamelen;
  if(permissionsFile == 0)
    return 0;
  sprintf(permissionsFile,"%s/security",directory);

  filenamelen=(u_long)(strlen(filename));
  while(filenamelen && filename[filenamelen]=='.')
  {
    filename[filenamelen--]='\0';
  }
  parser =(XmlParser*)dictionary.getData(permissionsFile);
  /*!
   *If the parser is still present use it.
   */
  if(parser)
  {
    u_long fileModTime;
    /*! If the file was modified reload it. */
    fileModTime=File::getLastModTime(permissionsFile);
    if((fileModTime != (u_long)-1)  && (parser->getLastModTime() != fileModTime))
    {
      parser->close();
      if(parser->open(permissionsFile) == -1)
      {
        dictionary.removeNode(permissionsFile);
        delete [] permissionsFile;
        return 0;
      }

    }
    delete [] permissionsFile;
    return ::getPermissionMask(user, password, directory, filename, sysdirectory, 
                             password2, auth_type, len_auth, permission2, parser);
  }
  else
  {
    /*! 
     *Create the parser and append at the dictionary.
     */
    parser = new XmlParser();
    if(parser == 0)
    {  
      delete [] permissionsFile;
      return 0;
    }
    if(!File::fileExists(permissionsFile))
    {
      /*!
       *If the security file doesn't exist try with a default one.
       */
      if(sysdirectory!=0)
      {
        delete parser;
        delete [] permissionsFile;
        return getPermissionMask(user, password, sysdirectory, filename, 0, password2,
                                 auth_type,len_auth,permission2);
      }
      else
      {
        delete parser;
        delete [] permissionsFile;
        return 0;
      }
    }
    if(parser->open(permissionsFile) == -1)
    {
      delete parser;
      delete [] permissionsFile;
      return 0;
    }
    if(dictionary.nodesNumber() >= limit)
    {
      XmlParser* toremove =(XmlParser*) dictionary.removeNodeAt(1);
      if(toremove)
        delete toremove;
    }
    if(dictionary.append(permissionsFile, (void*)parser) == 0)
    {
      delete parser;
      delete [] permissionsFile;
      return 0;
    }
    delete [] permissionsFile;
    return ::getPermissionMask(user, password, directory, filename, sysdirectory, 
                             password2, auth_type, len_auth, permission2, parser);  
  }

  return 0;
}
