/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2009, 2010 Free Software Foundation, Inc.
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

#ifndef PROXY_H
# define PROXY_H

# include "stdafx.h"

# include <include/protocol/http/http_response.h>
# include <include/protocol/http/http_request.h>
# include <include/protocol/http/http_headers.h>
# include <include/protocol/http/http_data_handler.h>

class FiltersChain;
class Socket;

class Proxy : public HttpDataHandler
{
public:
  static void setTimeout (int);
  static int getTimeout ();

  virtual int send (HttpThreadContext*, const char* scriptpath,
                    const char* exec = 0, bool execute = false,
                    bool onlyHeader = false);
protected:
  int flushToClient (HttpThreadContext* td, Socket& client,
                     FiltersChain &out, int onlyHeader);
  int readPayLoad (HttpThreadContext* td,
                   HttpResponseHeader* res,
                   FiltersChain *out,
                   Socket *client,
                   const char *initBuffer,
                   u_long initBufferSize,
                   int timeout,
                   bool useChunks = false,
                   bool keepalive = false,
                   string *serverTransferEncoding = NULL);

  static int timeout;
};
#endif
