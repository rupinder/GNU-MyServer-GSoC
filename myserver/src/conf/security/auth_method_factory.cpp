/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009 Free
  Software Foundation, Inc.
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

#include "stdafx.h"

#include <include/conf/security/auth_method_factory.h>
#include <include/conf/security/auth_method.h>

#include <string>

using namespace std;

AuthMethodFactory::AuthMethodFactory (CryptAlgoManager *cryptAlgoManager)
{
  this->cryptAlgoManager = cryptAlgoManager;
}

AuthMethodFactory::~AuthMethodFactory ()
{
  authMethods.clear ();
}

/*!
  Return an AuthMethod given its name.
*/
AuthMethod* AuthMethodFactory::getAuthMethod (string &name)
{
  return authMethods.get (name);
}

/*!
  Add a new AuthMethod to the factory.
  \param name AuthMethod name.
  \param authMethod The authMethod to add.
  \return The old authMethod registered with [name], in any.
*/
AuthMethod* AuthMethodFactory::addAuthMethod (string &name,
                                              AuthMethod* authMethod)
{
  authMethod->setCryptAlgoManager (cryptAlgoManager);
  return authMethods.put (name, authMethod);
}

/*!
  Check if the specified authMethod is present in the factory.
  \param name The authMethod name.
  \return a bool value to indicate if it is present or not.
*/
bool AuthMethodFactory::isAuthMethodPresent (string &name)
{
  return getAuthMethod (name) != NULL;
}
