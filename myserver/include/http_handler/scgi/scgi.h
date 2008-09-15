/* -*- mode: cpp-mode */
/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007, 2008 Free Software Foundation, Inc.
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

#ifndef SCGI_H
#define SCGI_H

#include "stdafx.h"
#include <include/protocol/http/http_headers.h>
#include <include/base/utility.h>
#include <include/base/socket/socket.h>
#include <include/conf/vhost/vhost.h>
#include <include/protocol/http/http_errors.h>
#include <include/connection/connection.h>
#include <include/base/string/stringutils.h>
#include <include/base/thread/thread.h>
#include <include/base/sync/mutex.h>
#include <include/protocol/http/http_data_handler.h>
#include <include/base/hash_map/hash_map.h>
#include <include/base/process/process_server_manager.h>
#include <include/filter/filters_chain.h>
#include <string>

using namespace std;


typedef ProcessServerManager::Server ScgiServer;


struct ScgiContext
{
  HttpThreadContext* td;
  ScgiServer* server;
  Socket sock;
  File tempOut;
};

class Scgi : public HttpDataHandler
{
public:
  static int getTimeout();
  static void setTimeout(int);
  Scgi();
  static int load(XmlParser*);
  int send(HttpThreadContext* td, ConnectionPtr connection,
           const char* scriptpath, const char *cgipath = 0,
           int execute = 0, int onlyHeader = 0);
  static int unLoad();
private:
  static ProcessServerManager *processServerManager;
  static int timeout;
  static int initialized;
  Socket getScgiConnection();
  int sendPostData(ScgiContext* ctx);
  int sendResponse(ScgiContext* ctx, int onlyHeader, FiltersChain*);
  int buildScgiEnvironmentString(HttpThreadContext*, char*, char*);
  int sendNetString(ScgiContext*, const char*, int);
  ScgiServer* isScgiServerRunning(const char*);
  ScgiServer* runScgiServer(ScgiContext*, const char*);
  ScgiServer* connect(ScgiContext*, const char*);
};
#endif
