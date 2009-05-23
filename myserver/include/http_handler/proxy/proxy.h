/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2009 Free Software Foundation, Inc.
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
#define PROXY_H

#include <include/protocol/http/http_response.h>
#include <include/protocol/http/http_request.h>
#include <include/protocol/http/http_headers.h>
#include <include/protocol/http/http_data_handler.h>

class FiltersChain;

class Proxy : public HttpDataHandler
{
public:
  static void setTimeout (int);
  static int getTimeout ();

	virtual int send (HttpThreadContext*, ConnectionPtr s,
                   const char* scriptpath, const char* exec = 0,
                   int execute = 0, int onlyHeader = 0);
protected:
  int flushToClient (HttpThreadContext* td, Socket& client,
                     FiltersChain &out, int onlyHeader);
  static int timeout;
};
#endif
