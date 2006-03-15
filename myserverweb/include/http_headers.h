/*
MyServer
Copyright (C) 2002, 2003, 2004 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef HTTP_HEADERS_H
#define HTTP_HEADERS_H

#include "../stdafx.h"
#include "../include/mime_manager.h"
#include "../include/http_request.h"
#include "../include/http_response.h"
#include "../include/stringutils.h"
#include "../include/file.h"
#include "../include/mem_buff.h"
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


#ifndef WIN32
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

// Bloodshed Dev-C++ Helper
#ifndef intptr_t
#define intptr_t int
#endif
extern class CBase64Utils base64Utils;

#define HTTP_AUTH_SCHEME_BASIC 0
#define HTTP_AUTH_SCHEME_DIGEST 1

/*!
 *Structure used by the HTTP protocol parser to describe a thread.
 */
struct HttpThreadContext
{
	int appendOutputs;/*! Used by SSI. */
  int lastError;/*! Used by SSI and set by raiseHTTPError. */
  int onlyHeader;/*! Is the client asking only for the header? */
	ConnectionPtr connection;
	MemBuf *buffer;
	MemBuf *buffer2;
	u_long buffersize;
	u_long buffersize2;
	u_long id;
	u_long nBytesToRead;
	u_long nHeaderChars;
	HttpResponseHeader response;
	HttpRequestHeader  request;
	string filenamePath;
	string pathInfo;
	string pathTranslated;
	string cgiRoot;
	string cgiFile;
	string scriptPath;
	string scriptDir;
	string scriptFile;
	string inputDataPath;
	string outputDataPath;
	HashMap<string,string*> other;
	char identity[32];
	File inputData;
	File outputData;
	int auth_scheme;
	void* lhttp;
  void* mime;
	const char * getHashedData (const char *name);
};


class HttpHeaders
{
public:
	static int buildHTTPRequestHeaderStruct(HttpRequestHeader *request, 
                                          HttpThreadContext *td,char *input=0);
	static int buildHTTPResponseHeaderStruct(HttpResponseHeader *response, 
                                           HttpThreadContext *td,char *input=0);
	static int validHTTPRequest(char*,HttpThreadContext*,u_long*,u_long*);
	static int validHTTPResponse(char*,HttpThreadContext*,u_long*,u_long*);
	static void resetHTTPRequest(HttpRequestHeader *request);
	static void resetHTTPResponse(HttpResponseHeader *response);
	static void buildDefaultHTTPResponseHeader(HttpResponseHeader*);
	static void buildHTTPResponseHeader(char *,HttpResponseHeader*);
};
#endif
