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
#ifndef SECURITY_CACHE_H
#define SECURITY_CACHE_H
#include "../include/hash_dictionary.h"
#include "../include/security.h"

class SecurityCache
{
private:
  /*! Object used to handle security on the server. */
  SecurityManager sm;
  /*! Store a list of opened files using a hash dictionary. */
  HashDictionary dictionary;
  int limit;
public:
  SecurityCache();
  ~SecurityCache();
  void free();
  void setMaxNodes(int);
  int getMaxNodes();
  int getPermissionMask(char* user, char* password,char* directory,
                        char* filename,char *sysdirectory=0,char *password2=0,
                        int *permission2=0, SecurityToken* st=0);
  int getErrorFileName(char *root,int error, char* sysdirectory, char** out);
};


#endif
