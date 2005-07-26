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
 

/*!
 *Get the value of the [name] field.
 */
string* HttpRequestHeader::getValue(const char* name, string* out)
{
  if(!strcmpi(name,"cmd"))
  {
    if(out)
      out->assign(cmd.c_str());
    return &cmd;
  }  

  if(!strcmpi(name,"ver"))
  { 
    if(out)
      out->assign( ver.c_str()); 
    return &ver;
  }
 
  if(!strcmpi(name,"uri"))
  { 
    if(out)
      out->assign( uri.c_str()); 
    return &uri;
  } 
 
  if(!strcmpi(name,"uriOpts"))
  { 
    if(out)
      out->assign( uriOpts.c_str());
    return &uriOpts;
  } 

  if(!strcmpi(name,"accept"))
  { 
    if(out)
      out->assign( accept.c_str()); 
    return &accept;
  }
 
  if(!strcmpi(name,"Content-Encoding"))
  { 
    if(out)
      out->assign( contentEncoding.c_str()); 
    return &contentEncoding;
  }
 if(!strcmpi(name,"Transfer-Encoding"))
 { 
   if(out)
     out->assign( transferEncoding.c_str()); 
   return &transferEncoding;
 }
 if(!strcmpi(name,"Authorization"))
 { 
   if(out)
     out->assign( auth.c_str()); 
   return &auth;
 }
 
 if(!strcmpi(name,"Accept-Encoding"))
 { 
   if(out)
     out->assign( acceptEncoding.c_str()); 
   return &acceptEncoding;
 }
 if(!strcmpi(name,"Accept-Language"))
 { 
    if(out)
   out->assign( acceptLanguage.c_str()); 
   return &acceptLanguage;
 }
 if(!strcmpi(name,"Accept-Charset"))
 { 
   if(out)
     out->assign( acceptCharset.c_str()); 
   return &acceptCharset;
 }  
 if(!strcmpi(name,"If-Modified-Since"))
 { 
   if(out)
     out->assign( ifModifiedSince.c_str()); 
   return &ifModifiedSince;
 }  
 if(!strcmpi(name,"Connection"))
 {     
   if(out)
     out->assign( connection.c_str()); 
   return &connection;
 }  
 if(!strcmpi(name,"User-Agent"))
 { 
   if(out)
     out->assign( userAgent.c_str()); 
   return &userAgent;
 } 
 if(!strcmpi(name,"Cookie"))
 { 
   if(out)
     out->assign( cookie.c_str()); 
   return &cookie;
 }
 if(!strcmpi(name,"Content-Type"))
 { 
   if(out)
     out->assign( contentType.c_str()); 
   return &contentType;
 } 
 if(!strcmpi(name,"Content-Length"))
 { 
   if(out)
     out->assign( contentLength.c_str()); 
   return &contentLength;
 } 

 if(!strcmpi(name,"Date"))
 { 
   if(out)
     out->assign( date.c_str()); 
   return &date;
 } 
 if(!strcmpi(name,"Expires"))
 { 
   if(out)
     out->assign( dateExp.c_str()); 
   return &dateExp;
 } 
 if(!strcmpi(name,"Last-Modified"))
 { 
   if(out)
     out->assign( lastModified.c_str()); 
   return &lastModified;
 } 

 if(!strcmpi(name,"Cache-Control"))
 { 
   if(out)
     out->assign( cacheControl.c_str()); 
   return &cacheControl;
 } 

 if(!strcmpi(name,"Pragma"))
 { 
   if(out)
     out->assign( pragma.c_str()); 
   return &pragma;
 } 

 if(!strcmpi(name,"Referer"))
 { 
   if(out)
     out->assign( referer.c_str()); 
   return &referer;
 } 

 if(!strcmpi(name,"From"))
 { 
   if(out)
     out->assign( from.c_str()); 
   return &from;
 } 

 if(!strcmpi(name,"Host"))
 { 
   if(out)
     out->assign( host.c_str()); 
   return &host;
 } 

 if(!strcmpi(name,"rangeType"))
 { 
   if(out)
     out->assign( rangeType.c_str()); 
   return &rangeType;
 } 
 
 if(!out)
   return 0;

 if(!strcmpi(name,"rangeByteBegin"))
 {
   ostringstream s;
   s << rangeByteBegin;
   out->assign(s.str());
   return 0; 
 }

 if(!strcmpi(name,"rangeByteEnd"))
 {
   ostringstream s;
   s << rangeByteEnd;
   out->assign(s.str());
   return 0; 
 }

 {
   char *s_pos;
   char *ptr = strstr(other.c_str(), name);
   if(!ptr)
   {
     out->assign("");
     return 0;
   }
   ptr += strlen(name);
   while(*ptr && (*ptr == ':') && (*ptr == ' ')) 
     ptr++;
   s_pos = ptr;
   while(*ptr && (*ptr!='\n') )
     ptr++;

   out->assign(s_pos, ptr - s_pos);
   return 0;
 }

}
