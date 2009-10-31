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


#include <include/conf/security/auth_domain.h>
#include <include/conf/security/security_manager.h>


AuthDomain::AuthDomain (SecurityToken *st)
{
  this->name.assign ("auth");
  securityToken = st;
}

AuthDomain::~AuthDomain ()
{

}

/*!
 *\see SecurityDomain::getValue.
 */
string *AuthDomain::getValue (string &name)
{
  if (!name.compare ("user"))
    return &(securityToken->getUser ());

  if (!name.compare ("password"))
    return &(securityToken->getPassword ());

  if (!name.compare ("directory"))
    return securityToken->getDirectory ();

  if (!name.compare ("sysdirectory"))
    return securityToken->getSysDirectory ();

  return NULL;
}
