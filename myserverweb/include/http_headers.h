/*
*MyServer
*Copyright (C) 2002 The MyServer Team
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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef HTTP_HEADERS_H
#define HTTP_HEADERS_H
#include "../stdafx.h"
#include "../include/Response_RequestStructs.h"
#include "../include/stringutils.h"
#include "../include/filemanager.h"
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

#ifndef WIN32
#include "../include/lfind.h"

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
struct httpThreadContext
{
	int appendOutputs;/*Used by SSI*/
	LPCONNECTION connection;
	char *buffer;
	char *buffer2;
	u_long buffersize;
	u_long buffersize2;
	u_long id;
	u_long nBytesToRead;
	u_long nHeaderChars;
	HTTP_RESPONSE_HEADER  response;
	HTTP_REQUEST_HEADER  request;
	char filenamePath[MAX_PATH];
	char pathInfo[MAX_PATH];
	char pathTranslated[MAX_PATH];
	char cgiRoot[MAX_PATH];
	char cgiFile[MAX_PATH];
	char scriptPath[MAX_PATH];
	char scriptDir[MAX_PATH];
	char scriptFile[MAX_PATH];
	char identity[32];
	char inputDataPath[MAX_PATH];
	char outputDataPath[MAX_PATH];
	MYSERVER_FILE inputData;
	MYSERVER_FILE outputData;
	int auth_scheme;
	void* lhttp;
};


class http_headers
{
public:
	static int buildHTTPRequestHeaderStruct(HTTP_REQUEST_HEADER *request,httpThreadContext *td,char *input=0);
	static int buildHTTPResponseHeaderStruct(HTTP_RESPONSE_HEADER *response,httpThreadContext *td,char *input=0);
	static u_long validHTTPRequest(char*,httpThreadContext*,u_long*,u_long*);
	static u_long validHTTPResponse(char*,httpThreadContext*,u_long*,u_long*);
	static void resetHTTPRequest(HTTP_REQUEST_HEADER *request);
	static void resetHTTPResponse(HTTP_RESPONSE_HEADER *response);
	static void buildDefaultHTTPResponseHeader(HTTP_RESPONSE_HEADER*);
	static void buildHTTPResponseHeader(char *,HTTP_RESPONSE_HEADER*);
};
#endif
