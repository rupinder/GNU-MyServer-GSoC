/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
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

#ifndef CGI_H
#define CGI_H

#include "../include/Response_RequestStructs.h"
#include "../include/MIME_manager.h"
#include "../include/mscgi.h"
#include "../include/security.h"
#include "../include/http_headers.h"
#include "../include/http_data_handler.h"

extern const char *versionOfSoftware;

class Cgi : public HttpDataHandler
{
  static int cgiTimeout;
public:
  static void setTimeout(int);
  static int getTimeout();
	int send(HttpThreadContext*, ConnectionPtr s, const char* scriptpath, 
           const char* exec, int execute, int onlyHeader=0);
	static void buildCGIEnvironmentString(HttpThreadContext*, char*, int=1);
};
#endif

