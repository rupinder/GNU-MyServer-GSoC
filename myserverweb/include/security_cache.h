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
class security_cache
{
private:
  hash_dictionary dictionary;
  int limit;
public:
  security_cache();
  ~security_cache();
  void free();
  void setMaxNodes(int);
  int getMaxNodes();
  int getPermissionMask(char* user, char* password,char* directory,
                        char* filename,char *sysdirectory=0,char *password2=0,
                        char* auth_type=0,int len_auth=0,int *permission2=0);
};


#endif
