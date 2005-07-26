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
  ver.clear();
	transferEncoding.clear();	
	contentEncoding.clear();	
	cmd.clear();		
	accept.clear();
	auth.clear();
	acceptEncoding.clear();	
	acceptLanguage.clear();	
	acceptCharset.clear();
	connection.clear();
	userAgent.clear();
	cookie.clear();
	contentType.clear();
	contentLength.clear();
	date.clear();
	from.clear();
	dateExp.clear();	
	lastModified.clear();	
	uri.clear();
	uriOpts.clear();
	uriOptsPtr=NULL;
	referer.clear();
	host.clear();
	cacheControl.clear();
	ifModifiedSince.clear();
	other.clear();
	pragma.clear();
	rangeType.clear();
	rangeByteBegin=0;
	rangeByteEnd=0;
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
	ver.clear();	
	serverName.clear();
	contentType.clear();
	connection.clear();
	mimeVer.clear();
	p3p.clear();
	cookie.clear();
	contentLength.clear();
	errorType.clear();
	contentEncoding.clear();
	transferEncoding.clear();
	location.clear();
	date.clear();		
	auth.clear();
	dateExp.clear();	
	other.clear();
	lastModified.clear();
	cacheControl.clear();
	contentRange.clear();
}

int HttpRequestHeader::getValue(const char* name, string& out)
{
  if(!strcmpi(name,"cmd"))
  {
    out.assign(cmd.c_str());
    return 1;
  }  

  if(!strcmpi(name,"ver"))
  { 
    out.assign( ver.c_str()); 
    return 1;
  }
 
  if(!strcmpi(name,"uri"))
  { 
    out.assign( uri.c_str()); 
    return 1;
  } 
 
  if(!strcmpi(name,"uriOpts"))
  { 
    out.assign( uriOpts.c_str());
    return 1;
  } 

  if(!strcmpi(name,"accept"))
  { 
    out.assign( accept.c_str()); 
    return 1;
  }
 
  if(!strcmpi(name,"Content-Encoding"))
  { 
    out.assign( contentEncoding.c_str()); 
    return 1;
  }
 if(!strcmpi(name,"Transfer-Encoding"))
 { 
   out.assign( transferEncoding.c_str()); 
   return 1;
 }
 if(!strcmpi(name,"Authorization"))
 { 
   out.assign( auth.c_str()); 
   return 1;
 }
 
 if(!strcmpi(name,"Accept-Encoding"))
 { 
   out.assign( acceptEncoding.c_str()); 
   return 1;
 }
 if(!strcmpi(name,"Accept-Language"))
 { 
   out.assign( acceptLanguage.c_str()); 
   return 1;
 }
 if(!strcmpi(name,"Accept-Charset"))
 { 
   out.assign( acceptCharset.c_str()); 
   return 1;
 }  
 if(!strcmpi(name,"If-Modified-Since"))
 { 
   out.assign( ifModifiedSince.c_str()); 
   return 1;
 }  
 if(!strcmpi(name,"Connection"))
 { 
   out.assign( connection.c_str()); 
   return 1;
 }  
 if(!strcmpi(name,"User-Agent"))
 { 
   out.assign( userAgent.c_str()); 
   return 1;
 } 
 if(!strcmpi(name,"Cookie"))
 { 
   out.assign( cookie.c_str()); 
   return 1;
 }
 if(!strcmpi(name,"Content-Type"))
 { 
   out.assign( contentType.c_str()); 
   return 1;
 } 
 if(!strcmpi(name,"Content-Length"))
 { 
   out.assign( contentLength.c_str()); 
   return 1;
 } 

 if(!strcmpi(name,"Date"))
 { 
   out.assign( date.c_str()); 
   return 1;
 } 
 if(!strcmpi(name,"Expires"))
 { 
   out.assign( dateExp.c_str()); 
   return 1;
 } 
 if(!strcmpi(name,"Last-Modified"))
 { 
   out.assign( lastModified.c_str()); 
   return 1;
 } 

 if(!strcmpi(name,"Cache-Control"))
 { 
   out.assign( cacheControl.c_str()); 
   return 1;
 } 

 if(!strcmpi(name,"Pragma"))
 { 
   out.assign( pragma.c_str()); 
   return 1;
 } 

 if(!strcmpi(name,"Referer"))
 { 
   out.assign( referer.c_str()); 
   return 1;
 } 

 if(!strcmpi(name,"From"))
 { 
   out.assign( from.c_str()); 
   return 1;
 } 

 if(!strcmpi(name,"Host"))
 { 
   out.assign( host.c_str()); 
   return 1;
 } 

 if(!strcmpi(name,"rangeType"))
 { 
   out.assign( rangeType.c_str()); 
   return 1;
 } 
 

 if(!strcmpi(name,"rangeByteBegin"))
 {
   ostringstream s;
   s << rangeByteBegin;
   out.assign(s.str());
   return 1; 
 }

 if(!strcmpi(name,"rangeByteEnd"))
 {
   ostringstream s;
   s << rangeByteEnd;
   out.assign(s.str());
   return 1; 
 }

 {
   char *s_pos;
   char *ptr = strstr(other.c_str(), name);
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
