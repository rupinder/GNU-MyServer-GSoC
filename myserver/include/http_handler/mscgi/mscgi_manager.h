/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008 Free Software Foundation, Inc.
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#ifndef CGI_MANAGER_H
# define CGI_MANAGER_H

# ifdef WIN32
#  define EXPORTABLE _declspec (dllexport)
# endif

# include "stdafx.h"

# include <include/server/server.h>
# include <include/protocol/http/http.h>
# include <include/http_handler/mscgi/mscgi.h>
# include <include/base/file/file.h>
# include <include/protocol/http/http_request.h>
# include <include/protocol/http/http_response.h>
# include <include/base/string/stringutils.h>
# define LOCAL_BUFFER_DIM 150

# ifdef WIN32
class EXPORTABLE MscgiManager
# else
class MscgiManager
# endif
{
public:
	Server *getServer ();
	MsCgiData* getCgiData ();
	void setContentType (const char *);
	void addHeader (const char*,  const char *);
	int setPageError (int);
	int raiseError (int);
	MscgiManager (MsCgiData* data);
	~MscgiManager (void);
	int operator <<(const char*);
	char* operator >>(const char*);
	int start (MsCgiData* data);
	int clean ();
	void getenv (const char*, char*, u_long*);
	char* getParam (const char*);
	char* postParam (const char*);
	int write (const char*);
	int write (const void*, int);
private:
	HttpThreadContext* td;
	MsCgiData* cgidata;
	char localbuffer[LOCAL_BUFFER_DIM];
};

#endif
