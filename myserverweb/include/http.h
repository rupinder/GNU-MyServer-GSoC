/*
*myServer
*Copyright (C) 2002 Giuseppe Scrivano
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

#ifndef HTTP_H
#define HTTP_H
#include "../stdafx.h"
#include "../include/cgi.h"
#include "../include/connectionstruct.h"
#include "../include/security.h"
#include "../include/Response_RequestStructs.h"
extern const char *versionOfSoftware;
extern class CBase64Utils base64Utils;
/*
*Structure used by the HTTP protocol to describe a thread.
*/
struct httpThreadContext
{
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
	MYSERVER_FILE_HANDLE inputData;
	MYSERVER_FILE_HANDLE outputData;
	LOGGEDUSERID hImpersonation;
	LPCONNECTION connection;
};
/*
*The main function is controlHTTPConnection(...), that parses the request builds a response.
*/
int controlHTTPConnection(LPCONNECTION a,char *b1,char *b2,int bs1,int bs2,u_long nbtr,LOGGEDUSERID *imp,u_long id);
int sendHTTPRESOURCE(httpThreadContext*,LPCONNECTION s,char *filename,int systemrequest=false,int OnlyHeader=false,int firstByte=0,int lastByte=-1,int yetmapped=0);
int sendHTTPFILE(httpThreadContext*,LPCONNECTION s,char *filenamePath,int OnlyHeader=false,int firstByte=0,int lastByte=-1);
int sendHTTPDIRECTORY(httpThreadContext*,LPCONNECTION s,char* folder);
void buildHTTPResponseHeader(char *str,HTTP_RESPONSE_HEADER*);
void buildDefaultHTTPResponseHeader(HTTP_RESPONSE_HEADER*);
int raiseHTTPError(httpThreadContext*,LPCONNECTION a,int ID);
int hardHTTPError500(httpThreadContext* td,LPCONNECTION a);
void getPath(httpThreadContext* td,char *filenamePath,const char *filename,int systemrequest);
int getMIME(char *MIME,char *filename,char *dest,char *dest2);
u_long validHTTPRequest(httpThreadContext*,u_long*,u_long*);
u_long validHTTPResponse(httpThreadContext*,u_long*,u_long*);
void resetHTTPRequest(HTTP_REQUEST_HEADER *request);
void resetHTTPResponse(HTTP_RESPONSE_HEADER *response);
int sendHTTPRedirect(httpThreadContext* td,LPCONNECTION a,char *newURL);
int sendHTTPNonModified(httpThreadContext* td,LPCONNECTION a);
int buildHTTPRequestHeaderStruct(HTTP_REQUEST_HEADER *request,httpThreadContext *td,char *input=0);
int buildHTTPResponseHeaderStruct(HTTP_RESPONSE_HEADER *response,httpThreadContext *td,char *input=0);

#endif
