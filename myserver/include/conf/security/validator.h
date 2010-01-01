/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2008, 2009, 2010 Free Software Foundation, Inc.
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

#ifndef VALIDATOR_H
# define VALIDATOR_H

# include "stdafx.h"
# include <include/base/hash_map/hash_map.h>

# include <include/conf/security/auth_method.h>
# include <include/conf/security/security_domain.h>
# include <include/conf/security/security_manager.h>

# include <list>
using namespace std;



class Validator
{
public:
  Validator ();
  virtual ~Validator ();

  int getPermissionMask (SecurityToken* st,
                         SecurityDomain **domains,
                         AuthMethod* authMethod);

  int getPermissionMask (SecurityToken* st,
                         list<SecurityDomain*> *domains,
                         AuthMethod* authMethod);

  virtual int getPermissionMaskImpl (SecurityToken* st,
                                     HashMap<string, SecurityDomain*> *hashedDomains,
                                     AuthMethod* authMethod);


  string *getValue (HashMap<string, SecurityDomain*> *hashedDomains,
                    string &name);
protected:
  bool comparePassword (const char *password, const char *savedPassword,
                        const char *algorithm);

  inline void addDomain (HashMap<string, SecurityDomain*> *hashedDomains,
                         SecurityDomain *domain)
  {
    string &name = domain->getName ();
    hashedDomains->put (name, domain);
  }

  int getPermissionMaskInt (SecurityToken* st,
                            HashMap<string, SecurityDomain*> *hashedDomains,
                            AuthMethod* authMethod);
};

#endif
