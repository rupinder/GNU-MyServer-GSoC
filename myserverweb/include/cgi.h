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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

class cgi : public http_data_handler
{
  static int cgi_timeout;
public:
  static void setTimeout(int);
  static int getTimeout();
	int send(httpThreadContext*,ConnectionPtr s,char* scriptpath,char* exec,
              int execute, int only_header=0);
	static void buildCGIEnvironmentString(httpThreadContext*,char*,int=1);
};
#endif

