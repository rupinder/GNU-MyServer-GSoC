/*
MyServer
Copyright (C) 2005 The MyServer Team
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
#include "../include/http_response.h"
#include "../include/stringutils.h"
#include <iostream>
#include <sstream>
using namespace std;

/*!
 *Get the value of the [name] field.
 */
string* HttpResponseHeader::getValue(const char* name, string* out)
{
  if(!strcmpi(name,"Ver"))
  {
    if(out)
      out->assign(ver.c_str());
    return &ver;
  }  

  if(!strcmpi(name,"Server"))
  { 
    if(out)
      out->assign( serverName.c_str()); 
    return &ver;
  }

  if(!strcmpi(name,"Content-Type"))
  { 
    if(out)
      out->assign( contentType.c_str()); 
    return &contentType;
  }

  if(!strcmpi(name,"Connection"))
  { 
    if(out)
      out->assign( connection.c_str()); 
    return &connection;
  }

  if(!strcmpi(name,"Content-Type"))
  { 
    if(out)
      out->assign( contentType.c_str()); 
    return &contentType;
  }

  if(!strcmpi(name,"MIME-Version"))
  { 
    if(out)
      out->assign( mimeVer.c_str()); 
    return &mimeVer;
  }

  if(!strcmpi(name,"Cookie"))
  { 
    if(out)
      out->assign( cookie.c_str()); 
    return &cookie;
  }

  if(!strcmpi(name,"Content-Length"))
  { 
    if(out)
      out->assign( contentLength.c_str()); 
    return &contentLength;
  }

  if(!strcmpi(name,"Last-Modified"))
  { 
    if(out)
      out->assign( lastModified.c_str()); 
    return &lastModified;
  }

  if(!strcmpi(name,"Location"))
  { 
    if(out)
      out->assign( location.c_str()); 
    return &location;
  }

  if(!strcmpi(name,"Date"))
  { 
    if(out)
      out->assign(date.c_str()); 
    return &date;
  }

  if(!strcmpi(name,"Date-Expires"))
  { 
    if(out)
      out->assign( location.c_str()); 
    return &location;
  }

  if(!strcmpi(name,"WWW-Authenticate"))
  { 
    if(out)
      out->assign(auth.c_str()); 
    return &auth;
  }

	{
		HttpResponseHeader::Entry *e = other.get(name);
		if(e)
		{
			if(out)
				out->assign(*(e->value));
			return e->value;
		}
		return 0;
	}
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
	cookie.clear();
	contentLength.clear();
	errorType.clear();
	location.clear();
	date.clear();		
	auth.clear();
	dateExp.clear();
	{
		HashMap<string, HttpResponseHeader::Entry*>::Iterator it = other.begin();
		HashMap<string, HttpResponseHeader::Entry*>::Iterator end = other.end();
		for(;it!=end;it++){
			delete (*it);
		}
	}
	other.clear();
	lastModified.clear();
}
