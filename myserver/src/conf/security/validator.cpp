/*
  MyServer
  Copyright (C) 2008, 2009 Free Software Foundation, Inc.
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

#include <include/conf/security/validator.h>
#include <include/conf/security/auth_domain.h>

Validator::Validator ()
{

}

Validator::~Validator ()
{

}


/*!
 *Get the permission mask for the requested resource.
 *Delegate the problem to getPermissionMaskInt.
 */
int Validator::getPermissionMask (SecurityToken* st,
                                  list<SecurityDomain*> *domains,
                                  AuthMethod* authMethod)
{
  HashMap<string, SecurityDomain*> hashedDomains;

  if (st->isDone ())
    return st->getMask ();

  if (domains)
  {
    for (list<SecurityDomain*>::iterator it = domains->begin ();
         it != domains->end (); it++)
    {
      addDomain (&hashedDomains, *it);
    }
  }

  return getPermissionMaskInt (st, &hashedDomains, authMethod);
}

/*!
 *Get the permission mask for the requested resource.
 *Delegate the problem to getPermissionMaskInt.
 */
int Validator::getPermissionMask (SecurityToken* st,
                                  SecurityDomain **domains,
                                  AuthMethod* authMethod)
{
  HashMap<string, SecurityDomain*> hashedDomains;

  if (st->isDone ())
    return st->getMask ();

  if (domains)
  {
    for (u_int i = 0; domains[i]; i++)
    {
      addDomain (&hashedDomains, domains[i]);
    }
  }

  return getPermissionMaskInt (st, &hashedDomains, authMethod);
}



/*!
 *Get the permission mask for the requested resource.
 *Decorate getPermissionMaskImpl.
 */
int Validator::getPermissionMaskInt (SecurityToken* st,
                                     HashMap<string, SecurityDomain*> *hashedDomains,
                                     AuthMethod* authMethod)
{
  int ret = 0;

  if (authMethod)
    ret = authMethod->getPermissionMask (st);

  if (!getPermissionMaskImpl (st, hashedDomains, authMethod))
    ret = 0;

  st->setDone (true);

  return ret;
}


/*!
 *Get the permission mask for the requested resource.
 */
int Validator::getPermissionMaskImpl (SecurityToken* st,
                                      HashMap<string, SecurityDomain*> *hashedDomains,
                                      AuthMethod* authMethod)
{
  return 0;
}

/*!
 *Get the value for [name] in the form domain.variable.
 *\param hashedDomains Registered security domains.
 *\param name Variable name.
 *\return The value of the requested variable.
 */
string *Validator::getValue (HashMap<string, SecurityDomain*> *hashedDomains, string &name)
{
  string domain;
  string var;
  size_t pos = name.find ('.');
  SecurityDomain *securityDomain = NULL;

  if (pos != string::npos)
  {
    domain = name.substr (0, pos);
    var = name.substr (pos + 1, string::npos);
  }
  else
  {
    domain.assign ("");
    var = name;
  }

  securityDomain = hashedDomains->get (domain);

  if (securityDomain)
    return securityDomain->getValue (var);

  return NULL;
}
