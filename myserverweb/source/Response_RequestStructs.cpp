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
#include "../include/stringutils.h"

#include <sstream>

using namespace std;

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

int HttpRequestHeader::getValue(const char* name, string& out)
{
  if(!strcmpi(name,"CMD"))
  {
    out.assign(CMD.c_str());
    return 1;
  }  

  if(!strcmpi(name,"VER"))
  { 
    out.assign( VER.c_str()); 
    return 1;
  }
 
  if(!strcmpi(name,"URI"))
  { 
    out.assign( URI.c_str()); 
    return 1;
  } 
 
  if(!strcmpi(name,"URIOPTS"))
  { 
    out.assign( URIOPTS.c_str());
    return 1;
  } 

  if(!strcmpi(name,"ACCEPT"))
  { 
    out.assign( ACCEPT.c_str()); 
    return 1;
  }
 
  if(!strcmpi(name,"Content-Encoding"))
  { 
    out.assign( CONTENT_ENCODING.c_str()); 
    return 1;
  }
 if(!strcmpi(name,"Transfer-Encoding"))
 { 
   out.assign( TRANSFER_ENCODING.c_str()); 
   return 1;
 }
 if(!strcmpi(name,"Authorization"))
 { 
   out.assign( AUTH.c_str()); 
   return 1;
 }
 
 if(!strcmpi(name,"Accept-Encoding"))
 { 
   out.assign( ACCEPTENC.c_str()); 
   return 1;
 }
 if(!strcmpi(name,"Accept-Language"))
 { 
   out.assign( ACCEPTLAN.c_str()); 
   return 1;
 }
 if(!strcmpi(name,"Accept-Charset"))
 { 
   out.assign( ACCEPTCHARSET.c_str()); 
   return 1;
 }  
 if(!strcmpi(name,"If-Modified-Since"))
 { 
   out.assign( IF_MODIFIED_SINCE.c_str()); 
   return 1;
 }  
 if(!strcmpi(name,"Connection"))
 { 
   out.assign( CONNECTION.c_str()); 
   return 1;
 }  
 if(!strcmpi(name,"User-Agent"))
 { 
   out.assign( USER_AGENT.c_str()); 
   return 1;
 } 
 if(!strcmpi(name,"Cookie"))
 { 
   out.assign( COOKIE.c_str()); 
   return 1;
 }
 if(!strcmpi(name,"Content-Type"))
 { 
   out.assign( CONTENT_TYPE.c_str()); 
   return 1;
 } 
 if(!strcmpi(name,"Content-Length"))
 { 
   out.assign( CONTENT_LENGTH.c_str()); 
   return 1;
 } 

 if(!strcmpi(name,"Date"))
 { 
   out.assign( DATE.c_str()); 
   return 1;
 } 
 if(!strcmpi(name,"Expires"))
 { 
   out.assign( DATEEXP.c_str()); 
   return 1;
 } 
 if(!strcmpi(name,"Last-Modified"))
 { 
   out.assign( LAST_MODIFIED.c_str()); 
   return 1;
 } 

 if(!strcmpi(name,"Cache-Control"))
 { 
   out.assign( CACHE_CONTROL.c_str()); 
   return 1;
 } 

 if(!strcmpi(name,"Pragma"))
 { 
   out.assign( PRAGMA.c_str()); 
   return 1;
 } 

 if(!strcmpi(name,"Referer"))
 { 
   out.assign( REFERER.c_str()); 
   return 1;
 } 

 if(!strcmpi(name,"From"))
 { 
   out.assign( FROM.c_str()); 
   return 1;
 } 

 if(!strcmpi(name,"Host"))
 { 
   out.assign( HOST.c_str()); 
   return 1;
 } 

 if(!strcmpi(name,"RANGETYPE"))
 { 
   out.assign( RANGETYPE.c_str()); 
   return 1;
 } 
 

 if(!strcmpi(name,"RANGEBYTEBEGIN"))
 {
   ostringstream s;
   s << RANGEBYTEBEGIN;
   out.assign(s.str());
   return 1; 
 }

 if(!strcmpi(name,"RANGEBYTEEND"))
 {
   ostringstream s;
   s << RANGEBYTEEND;
   out.assign(s.str());
   return 1; 
 }

 {
   char *s_pos;
   char *ptr = strstr(OTHER.c_str(), name);
   if(!ptr)
   {
     out.assign("");
     return 0;
   }
   ptr += strlen(name);
   while(*ptr && (*ptr == ':') && (*ptr == ' ')) 
     ptr++;
   s_pos = ptr;
   while(*ptr && (*ptr!='\n') )
     ptr++;

   out.assign(s_pos, ptr - s_pos);
   return 1;
 }

}
