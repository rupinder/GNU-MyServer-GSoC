/*
*myServer
*Copyright (C) Rocky_10_Balboa
*This library is free software; you can redistribute it and/or
*modify it under the terms of the GNU Library General Public
*License as published by the Free Software Foundation; either
*version 2 of the License, or (at your option) any later version.

*This library is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*Library General Public License for more details.

*You should have received a copy of the GNU Library General Public
*License along with this library; if not, write to the
*Free Software Foundation, Inc., 59 Temple Place - Suite 330,
*Boston, MA  02111-1307, USA.
*/
#pragma once
#include "..\stdafx.h"
#include "..\include\cgi.h"
#include "..\include\connectionstruct.h"
#include "..\include\security.h"
#include "..\include\Response_RequestStructs.h"
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
	MYSERVER_FILE_HANDLE inputData;
	LOGGEDUSERID hImpersonation;
	LPCONNECTION connection;
};
/*
*The main function is controlHTTPConnection(...), that parse the request builds a response.
*/
int controlHTTPConnection(LPCONNECTION a,char *b1,char *b2,int bs1,int bs2,u_long nbtr,LOGGEDUSERID *imp,u_long id);
int sendHTTPRESOURCE(httpThreadContext*,LPCONNECTION s,char *filename,int systemrequest=FALSE,int OnlyHeader=FALSE,int firstByte=0,int lastByte=-1,int yetmapped=0);
int sendHTTPFILE(httpThreadContext*,LPCONNECTION s,char *filenamePath,int OnlyHeader=FALSE,int firstByte=0,int lastByte=-1);
int sendHTTPDIRECTORY(httpThreadContext*,LPCONNECTION s,char* folder);
void buildHTTPResponseHeader(char *str,HTTP_RESPONSE_HEADER*);
void buildDefaultHTTPResponseHeader(HTTP_RESPONSE_HEADER*);
int raiseHTTPError(httpThreadContext*,LPCONNECTION a,int ID);
void getPath(char *filenamePath,const char *filename,int systemrequest);
int getMIME(char *MIME,char *filename,char *dest,char *dest2);
u_long validHTTPRequest(httpThreadContext*,u_long*,u_long*);
void resetHTTPRequest(HTTP_REQUEST_HEADER *request);
int sendHTTPRedirect(httpThreadContext* td,LPCONNECTION a,char *newURL);
int sendHTTPNonModified(httpThreadContext* td,LPCONNECTION a);