/*
MyServer
Copyright (C) 2005, 2008 The MyServer Team
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

#ifndef HTTP_FILE_H
#define HTTP_FILE_H
#include "../stdafx.h"
#include "../include/protocol.h"
#include "../include/http_headers.h"
#include "../include/http_data_handler.h"
#include "../include/memory_stream.h"

class HttpFile  : public HttpDataHandler
{
public:
  static int load(XmlParser*);
  static int unLoad();
  virtual int send(HttpThreadContext*, ConnectionPtr s, 
                   const char *filenamePath, const char* cgi,
                   int execute = 0, int OnlyHeader = 0); 
  HttpFile();
  virtual ~HttpFile();
private:
  static int appendDataToHTTPChannel(HttpThreadContext* td, 
                                     char* buffer, 
                                     u_long size,
                                     File* appendFile, 
                                     FiltersChain* chain,
                                     bool append, 
                                     bool useChunks,
                                     u_long realBufferSize,
                                     MemoryStream *tmpStream);
};


#endif
