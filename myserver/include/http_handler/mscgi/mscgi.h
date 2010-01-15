/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008, 2009, 2010 Free
  Software Foundation, Inc.
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

#ifndef MSCGI_H
# define MSCGI_H
# include "myserver.h"
# include <include/protocol/http/http_request.h>
# include <include/protocol/http/http_response.h>
# include <include/connection/connection.h>
# include <include/conf/mime/mime_manager.h>
# include <include/base/file/file.h>
# include <include/server/server.h>
# include <include/protocol/http/http_headers.h>
# include <include/protocol/http/http_data_handler.h>
# include <include/base/dynamic_lib/dynamiclib.h>
struct HttpThreadContext;

class MsCgi;

struct MsCgiData
{
  char *envString;
  HttpThreadContext* td;
  int errorPage;
  bool headerSent;
  Server* server;
  MsCgi* mscgi;
  FiltersChain *filtersChain;
  bool keepAlive;
  bool useChunks;
  bool onlyHeader;
  bool error;

};

typedef int (*CGIMAIN)(const char*, MsCgiData*);

class MsCgi : public HttpDataHandler
{
public:
  virtual int load ();
  virtual int unLoad ();
  virtual int send (HttpThreadContext*, const char* exec,
                    const char* cmdLine = 0, bool execute = false,
                    bool onlyHeader = false);

  int write (const char*, u_long, MsCgiData*);
  int sendHeader (MsCgiData*);

private:
  static DynamicLibrary mscgiModule;
  MsCgiData data;
};
#endif
