/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2007, 2008, 2009 Free Software Foundation, Inc.
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

#ifndef FASTCGI_H
# define FASTCGI_H

# include "stdafx.h"
# include <include/protocol/http/http_headers.h>
# include <include/base/utility.h>
# include <include/base/socket/socket.h>
# include <include/conf/vhost/vhost.h>
# include <include/protocol/http/http_errors.h>
# include <include/connection/connection.h>
# include <include/base/string/stringutils.h>
# include <include/base/thread/thread.h>
# include <include/base/sync/mutex.h>
# include <include/protocol/http/http_data_handler.h>
# include <include/base/hash_map/hash_map.h>
# include <include/base/process/process_server_manager.h>
# include <string>

using namespace std;

typedef struct
{
  unsigned char version;
  unsigned char type;
  unsigned char requestIdB1;
  unsigned char requestIdB0;
  unsigned char contentLengthB1;
  unsigned char contentLengthB0;
  unsigned char paddingLength;
  unsigned char reserved;
} FcgiHeader;

/*!
 *Value for version component of FcgiHeader.
 */
# define FCGIVERSION_1           1

/*!
 *Current version of the FastCGI protocol.
 */
# define FCGIVERSION FCGIVERSION_1

/*!
 *Values for type component of FcgiHeader
 */
# define FCGIBEGIN_REQUEST       1
# define FCGIABORT_REQUEST       2
# define FCGIEND_REQUEST         3
# define FCGIPARAMS              4
# define FCGISTDIN               5
# define FCGISTDOUT              6
# define FCGISTDERR              7
# define FCGIDATA                8
# define FCGIGET_VALUES          9
# define FCGIGET_VALUES_RESULT  10
# define FCGIUNKNOWN_TYPE       11
# define FCGIMAXTYPE (FCGIUNKNOWN_TYPE)

typedef struct
{
  unsigned char roleB1;
  unsigned char roleB0;
  unsigned char flags;
  unsigned char reserved[5];
} FcgiBeginRequestBody;

typedef struct
{
  FcgiHeader header;
  FcgiBeginRequestBody body;
} FcgiBeginRequestRecord;

/*!
 *Mask for flags component of FcgiBeginRequestBody.
 */
# define FCGIKEEP_CONN  1

/*!
 *Values for role component of FcgiBeginRequestBody.
 */
# define FCGIRESPONDER  1
# define FCGIAUTHORIZER 2
# define FCGIFILTER     3

typedef struct
{
  unsigned char appStatusB3;
  unsigned char appStatusB2;
  unsigned char appStatusB1;
  unsigned char appStatusB0;
  unsigned char protocolStatus;
  unsigned char reserved[3];
} FcgiEndRequestBody;

typedef struct
{
  FcgiHeader header;
  FcgiEndRequestBody body;
} FcgiEndRequestRecord;

/*!
 *Values for protocolStatus component of FcgiEndRequestBody.
 */
# define FCGIREQUEST_COMPLETE 0
# define FCGICANT_MPX_CONN    1
# define FCGIOVERLOADED       2
# define FCGIUNKNOWN_ROLE     3

typedef struct
{
  unsigned char type;
  unsigned char reserved[7];
} FcgiUnknownTypeBody;

typedef struct
{
  FcgiHeader header;
  FcgiUnknownTypeBody body;
} FcgiUnknownTypeRecord;


typedef ProcessServerManager::Server FastCgiServer;

struct FcgiContext
{
  HttpThreadContext* td;
  FastCgiServer* server;
  Socket sock;

  bool useChunks;
  bool keepalive;
  bool headerSent;
};

class FiltersChain;

class FastCgi : public HttpDataHandler
{
public:
  static int getTimeout ();
  static void setTimeout (int);
  FastCgi ();
  virtual int load ();
  virtual int send (HttpThreadContext* td, const char* scriptpath,
                    const char *cgipath, bool execute = false,
                    bool onlyHeader = false);

  virtual int unLoad ();
private:
  static ProcessServerManager *processServerManager;
  static int timeout;
  static int initialized;

  int handleHeader (FcgiContext* con, FiltersChain* chain,
                    bool *responseCompleted, bool onlyHeader);
  int sendData (FcgiContext* con, u_long dim,
                u_long timeout, FiltersChain* chain,
                bool *responseCompleted, bool onlyHeader);
  int fastCgiRequest (FcgiContext* con, int id);
  int readHeader (FcgiContext *con, FcgiHeader* header,
                  u_long started, u_long timeout, int id);


  void generateFcgiHeader ( FcgiHeader&, int ,int, int );
  Socket getFcgiConnection ();
  int buildFASTCGIEnvironmentString (HttpThreadContext*,char*,char*);
  int sendFcgiBody (FcgiContext* con, char* buffer, int len, int type, int id);
  FastCgiServer* isFcgiServerRunning (const char*);
  FastCgiServer* runFcgiServer (FcgiContext*, const char*);
  FastCgiServer* connect (FcgiContext*, const char*);
};
#endif
