/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2005, 2008, 2009 Free Software Foundation, Inc.
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
# define HTTP_FILE_H
# include "stdafx.h"

# include <include/protocol/protocol.h>
# include <include/protocol/http/http_headers.h>
# include <include/protocol/http/http_data_handler.h>

class HttpFile  : public HttpDataHandler
{
public:
  virtual int load ();
  virtual int unLoad ();
  virtual int send (HttpThreadContext* td,
                   const char *filenamePath, const char* cgi,
                   bool execute = false, bool OnlyHeader = false);
  HttpFile ();
  virtual ~HttpFile ();

protected:
  int putFile (HttpThreadContext* td,
               string& filename);
  int deleteFile (HttpThreadContext* td,
                  string& filename);
private:
  static void generateEtag (string & etag, u_long mtime, u_long size);
};


#endif
