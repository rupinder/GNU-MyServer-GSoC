/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2005, 2006, 2008, 2009 Free Software Foundation, Inc.
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
# define SECURITY_CACHE_H

# include "stdafx.h"

# include <include/base/hash_map/hash_map.h>
# include <include/conf/security/security_manager.h>

# include <include/conf/security/auth_method_factory.h>
# include <include/conf/security/auth_method.h>
# include <include/conf/security/validator_factory.h>
# include <include/conf/security/validator.h>
# include <include/conf/security/xml_validator.h>

# include <string>

using namespace std;

class SecurityCache
{

public:
  SecurityCache ();
  ~SecurityCache ();
  void free ();
  void setMaxNodes (int);
  int getMaxNodes ();

  XmlParser* getParser (const string &dir, const string &sys, bool useXpath = true,
                        const char* secName = ".security.xml", u_long maxSize = 0);
  int getSecurityFile (const string &file, const string &sys,
                       string &out, const char* secName = ".security.xml");
  int getErrorFileName (const char *root, int error,
                        const char* sysdirectory, string& out){return 0;}
private:

  /* Store a list of opened files using a hash dictionary.  */
  HashMap<string, XmlParser*> dictionary;
  int limit;
};

#endif
