/*
*myServer
*Copyright (C) Rocky_10_Balboa
*This library is free software; you can redistribute it and/or
*modify it under the terms of the GNU Library General Public
*License as published by the Free Software Foundation; either
*version 2 of the License, or (at your option) any later version.

*This library is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*Library General Public License for more details.

*You should have received a copy of the GNU Library General Public
*License along with this library; if not, write to the
*Free Software Foundation, Inc., 59 Temple Place - Suite 330,
*Boston, MA  02111-1307, USA.
*/
#pragma once
#define EXPORTABLE _declspec(dllexport)
#define NO_INCLUDE_SOCKETLIB
#include "../include/http.h"
/*
*Do not use this method in a CGI script.
*It is used for server internal operations.
*/
int EXPORTABLE initialize(httpThreadContext*,LPCONNECTION);
class EXPORTABLE cgi_manager
{
private:

public:
	cgi_manager(void);
	~cgi_manager(void);
	operator <<(char*);
	char* operator >>(char*);
	int Start();
	int Clean();
	char* GetParam(char*);
	char* PostParam(char*);
	int Write(char*);
}; 

