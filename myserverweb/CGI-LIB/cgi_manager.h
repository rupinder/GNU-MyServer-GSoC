/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef CGI_MANAGER_H
#define CGI_MANAGER_H

#ifdef WIN32
#define EXPORTABLE _declspec(dllexport)
#endif

#include "../stdafx.h"
/*
*Do not link the library using with the SSL support.
*/
#ifndef DO_NOT_USE_SSL
#define DO_NOT_USE_SSL
#endif

#include "../include/http.h"
#include "../include/filemanager.h"
#include "../include/Response_RequestStructs.h"
#include "../include/stringutils.h"
#define LOCAL_BUFFER_DIM 150

#ifdef WIN32
class EXPORTABLE cgi_manager
#else
class CgiManager
#endif
{
private:
	HttpThreadContext* td;
	MsCgiData* cgidata;
	char localbuffer[LOCAL_BUFFER_DIM];
public:
	MsCgiData* getCgiData();
	void setContentType(char *);
	int  setPageError(int);
	int raiseError(int);
	CgiManager(MsCgiData* data);
	~CgiManager(void);
	int  operator <<(char*);
	char*  operator >>(char*);
	int   Start(MsCgiData* data);
	int Clean();
	void   getenv(char*,char*,unsigned int*);
	char*  GetParam(char*);
	char*  PostParam(char*);
	int Write(char*);
	int Write(void*, int);
};

#endif
