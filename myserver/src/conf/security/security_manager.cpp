/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
  Free Software Foundation, Inc.
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

#include "myserver.h"

#include <include/conf/security/security_manager.h>
#include <include/conf/security/validator.h>
#include <include/conf/security/validator_factory.h>
#include <include/conf/security/auth_method.h>
#include <include/conf/security/auth_method_factory.h>
#include <include/conf/security/security_token.h>

#include <string>
#include <sstream>
#include <memory>

using namespace std;

SecurityManager::SecurityManager (ValidatorFactory* vf, AuthMethodFactory* amf)
{
  validatorFactory = vf;
  authMethodFactory = amf;
}

SecurityManager::~SecurityManager ()
{

}

/*!
  Get the permission mask for the requested resource.
 */
int SecurityManager::getPermissionMask (SecurityToken* st,
                                        SecurityDomain **domains,
                                        string &validator,
                                        string &authMethod)
{
  Validator *v;
  AuthMethod *am;

  if (getValidatorAndAuthMethod (validator, authMethod, &v, &am))
    return 0;

  return v->getPermissionMask (st, domains, am);
}

/*!
  Get the permission mask for the requested resource.
 */
int SecurityManager::getPermissionMask (SecurityToken* st,
                                        list<SecurityDomain*> *domains,
                                        string &validator,
                                        string &authMethod)
{
  Validator *v;
  AuthMethod *am;

  if (getValidatorAndAuthMethod (validator, authMethod, &v, &am))
    return 0;

  return v->getPermissionMask (st, domains, am);
}


/*!
  Initialize the pointers to the specified Validator and AuthMethod.
  \param validatorName The validator name to find.
  \param authMethodName The auth method name to find.
  \param validator Pointer to the Validator to initialize.
  \param authMethod Pointer to the AuthMethod initialize.
  \return 0 if the pointers were successfully initialized.
 */
int SecurityManager::getValidatorAndAuthMethod (string &validatorName,
                                                string &authMethodName,
                                                Validator **validator,
                                                AuthMethod **authMethod)
{

  string xml ("xml");

  *validator = validatorFactory->getValidator (validatorName);
  *authMethod = authMethodFactory->getAuthMethod (authMethodName);

  if (!(*validator))
    *validator = validatorFactory->getValidator (xml);

  if (!(*authMethod))
    *authMethod = authMethodFactory->getAuthMethod (xml);

  if (*validator && *authMethod)
    return 0;

  return 1;
}
