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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef HTTP_DATA_HANDLER_H
#define HTTP_DATA_HANDLER_H
#include "../stdafx.h"
#include "../include/protocol.h"
#include "../include/http_headers.h"

/*!
 *Base class to handle HTTP data.
 */
class HttpDataHandler
{
private:

public:
  static int load();
  static int unload();
	virtual int send(httpThreadContext*, ConnectionPtr s,char *filenamePath,
                   char* cgi, int OnlyHeader=0);
  HttpDataHandler();
  virtual ~HttpDataHandler();
};


#endif
