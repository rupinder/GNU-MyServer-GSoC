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

/*
*Do not use this method in a CGI script.
*It is used for server internal operations.
*/
int EXPORTABLE initialize(httpThreadContext*,LPCONNECTION,cgi_data*);
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
	void getEnvVariable(char*,char*,unsigned int*);
	char* GetParam(char*);
	char* PostParam(char*);
	int Write(char*);
	MYSERVER_FILE_HANDLE getInputDataFile();
}; 

