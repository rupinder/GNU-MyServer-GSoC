/*
MyServer
Copyright (C) 2007, 2008 The Free Software Foundation Inc.
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
#include <php_embed.h>

#include <myserver.h>
#include <include/connection/connection.h>
#include <include/socket/socket.h>
#include <include/server/server.h>
#include <include/base/sync/semaphore.h>
#include <include/base/sync/mutex.h>
#include <include/base/file/file.h>
#include <include/base/file/files_utility.h>
#include <include/protocol/http/http.h>
#include <include/protocol/http/dyn_http_manager.h>

#ifdef WIN32
#define EXPORTABLE(x) x _declspec(dllexport);
#else
#define EXPORTABLE(x) extern "C" x
#endif

EXPORTABLE(char*) name(char* name, u_long len);


EXPORTABLE(int) load(void* server, void* parser);

EXPORTABLE(int) postLoad(void* server,void* parser)

EXPORTABLE(int) unload(void* p);

class PhpManager 
{
public:
	PhpManager();
	virtual ~PhpManager();
	virtual int send(HttpThreadContext*, ConnectionPtr s, const char *filenamePath,
                   const char* cgi, int selfExecuted, int onlyHeader = 0);

};

EXPORTABLE(int) sendManager(HttpThreadContext* td, ConnectionPtr s, const char *filenamePath,
													 const char* cgi, int onlyHeader);


