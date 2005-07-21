/*
*MyServer
*Copyright (C) 2005 The MyServer Team
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

#include "../stdafx.h"
#include "../include/Response_RequestStructs.h"

/*!
 *Create the object.
 */
HttpRequestHeader::HttpRequestHeader()
{
  free();
}

/*!
 *Destroy the object.
 */
HttpRequestHeader::~HttpRequestHeader()
{

}

/*!
 *free the structure.
 */
void HttpRequestHeader::free()
{
  VER.clear();
	TRANSFER_ENCODING.clear();	
	CONTENT_ENCODING.clear();	
	CMD.clear();		
	ACCEPT.clear();
	AUTH.clear();
	ACCEPTENC.clear();	
	ACCEPTLAN.clear();	
	ACCEPTCHARSET.clear();
	CONNECTION.clear();
	USER_AGENT.clear();
	COOKIE.clear();
	CONTENT_TYPE.clear();
	CONTENT_LENGTH.clear();
	DATE.clear();
	FROM.clear();
	DATEEXP.clear();	
	MODIFIED_SINCE.clear();
	LAST_MODIFIED.clear();	
	URI.clear();
	URIOPTS.clear();
	URIOPTSPTR=NULL;
	REFERER.clear();
	HOST.clear();
	CACHE_CONTROL.clear();
	IF_MODIFIED_SINCE.clear();
	OTHER.clear();
	PRAGMA.clear();
	RANGETYPE.clear();
	RANGEBYTEBEGIN=0;
	RANGEBYTEEND=0;
	uriEndsWithSlash=0;
	digestRealm[0]='\0';
	digestOpaque[0]='\0';
	digestNonce[0]='\0';
	digestCnonce[0]='\0';
	digestUri[0]='\0';
	digestMethod[0]='\0';
	digestUsername[0]='\0';
	digestResponse[0]='\0';
	digestQop[0]='\0';
	digestNc[0]='\0';
}


/*!
 *Create the object.
 */
HttpResponseHeader::HttpResponseHeader()
{
  free();
}


/*!
 *Destroy the object.
 */
HttpResponseHeader::~HttpResponseHeader()
{

}

/*!
 *Reset the object.
 */
void HttpResponseHeader::free()
{
	VER.clear();	
	SERVER_NAME.clear();
	CONTENT_TYPE.clear();
	CONNECTION.clear();
	MIMEVER.clear();
	P3P.clear();
	COOKIE.clear();
	CONTENT_LENGTH.clear();
	ERROR_TYPE.clear();
	CONTENT_ENCODING.clear();
	TRANSFER_ENCODING.clear();
	LOCATION.clear();
	DATE.clear();		
	AUTH.clear();
	DATEEXP.clear();	
	OTHER.clear();
	LAST_MODIFIED.clear();
	CACHE_CONTROL.clear();
	CONTENT_RANGE.clear();
}
