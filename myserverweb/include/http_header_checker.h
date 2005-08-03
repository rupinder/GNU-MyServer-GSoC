/*
*MyServer
*Copyright (C) 2005 The MyServer Team
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
#include "../include/protocol.h"
#include "../include/http_headers.h"
#include "../include/myserver_regex.h"
#include "../include/security_cache.h"
#include "../include/cXMLParser.h"
#include "../include/threads.h"
#include "../include/http_file.h"
#include "../include/http_dir.h"
#include "../include/dyn_http_command.h"

#include <string>
#include <sstream>
#include <list>
using namespace std;

class HttpHeaderChecker
{
public:
  enum CMD{DENY_IF=0,  ACCEPT_IF=1};  
  struct Rule
  {
    string name;
    Regex value;
    CMD cmd;
  };
  HttpHeaderChecker();
  ~HttpHeaderChecker();  
  void addRule(HttpHeaderChecker::Rule*);
  void clear();
  int isAllowed(HttpHeader*);
  CMD getDefaultCmd();
  void setDefaultCmd(CMD);
protected:
  list<HttpHeaderChecker::Rule*> rules;
  CMD defaultCmd;
  HttpHeader * obj;
};

#endif
