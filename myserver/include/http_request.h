/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007 The MyServer Team
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

#include <string>
#include "../include/hash_map.h"
#include "../include/http_header.h"

#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

using namespace std;


/*! Max length for a HTTP request fields. */
#define HTTP_REQUEST_CMD_DIM 16
#define HTTP_REQUEST_VER_DIM 10
#define HTTP_REQUEST_AUTH_DIM 16
#define HTTP_REQUEST_CONTENT_LENGTH_DIM 12
#define HTTP_REQUEST_URI_DIM 4096
#define HTTP_REQUEST_URI_OPTS_DIM 4096
#define HTTP_REQUEST_OTHER_DIM 4096
#define HTTP_REQUEST_RANGE_TYPE_DIM 16


/*!
 *Structure to describe an HTTP request.
 */
struct HttpRequestHeader : public HttpHeader
{
  struct Entry
  {
    string *name;
    string *value;
		Entry()
		{
			name = new string();
			value = new string();
		}
		
		Entry(string& n, string& v)
		{
			name = new string();
			value = new string();

			name->assign(n);
			value->assign(v);
		}

		~Entry()
		{
			delete name;
			delete value;
			
		}

  };
	string cmd;		
	string ver;		
	string auth;
	string contentLength;
	string uri;
	string uriOpts;		
	char *uriOptsPtr;		
	string rangeType;	
	u_long rangeByteBegin;
	u_long rangeByteEnd;
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


	HashMap<string, HttpRequestHeader::Entry*> other;
  virtual string* getValue(const char* name, string* out);
  virtual string* setValue(const char* name, const char* in);

  HttpRequestHeader();
  ~HttpRequestHeader();
  void free();
 
};

#endif
