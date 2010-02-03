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

#ifndef AUTH_METHOD_FACTORY_H
# define AUTH_METHOD_FACTORY_H

# include "myserver.h"
# include <include/base/hash_map/hash_map.h>
# include <string>
# include <include/base/crypt/crypt_algo_manager.h>

using namespace std;

class AuthMethod;


class AuthMethodFactory
{
public:
  AuthMethodFactory (CryptAlgoManager *cryptAlgoManager);
  virtual ~AuthMethodFactory ();
  AuthMethod* getAuthMethod (string &name);
  AuthMethod* addAuthMethod (string &name, AuthMethod* authMethod);
  bool isAuthMethodPresent (string &name);

private:
  CryptAlgoManager *cryptAlgoManager;
  HashMap<string, AuthMethod*> authMethods;
};
#endif
