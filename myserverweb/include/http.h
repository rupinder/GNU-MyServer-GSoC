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
#include "..\include\Response_RequestStructs.h"


BOOL sendHTTPRESOURCE(LPCONNECTION s,char *filename,BOOL systemrequest=FALSE,BOOL OnlyHeader=FALSE,int firstByte=0,int lastByte=-1);
BOOL sendHTTPFILE(LPCONNECTION s,char *filenamePath,BOOL OnlyHeader=FALSE,int firstByte=0,int lastByte=-1);
BOOL sendHTTPDIRECTORY(LPCONNECTION s,char* folder);
void buildHttpResponseHeader(char *str,HTTP_RESPONSE_HEADER*);
void buildDefaultHttpResponseHeader(HTTP_RESPONSE_HEADER*);
BOOL controlHTTPConnection(LPCONNECTION a,char *b1,char *b2,int bs1,int bs2,DWORD nbtr,LOGGEDUSERID *imp);
BOOL sendMSCGI(LPCONNECTION s,char* exec,char* cmdLine=0);
BOOL sendCGI(LPCONNECTION s,char* filename,char* ext,char* exec);
void raiseHTTPError(LPCONNECTION a,int ID);
BOOL getMIME(char *MIME,char *filename,char *dest,char *dest2);
void getPath(char *filenamePath,char *filename,BOOL systemrequest);