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

#ifndef MSCGI_H
#define MSCGI_H
#include "../stdafx.h"
#include "../include/Response_RequestStructs.h"
#include "../include/connectionstruct.h"
#include "../include/MIME_manager.h"
#include "../include/cgi.h"
#include "../include/filemanager.h"
#include "../include/http_headers.h"
#include "../include/http_data_handler.h"
struct httpThreadContext;

struct MsCgiData
{
	char *envString;
	httpThreadContext* td;
	int errorPage;
	MYSERVER_FILE stdOut;

};
typedef int (*CGIMAIN)(char*, MsCgiData*); 

class MsCgi : public HttpDataHandler
{
public:
	/*!
	*Functions to Load and Free the MSCGI library.
	*/
	static int load();
	static int unload();
	/*!
	*Use this to send a MSCGI file through the HTTP protocol.
	*/
	int send(httpThreadContext*, ConnectionPtr s, char* exec,
                char* cmdLine=0, int execute=0, int only_header=0);
	typedef int (*CGIMAIN)(char*, MsCgiData*); 
};
#endif
