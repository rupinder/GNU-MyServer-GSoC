/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2008, 2009, 2010 Free Software
  Foundation, Inc.
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

#ifndef HTTP_RESPONSE_H
# define HTTP_RESPONSE_H

# include "myserver.h"

# include <string>
# include <include/base/hash_map/hash_map.h>
# include <include/protocol/http/http_header.h>

using namespace std;

/*! Max length for a HTTP response fields. */
# define HTTP_RESPONSE_VER_DIM 10
# define HTTP_RESPONSE_OTHER_DIM 4096


/*!
 * Structure to describe an HTTP response
 */
struct HttpResponseHeader : public HttpHeader
{
  const static int INFORMATIONAL = 100;
  const static int SUCCESSFUL = 200;
  const static int REDIRECTION = 300;
  const static int CLIENT_ERROR = 400;
  const static int SERVER_ERROR = 500;

  int httpStatus;
  string ver;
  string contentLength;
  string errorType;

  HttpResponseHeader ();
  ~HttpResponseHeader ();

  int getStatusType ();

  virtual string* getValue (const char* name, string* out);
  virtual string* setValue (const char* name, const char* in);
  void free ();
};

#endif
