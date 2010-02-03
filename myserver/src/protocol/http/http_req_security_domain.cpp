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

#include <include/protocol/http/http_req_security_domain.h>
#include <include/conf/security/security_domain.h>

/*!
 *C'tor.
 *\param request The HTTP Request object to use.
 */
HttpReqSecurityDomain::HttpReqSecurityDomain (HttpRequestHeader *request)
  : SecurityDomain ("http_request")
{
  this->request = request;
}

HttpReqSecurityDomain::~HttpReqSecurityDomain ()
{

}

/*!
 *\see SecurityDomain#getValue.
 */
string *HttpReqSecurityDomain::getValue (string &name)
{
  return request->getValue (name.c_str (), NULL);
}
