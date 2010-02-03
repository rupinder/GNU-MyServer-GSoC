/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2008, 2009, 2010 Free Software
  Foundation, Inc.
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

#ifndef SECURITY_MANAGER_H
# define SECURITY_MANAGER_H

# include "myserver.h"
# include <include/conf/security/security_token.h>
# include <include/connection/connection.h>
# include <include/base/xml/xml_parser.h>
# include <include/protocol/http/http_headers.h>
# include <include/base/hash_map/hash_map.h>

# include <string>

using namespace std;

class AuthMethod;
class Validator;
class SecurityDomain;
class AuthMethodFactory;
class ValidatorFactory;
class Vhost;
class Server;

/*!
 *Define permissions flags.
 */
enum PERMISSION_MASK
  {
    /*! No permissions.  */
    MYSERVER_PERMISSION_NONE      = 0,

    /*! Current user can read the file.  */
    MYSERVER_PERMISSION_READ     =      (1<<0),

    /*! Current user can write to the file.  */
    MYSERVER_PERMISSION_WRITE     = (1<<1),

    /*! Current user can execute the file.  */
    MYSERVER_PERMISSION_EXECUTE  = (1<<2),

    /*! Current user can remove the file.  */
    MYSERVER_PERMISSION_DELETE    = (1<<3),

    /*! Current user can browse the directory content.  */
    MYSERVER_PERMISSION_BROWSE    = (1<<4),

    /*! All permissions.  */
    MYSERVER_PERMISSION_ALL       = -1
  };

class SecurityManager
{
public:
  SecurityManager (ValidatorFactory*, AuthMethodFactory*);
  ~SecurityManager ();

  int getPermissionMask (SecurityToken* st,
                         SecurityDomain **domains,
                         string& validator,
                         string& authMethod);

  int getPermissionMask (SecurityToken* st,
                         list<SecurityDomain*> *domains,
                         string& validator,
                         string& authMethod);

private:
  int getValidatorAndAuthMethod (string &validatorName,
                                 string &authMethodName,
                                 Validator **validator,
                                 AuthMethod **authMethod);

  ValidatorFactory *validatorFactory;
  AuthMethodFactory *authMethodFactory;
};

#endif
