/*
MyServer
Copyright (C) 2002, 2003, 2004, 2008 Free Software Foundation, Inc.
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

#ifndef HTTP_HEADERS_H
#define HTTP_HEADERS_H

#include "../stdafx.h"
#include "../include/connection.h"

extern "C" {
#ifdef WIN32
#include <direct.h>
#include <errno.h>
#endif
#ifdef NOT_WIN
#include <string.h>
#include <errno.h>
#endif
}


#include <string>
using namespace std;


#define HTTP_AUTH_SCHEME_BASIC 0
#define HTTP_AUTH_SCHEME_DIGEST 1

struct HttpThreadContext;
struct HttpRequestHeader;
struct HttpResponseHeader;

class HttpHeaders
{
public:
	static int buildHTTPRequestHeaderStruct(const char* input,
                                          u_long inputSize,
                                          u_long* nHeaderChars,
                                          HttpRequestHeader *request, 
                                          Connection* connection);

	static int buildHTTPResponseHeaderStruct(const char *input, 
                                           HttpResponseHeader *response, 
                                           u_long* nbtr);

	static int validHTTPRequest(const char*, u_long, u_long*, u_long*);
	static int validHTTPResponse(const char*, u_long*, u_long*);

	static void resetHTTPRequest(HttpRequestHeader *request);
	static void resetHTTPResponse(HttpResponseHeader *response);

	static void buildDefaultHTTPResponseHeader(HttpResponseHeader*);
	static void buildDefaultHTTPRequestHeader(HttpRequestHeader*);

	static void buildHTTPResponseHeader(char *, HttpResponseHeader*);
	static void buildHTTPRequestHeader(char *, HttpRequestHeader*);
};
#endif
