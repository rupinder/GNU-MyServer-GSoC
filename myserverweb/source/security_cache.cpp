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
 *Constructor for the security_cache object.
 */
security_cache::security_cache()
{
  /*!
   *By default store 25 nodes.
   */
  limit = 25;

}

/*!
 *Destroy the security cache object.
 */
security_cache::~security_cache()
{
  free();
}

/*!
 *Free the memory used by the security_cache object.
 */
void security_cache::free()
{
  int i;
  for(i=1; i <= dictionary.nodesNumber(); i++ )
  {
    cXMLParser *el =(cXMLParser*) dictionary.getData(i);
    if(el)
      delete el;
  }
  dictionary.free();
}

/*!
 *Set a limit on the nodes to keep in memory.
 */
void security_cache::setMaxNodes(int new_limit)
{
  limit = new_limit;
}

/*!
 *Get the actual limit of open nodes.
 */
int security_cache::getMaxNodes()
{
  return limit;
}

/*!
 *Get the permission mask for the specified file and user.
 */
int security_cache::getPermissionMask(char* user, char* password,char* directory,
                                      char* filename,char *sysdirectory,
                                      char *password2,char* auth_type,
                                      int len_auth,int *permission2)
{
  int permissionsFileLen = strlen(directory)+10;
  char* permissionsFile = new char[permissionsFileLen];
  if(permissionsFile == 0)
    return 0;
  sprintf(permissionsFile,"%s/security",directory);

  u_long filenamelen=(u_long)(strlen(filename));
  while(filenamelen && filename[filenamelen]=='.')
  {
    filename[filenamelen--]='\0';
  }
  cXMLParser *parser =(cXMLParser*)dictionary.getData(permissionsFile);
  /*!
   *If the parser is still present use it.
   */
  if(parser)
  {
    delete [] permissionsFile;
    return ::getPermissionMask(user, password, directory, filename, sysdirectory, 
                             password2, auth_type, len_auth, permission2, parser);
  }
  else
  {
    /*! 
     *Create the parser and append at the dictionary.
     */
    parser = new cXMLParser();
    if(parser == 0)
    {  
      delete [] permissionsFile;
      return 0;
    }
    if(!MYSERVER_FILE::fileExists(permissionsFile))
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
      cXMLParser* toremove =(cXMLParser*) dictionary.removeNodeAt(1);
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
