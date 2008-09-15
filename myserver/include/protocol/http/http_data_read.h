/* -*- mode: cpp-mode */
/*
MyServer
Copyright (C) 2002-2008 Free Software Foundation, Inc.
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

#ifndef HTTP_DATA_READ_H
#define HTTP_DATA_READ_H
#include "stdafx.h"
#include <include/protocol/http/http_thread_context.h>
#include <include/protocol/protocol.h>
#include <include/protocol/http/http_headers.h>
#include <include/conf/security/security_cache.h>
#include <include/base/xml/xml_parser.h>
#include <include/base/thread/thread.h>
#include <include/base/sync/mutex.h>
#include <include/plugin/http_command/dyn_http_command_manager.h>
#include <include/plugin/http_command/dyn_http_command.h>
#include <include/plugin/http_manager/dyn_http_manager_list.h>
#include <include/plugin/http_manager/dyn_http_manager.h>
#include <include/base/multicast/multicast.h>
#include <include/protocol/http/http_data_handler.h>
#include <string>
#include <sstream>
#include <vector>
using namespace std;


class HttpDataRead
{
public:
  static int readPostData(HttpThreadContext* td, int* ret);

  static int readContiguousPrimitivePostData(char* inBuffer,
                                             u_long *inBufferPos,
                                             u_long inBufferSize,
                                             Socket *inSocket,
                                             char* outBuffer,
                                             u_long outBufferSize,
                                             u_long* nbr,
                                             u_long timeout);
  static int readChunkedPostData(char* inBuffer,
                                 u_long *inBufferPos,
                                 u_long inBufferSize,
                                 Socket *inSocket,
                                 char* outBuffer,
                                 u_long outBufferSize,
                                 u_long* nbr,
                                 u_long timeout,
                                 File* out);
  
};

#endif
