/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
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

#include <string>
using namespace std;

#ifndef RESPONSE_REQUESTSTRUCTS_H
#define RESPONSE_REQUESTSTRUCTS_H
#define HTTP_RESPONSE_VER_DIM 10
#define HTTP_RESPONSE_SERVER_NAME_DIM 64
#define HTTP_RESPONSE_CONTENT_TYPE_DIM 48
#define HTTP_RESPONSE_CONTENT_RANGE_DIM 32
#define HTTP_RESPONSE_CONNECTION_DIM 32
#define HTTP_RESPONSE_MIMEVER_DIM 8
#define HTTP_RESPONSE_P3P_DIM 256
#define HTTP_RESPONSE_COOKIE_DIM 8192
#define HTTP_RESPONSE_CONTENT_LENGTH_DIM 8
#define HTTP_RESPONSE_ERROR_TYPE_DIM 32
#define HTTP_RESPONSE_LOCATION_DIM MAX_PATH
#define HTTP_RESPONSE_DATE_DIM 32
#define HTTP_RESPONSE_CONTENT_ENCODING_DIM 32
#define HTTP_RESPONSE_TRANSFER_ENCODING_DIM 32
#define HTTP_RESPONSE_DATEEXP_DIM 32
#define HTTP_RESPONSE_CACHE_CONTROL_DIM 64
#define HTTP_RESPONSE_AUTH_DIM 256
#define HTTP_RESPONSE_OTHER_DIM 512
#define HTTP_RESPONSE_LAST_MODIFIED_DIM 32

/*!
*Structure to describe an HTTP response
*/
struct HttpResponseHeader
{
	int httpStatus;
	string VER;	
	string SERVER_NAME;
	string CONTENT_TYPE;
	string CONNECTION;
	string MIMEVER;
  string P3P;
	string COOKIE;
	string CONTENT_LENGTH;
	string ERROR_TYPE;
	string LAST_MODIFIED;
	string LOCATION;
	string DATE;		
	string DATEEXP;	
	string AUTH;
	string OTHER;	
	string CONTENT_ENCODING;
	string TRANSFER_ENCODING;
	string CACHE_CONTROL;
	string CONTENT_RANGE;

  HttpResponseHeader();
  ~HttpResponseHeader();
  void free();
};

#define HTTP_REQUEST_CMD_DIM 16
#define HTTP_REQUEST_VER_DIM 10
#define HTTP_REQUEST_ACCEPT_DIM 4096
#define HTTP_REQUEST_AUTH_DIM 16
#define HTTP_REQUEST_ACCEPTENC_DIM 256
#define HTTP_REQUEST_ACCEPTLAN_DIM 512
#define HTTP_REQUEST_ACCEPTCHARSET_DIM 512
#define HTTP_REQUEST_CONNECTION_DIM 32
#define HTTP_REQUEST_USER_AGENT_DIM 128
#define HTTP_REQUEST_COOKIE_DIM 2048
#define HTTP_REQUEST_CONTENT_TYPE_DIM 96
#define HTTP_REQUEST_CONTENT_LENGTH_DIM 12
#define HTTP_REQUEST_CONTENT_ENCODING_DIM 16
#define HTTP_REQUEST_TRANSFER_ENCODING_DIM 16
#define HTTP_REQUEST_DATE_DIM 32
#define HTTP_REQUEST_DATEEXP_DIM 32
#define HTTP_REQUEST_MODIFIED_SINCE_DIM 32
#define HTTP_REQUEST_LAST_MODIFIED_DIM 32
#define HTTP_REQUEST_URI_DIM 1024
#define HTTP_REQUEST_PRAGMA_DIM 256
#define HTTP_REQUEST_IF_MODIFIED_SINCE_DIM 35
#define HTTP_REQUEST_URIOPTS_DIM 1024
#define HTTP_REQUEST_REFERER_DIM MAX_PATH
#define HTTP_REQUEST_FROM_DIM MAX_PATH
#define HTTP_REQUEST_HOST_DIM 128
#define HTTP_REQUEST_OTHER_DIM 256
#define HTTP_REQUEST_CACHE_CONTROL_DIM 64
#define HTTP_REQUEST_RANGETYPE_DIM 16

/*!
*Structure to describe an HTTP request.
*/
struct HttpRequestHeader
{
	string CMD;		
	string VER;		
	string ACCEPT;
	string CONTENT_ENCODING;
	string TRANSFER_ENCODING;
	string AUTH;
	string ACCEPTENC;	
	string ACCEPTLAN;	
	string ACCEPTCHARSET;
	string IF_MODIFIED_SINCE;
	string CONNECTION;
	string USER_AGENT;
	string COOKIE;
	string CONTENT_TYPE;
	string CONTENT_LENGTH;
	string DATE;
	string DATEEXP;	
	string MODIFIED_SINCE;
	string LAST_MODIFIED;	
	string URI;
	string CACHE_CONTROL;
	string PRAGMA;
	string URIOPTS;		
	char *URIOPTSPTR;		
	string REFERER;	
	string FROM;
	string HOST;			
	string OTHER;
	string RANGETYPE;	
	u_long  RANGEBYTEBEGIN;
	u_long  RANGEBYTEEND;
	int uriEndsWithSlash;
	
	/*! Digest authorization scheme stuff.  */
	char digestRealm[48+1];
	char digestOpaque[48+1];
	char digestNonce[48+1];
	char digestCnonce[48+1];
	char digestUri[1024+1];
	char digestMethod[16+1];		
	char digestUsername[48+1];
	char digestResponse[48+1];
	char digestQop[16+1];
	char digestNc[10+1];

  HttpRequestHeader();
  ~HttpRequestHeader();
  void free();
 
}; 
#endif
