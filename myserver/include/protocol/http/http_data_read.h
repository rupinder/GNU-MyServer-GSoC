/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
  Free Software Foundation, Inc.
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
# define HTTP_DATA_READ_H
# include "myserver.h"
# include <include/protocol/http/http_thread_context.h>
# include <include/protocol/protocol.h>
# include <include/protocol/http/http_headers.h>
# include <include/conf/security/security_cache.h>
# include <include/base/xml/xml_parser.h>
# include <include/base/thread/thread.h>
# include <include/base/sync/mutex.h>
# include <include/protocol/http/dyn_http_command_manager.h>
# include <include/protocol/http/dyn_http_command.h>
# include <include/protocol/http/dyn_http_manager_list.h>
# include <include/protocol/http/dyn_http_manager.h>
# include <include/base/multicast/multicast.h>
# include <include/protocol/http/http_data_handler.h>
# include <string>
# include <sstream>
# include <vector>
using namespace std;


class HttpDataRead
{
public:
  static int readPostData (HttpThreadContext *td, int *ret);

  static int readContiguousPrimitivePostData (const char *inBuffer,
                                              size_t *inBufferPos,
                                              size_t inBufferSize,
                                              Socket *inSocket,
                                              char *outBuffer,
                                              size_t outBufferSize,
                                              size_t *nbr,
                                              u_long timeout);

  static int readChunkedPostData (const char *inBuffer,
                                  size_t *inBufferPos,
                                  size_t inBufferSize,
                                  Socket *inSocket,
                                  char *outBuffer,
                                  size_t outBufferSize,
                                  size_t *nbr,
                                  u_long timeout,
                                  Stream *out,
                                  long maxChunks,
                                  size_t *remainingChunk = NULL);

};

#endif
