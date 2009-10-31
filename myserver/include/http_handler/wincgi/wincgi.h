/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2002, 2003, 2004, 2008, 2009 Free Software Foundation, Inc.
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
# define WINCGI_H

# include <include/protocol/http/http_request.h>
# include <include/protocol/http/http_response.h>
# include <include/conf/mime/mime_manager.h>
# include <include/protocol/http/http_headers.h>
# include <include/protocol/http/http_data_handler.h>

class WinCgi : public HttpDataHandler
{
public:
  WinCgi ();
  ~WinCgi ();

  virtual int send (HttpThreadContext* td, const char* exec,
                   const char* cmdLine = 0, bool execute = false,
                   bool onlyHeader = false);
};
#endif
