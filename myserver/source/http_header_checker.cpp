/*
MyServer
Copyright (C) 2005 The MyServer Team
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


#include "../include/http.h"
#include "../include/http_headers.h"
#include "../include/http_header_checker.h"

#include <string>
#include <ostream>

using namespace std;

extern "C" 
{
#ifdef WIN32
#include <direct.h>
#include <errno.h>
#endif

#ifdef NOT_WIN
#include <string.h>
#include <errno.h>
#endif
}

/*!
 *Add a rule to check to the chain.
 */
void HttpHeaderChecker::addRule(HttpHeaderChecker::Rule& r)
{
  HttpHeaderChecker::Rule* r2 = new Rule(r); 
  if(r2)
    addRule(r2);
}

/*!
 *Add a rule to check to the chain.
 */
void HttpHeaderChecker::addRule(HttpHeaderChecker::Rule* r)
{
  rules.push_back(r);
}

/*!
 *Clear all the used memory.
 */
void HttpHeaderChecker::clear()
{
  list<HttpHeaderChecker::Rule*>::iterator i = rules.begin();
  for(;i != rules.end(); i++)
  {
    Rule *r = *i;
    r->name.clear();
    r->value.free();
    delete r;
  }
  rules.clear();
}

/*!
 *Constructor.
 */
HttpHeaderChecker::HttpHeaderChecker()
{
  defaultCmd=ALLOW;
}

/*!
 *Destroy the object.
 */
HttpHeaderChecker::~HttpHeaderChecker()
{
  clear();
}

/*!
 *Check if the header is allowed by the chain.
 */
int HttpHeaderChecker::isAllowed(HttpHeader* h)
{
  list<HttpHeaderChecker::Rule*>::iterator i = rules.begin();
  for(; i != rules.end(); i++)
  {
    string *val = h->getValue((*i)->name.c_str(), 0);
    if(val && (*i)->value.isCompiled())
    {
      regmatch_t pm;
      if(!(*i)->value.exec(val->c_str(), 1,&pm, REG_NOTEOL))
      {
        return (*i)->cmd;
      }
    }
  }
  return defaultCmd;
}

/*!
 *Return the default action used by the chain.
 *Zero means deny access, one means that access is allowed by default.
 */
HttpHeaderChecker::CMD HttpHeaderChecker::getDefaultCmd()
{
  return defaultCmd;
}

/*!
 *Set te default action to use.
 *Zero to deny access, one to allow access by default.
 */
void HttpHeaderChecker::setDefaultCmd(HttpHeaderChecker::CMD cmd)
{
  defaultCmd = cmd;
}

/*!
 *Construct by copy.
 */
HttpHeaderChecker::HttpHeaderChecker(HttpHeaderChecker& hhc)
{
  clone(hhc);
}

/*!
 *Create a clone of the object.
 */
void HttpHeaderChecker::clone(HttpHeaderChecker& h)
{
  list<HttpHeaderChecker::Rule*>::iterator i = h.rules.begin();
  clear();
  for(;i != h.rules.end(); i++)
  {
    HttpHeaderChecker::Rule* r = new HttpHeaderChecker::Rule(*(*i));
    addRule(r);
  }  

  defaultCmd = h.defaultCmd;
}
