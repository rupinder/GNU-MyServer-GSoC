/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2005, 2007, 2008, 2009 Free Software Foundation, Inc.
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

#ifndef HTTP_DATA_HANDLER_H
#define HTTP_DATA_HANDLER_H

#include "stdafx.h"
#include <include/protocol/protocol.h>
#include "include/protocol/http/http_headers.h"
#include <include/filter/filters_chain.h>
#include <include/filter/memory_stream.h>


/*!
 *Base class to handle HTTP data.
 */
class HttpDataHandler
{
public:
  virtual int load ();
  virtual int unLoad ();

	virtual int send(HttpThreadContext*, const char* exec,
                   const char* cmdLine = 0, bool execute = false,
                   bool onlyHeader = false);

  HttpDataHandler();
  virtual ~HttpDataHandler();

	static void checkDataChunks(HttpThreadContext*, bool*, bool*);

  static int appendDataToHTTPChannel (HttpThreadContext* td,
                                      char* buffer,
                                      u_long size,
                                      File* appendFile,
                                      FiltersChain* chain,
                                      bool append,
                                      bool useChunks,
                                      u_long realBufferSize,
                                      MemoryStream *tmpStream);

	static int appendDataToHTTPChannel(HttpThreadContext* td, 
                                     char* buffer,
                                     u_long size,
                                     File* appendFile, 
                                     FiltersChain* chain,
                                     bool append, 
                                     bool useChunks);

};


#endif
