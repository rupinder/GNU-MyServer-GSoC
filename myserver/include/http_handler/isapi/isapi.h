/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2008, 2009, 2010 Free Software
  Foundation, Inc.
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

#ifndef ISAPI_H
# define ISAPI_H
# include "stdafx.h"
# include <include/protocol/http/http_headers.h>
# include <include/base/utility.h>
# include <include/protocol/http/http_errors.h>
# include <include/connection/connection.h>
# include <include/base/thread/thread.h>
# include <include/base/sync/mutex.h>
# include <include/protocol/http/http_data_handler.h>
# include <include/filter/filters_chain.h>
# ifdef WIN32

typedef LPVOID HCONN;

#  define HSE_VERSION_MAJOR 5
#  define HSE_VERSION_MINOR 1
#  define HSE_LOG_BUFFER_LEN 80
#  define HSE_MAX_EXT_DLL_NAME_LEN 256

#  define HSE_STATUS_SUCCESS 1
#  define HSE_STATUS_SUCCESS_AND_KEEP_CONN 2
#  define HSE_STATUS_PENDING 3
#  define HSE_STATUS_ERROR 4

#  define HSE_REQ_BASE 0
#  define HSE_REQ_SEND_URL_REDIRECT_RESP (HSE_REQ_BASE + 1)
#  define HSE_REQ_SEND_URL (HSE_REQ_BASE + 2)
#  define HSE_REQ_SEND_RESPONSE_HEADER (HSE_REQ_BASE + 3)
#  define HSE_REQ_DONE_WITH_SESSION (HSE_REQ_BASE + 4)
#  define HSE_REQ_END_RESERVED 1000
#  define HSE_REQ_MAP_URL_TO_PATH (HSE_REQ_END_RESERVED+1)
#  define HSE_REQ_GET_SSPI_INFO (HSE_REQ_END_RESERVED+2)
#  define HSE_REQ_TRANSMIT_FILE (HSE_REQ_END_RESERVED+6)
#  define HSE_REQ_MAP_URL_TO_PATH_EX (HSE_REQ_END_RESERVED+12)
#  define HSE_REQ_ASYNC_READ_CLIENT (HSE_REQ_END_RESERVED+10)
#  define HSE_REQ_IS_KEEP_CONN (HSE_REQ_END_RESERVED+8)

#  define HSE_URL_FLAGS_READ        0x00000001
#  define HSE_URL_FLAGS_WRITE        0x00000002
#  define HSE_URL_FLAGS_EXECUTE      0x00000004
#  define HSE_URL_FLAGS_SSL        0x00000008
#  define HSE_URL_FLAGS_DONT_CACHE    0x00000010
#  define HSE_URL_FLAGS_NEGO_CERT      0x00000020
#  define HSE_URL_FLAGS_REQUIRE_CERT    0x00000040
#  define HSE_URL_FLAGS_MAP_CERT      0x00000080
#  define HSE_URL_FLAGS_SSL128      0x00000100
#  define HSE_URL_FLAGS_SCRIPT      0x00000200



typedef struct _HSE_VERSION_INFO
{
  DWORD dwExtensionVersion;
  CHAR lpszExtensionDesc[HSE_MAX_EXT_DLL_NAME_LEN];
} HSE_VERSION_INFO, *LPHSE_VERSION_INFO;

typedef struct _HSE_URL_MAPEX_INFO
{
  CHAR   *lpszPath;
  DWORD  dwFlags;
  DWORD  cchMatchingPath;
  DWORD  cchMatchingURL;
  DWORD  dwReserved1;
  DWORD  dwReserved2;
} HSE_URL_MAPEX_INFO, * LPHSE_URL_MAPEX_INFO;

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
  BOOL (WINAPI * WriteClient)(HCONN ConnID, LPVOID Buffer, LPDWORD lpdwBytes,
                              DWORD dwReserved);
  BOOL (WINAPI * ReadClient)(HCONN ConnID, LPVOID lpvBuffer, LPDWORD lpdwSize);
  BOOL (WINAPI * ServerSupportFunction)(HCONN hConn, DWORD dwHSERRequest,
                                        LPVOID lpvBuffer, LPDWORD lpdwSize,
                                        LPDWORD lpdwDataType);
} EXTENSION_CONTROL_BLOCK, *LPEXTENSION_CONTROL_BLOCK;

struct ConnTableRecord
{
  FiltersChain chain;
  BOOL Allocated;
  int onlyHeader;
  int headerSent;
  int headerSize;
  HttpThreadContext *td;
  int dataSent;
  char* envString;
  ConnectionPtr connection;
  HANDLE ISAPIDoneEvent;
  void *isapi;
};
typedef BOOL (WINAPI * PFN_GETEXTENSIONVERSION)(HSE_VERSION_INFO *pVer);
typedef DWORD (WINAPI * PFN_HTTPEXTENSIONPROC)(EXTENSION_CONTROL_BLOCK *pECB);

# endif

class Isapi  : public HttpDataHandler
{
public:
# ifdef WIN32
  static ConnTableRecord *HConnRecord (HCONN hConn);
  int Redirect (HttpThreadContext* td,ConnectionPtr a,char *URL);
  int Senduri (HttpThreadContext* td,ConnectionPtr a,char *URL);
  int SendHeader (HttpThreadContext* td,ConnectionPtr a,char *URL);
  static BOOL buildAllHttpHeaders (HttpThreadContext* td,ConnectionPtr a,
                                   LPVOID output, LPDWORD maxLen);
  static BOOL buildAllRawHeaders (HttpThreadContext* td,ConnectionPtr a,
                                  LPVOID output, LPDWORD maxLen);
# endif
  Isapi ();
  static Mutex *isapiMutex;
  virtual int load ();
  virtual int unLoad ();
  virtual int send (HttpThreadContext* td, const char* scriptpath,
                    const char *cgipath = 0, bool execute = false,
                    bool onlyHeader = false);
private:
# ifdef WIN32
  static int initialized;
  static ConnTableRecord *connTable;
  static  u_long maxConnections;
# endif
};

# ifdef WIN32
BOOL WINAPI ISAPI_ServerSupportFunctionExport (HCONN hConn, DWORD dwHSERRequest,
                                               LPVOID lpvBuffer, LPDWORD lpdwSize,
                                               LPDWORD lpdwDataType);
BOOL WINAPI ISAPI_ReadClientExport (HCONN hConn, LPVOID lpvBuffer, LPDWORD lpdwSize ) ;
BOOL WINAPI ISAPI_WriteClientExport (HCONN hConn, LPVOID Buffer, LPDWORD lpdwBytes,
                                     DWORD dwReserved);
BOOL WINAPI ISAPI_GetServerVariableExport (HCONN, LPSTR, LPVOID, LPDWORD);
# endif

#endif
