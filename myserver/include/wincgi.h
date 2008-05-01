/*
MyServer
Copyright (C) 2002, 2003, 2004, 2008 The MyServer Team
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

#ifndef WINCGI_H
#define WINCGI_H

#include "../include/http_request.h"
#include "../include/http_response.h"
#include "../include/mime_manager.h"
#include "../include/security.h"
#include "../include/http_headers.h"
#include "../include/http_data_handler.h"

extern const char *versionOfSoftware;

class WinCgi : public HttpDataHandler
{
private:
  static u_long timeout;
public:
  WinCgi();
  ~WinCgi();
  static void setTimeout(u_long);
  static u_long getTimeout();
  virtual int send(HttpThreadContext*, ConnectionPtr s,
                   const char* exec, const char* cmdLine = 0,
                   int execute = 0, int onlyHeader = 0);
};
#endif
