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

#include <windows.h>
#define SOCKETLIBINCLUDED
#include "..\stdafx.h"
#include "..\include\http.h"
#include "..\include\utility.h"
#include "..\include\cserver.h"
#include "..\include\HTTPmsg.h"
#include "..\include\connectionstruct.h"


typedef LPVOID HCONN;

#define HSE_VERSION_MAJOR 5
#define HSE_VERSION_MINOR 0
#define HSE_LOG_BUFFER_LEN 80
#define HSE_MAX_EXT_DLL_NAME_LEN 256

#define HSE_STATUS_SUCCESS 1
#define HSE_STATUS_SUCCESS_AND_KEEP_CONN 2
#define HSE_STATUS_PENDING 3
#define HSE_STATUS_ERROR 4

#define HSE_REQ_BASE 0
#define HSE_REQ_SEND_URL_REDIRECT_RESP (HSE_REQ_BASE + 1)
#define HSE_REQ_SEND_URL (HSE_REQ_BASE + 2)
#define HSE_REQ_SEND_RESPONSE_HEADER (HSE_REQ_BASE + 3)
#define HSE_REQ_DONE_WITH_SESSION (HSE_REQ_BASE + 4)
#define HSE_REQ_END_RESERVED 1000
#define HSE_REQ_MAP_URL_TO_PATH (HSE_REQ_END_RESERVED+1)
#define HSE_REQ_GET_SSPI_INFO (HSE_REQ_END_RESERVED+2)
#define HSE_REQ_TRANSMIT_FILE (HSE_REQ_END_RESERVED+6)


typedef struct _HSE_VERSION_INFO 
{
  DWORD dwExtensionVersion;
  CHAR lpszExtensionDesc[HSE_MAX_EXT_DLL_NAME_LEN];
} HSE_VERSION_INFO, *LPHSE_VERSION_INFO;

typedef struct _EXTENSION_CONTROL_BLOCK 
{
  DWORD cbSize;
  DWORD dwVersion;
  HCONN ConnID;
  DWORD dwHttpStatusCode;
  CHAR lpszLogData[HSE_LOG_BUFFER_LEN];
  LPSTR lpszMethod;
  LPSTR lpszQueryString;
  LPSTR lpszPathInfo;
  LPSTR lpszPathTranslated;
  DWORD cbTotalBytes;
  DWORD cbAvailable;
  LPBYTE lpbData;
  LPSTR lpszContentType;
  BOOL (WINAPI * GetServerVariable)(HCONN hConn, LPSTR lpszVariableName, 						                      							  					
				                            LPVOID lpvBuffer, LPDWORD lpdwSize);
  BOOL (WINAPI * WriteClient)(HCONN ConnID, LPVOID Buffer, LPDWORD lpdwBytes, DWORD dwReserved);
  BOOL (WINAPI * ReadClient)(HCONN ConnID, LPVOID lpvBuffer, LPDWORD lpdwSize);
  BOOL (WINAPI * ServerSupportFunction)(HCONN hConn, DWORD dwHSERRequest, LPVOID lpvBuffer,
                                        LPDWORD lpdwSize, LPDWORD lpdwDataType);
} EXTENSION_CONTROL_BLOCK, *LPEXTENSION_CONTROL_BLOCK;

struct ConnTableRecord
{
  BOOL Allocated;
  httpThreadContext *td;
  char* envString;
  LPCONNECTION connection;
  HANDLE ISAPIDoneEvent;
};
void initISAPI();
void cleanupISAPI();
int ISAPIRedirect(httpThreadContext* td,LPCONNECTION a,char *URL);
int ISAPISendURI(httpThreadContext* td,LPCONNECTION a,char *URL);
int ISAPISendHeader(httpThreadContext* td,LPCONNECTION a,char *URL);
ConnTableRecord *HConnRecord(HCONN hConn);

typedef BOOL (WINAPI * PFN_GETEXTENSIONVERSION)(HSE_VERSION_INFO *pVer);
typedef DWORD (WINAPI * PFN_HTTPEXTENSIONPROC)(EXTENSION_CONTROL_BLOCK *pECB);

int sendISAPI(httpThreadContext* td,LPCONNECTION connection,char* scriptpath,char* /*ext*/,char *cgipath);

BOOL WINAPI ServerSupportFunctionExport(HCONN hConn, DWORD dwHSERRequest,LPVOID lpvBuffer, LPDWORD lpdwSize, LPDWORD lpdwDataType);
BOOL WINAPI ReadClientExport(HCONN hConn, LPVOID lpvBuffer, LPDWORD lpdwSize ) ;
BOOL WINAPI WriteClientExport(HCONN hConn, LPVOID Buffer, LPDWORD lpdwBytes, DWORD dwReserved);
BOOL WINAPI GetServerVariableExport(HCONN, LPSTR, LPVOID, LPDWORD);