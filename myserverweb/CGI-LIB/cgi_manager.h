/*
*myServer
*Copyright (C) 2002 Giuseppe Scrivano
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
#pragma once
#define EXPORTABLE _declspec(dllexport)
#include "../include/http.h"
#include "../include/response_requestStructs.h"

class EXPORTABLE cgi_manager
{
private:
	httpThreadContext* td;
	cgi_data* cgidata;
public:
	int setPageError(int);
	int raiseError(int);
	cgi_manager(cgi_data* data);
	~cgi_manager(void);
	operator <<(char*);
	char* operator >>(char*);
	int Start(cgi_data* data);
	int Clean();
	void getenv(char*,char*,unsigned int*);
	char* GetParam(char*);
	char* PostParam(char*);
	int Write(char*);
}; 

