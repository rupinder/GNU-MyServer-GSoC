/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2005, 2006, 2008 Free Software Foundation, Inc.
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
#ifndef SECURITY_CACHE_H
#define SECURITY_CACHE_H
#include <include/base/hash_map/hash_map.h>
#include <include/conf/security/security.h>

#include <string>

using namespace std;

class SecurityCache
{
public:
  SecurityCache();
  ~SecurityCache();
  void free();
  void setMaxNodes(int);
  int getMaxNodes();
  XmlParser* getParser(const char* dir, const char* sys, bool useXpath = true);
	int getSecurityFile(const char* file, const char* sys, string& out);
  int getPermissionMask(SecurityToken* st);
  int getErrorFileName(const char *root, int error, 
                       const char* sysdirectory, string& out);
private:
  /*! Object used to handle security on the server. */
  SecurityManager sm;
  /*! Store a list of opened files using a hash dictionary. */
  HashMap<string, XmlParser*> dictionary;
  int limit;
};

#endif
