/*
MyServer
Copyright (C) 2005 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef HTTP_HEADER_CHECKER_H
#define HTTP_HEADER_CHECKER_H

#include "../stdafx.h"
#include "../include/http_headers.h"
#include "../include/myserver_regex.h"
#include "../include/http_request.h"
#include "../include/http_response.h"

#include <string>
#include <sstream>
#include <list>
using namespace std;

class HttpHeaderChecker
{
public:
  enum CMD{DENY=0,  ALLOW=1};  
  struct Rule
  {
    string name;
    Regex value;
    CMD cmd;
    Rule()
    {name.assign(""); cmd=ALLOW;}

    Rule(Rule& r)
    {name.assign(r.name); value.clone(r.value); cmd=r.cmd;}
  };
  HttpHeaderChecker();
  HttpHeaderChecker(HttpHeaderChecker&);
  ~HttpHeaderChecker();  
  void addRule(HttpHeaderChecker::Rule*);
  void addRule(HttpHeaderChecker::Rule&);
  void clear();
  int isAllowed(HttpHeader*);
  CMD getDefaultCmd();
  void setDefaultCmd(CMD);
  void clone(HttpHeaderChecker&);
protected:
  list<HttpHeaderChecker::Rule*> rules;
  CMD defaultCmd;
};

#endif
