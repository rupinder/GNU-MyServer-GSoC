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
#include "StdAfx.h"
#include "cgi_manager.h"
#include "../include/Response_RequestStructs.h"
#pragma comment(lib,"wsock32.lib")
static char* buffer;
static char* buffer2;
static struct HTTP_RESPONSE_HEADER *res;
static struct HTTP_REQUEST_HEADER *req;

int initialize(void* p1,void* p2,void* p3,void* p4)
{
	buffer=(char*)p1;
	buffer2=(char*)p2;
	res=(HTTP_RESPONSE_HEADER*)p3;
	req=(HTTP_REQUEST_HEADER*)p4;
	return 1;
}
int cgi_manager::Write(char* str)
{
	lstrcat(buffer2,str);
	return 1;
}

int cgi_manager::Start()
{
	return 1;
}
int cgi_manager::Clean()
{
	return 1;
}

cgi_manager::cgi_manager(void)
{
	Start();
}
cgi_manager::~cgi_manager(void)
{
	Clean();
}
char* cgi_manager::GetParam(char* param)
{
	buffer[0]='\0';
	char *tmp=strdup(req->URIOPTS);
	char *token=strtok(tmp,"=");
	while( token != NULL )
	{
		if(!lstrcmpi(token,param))
		{
			token=strtok( NULL, "&" );
			if(token)
				lstrcpy(buffer,token);
			break;
		}
		else
		{
			token=strtok( NULL, "&" );
		}
		token = strtok( NULL, "=" );
	}
	delete[] tmp;
	return buffer;
}


char* cgi_manager::PostParam(char* param)
{
	if(req->URIOPTSPTR==NULL)
		return NULL;
	buffer[0]='\0';
	char *tmp=strdup(req->URIOPTSPTR);
	char *token=strtok(tmp,"=");
	while( token != NULL )
	{
		if(!lstrcmpi(token,param))
		{
			token=strtok( NULL, "&" );
			if(token)
				lstrcpy(buffer,token);
			break;
		}
		else
		{
			token=strtok( NULL, "&" );
		}
		token = strtok( NULL, "=" );
	}
	delete[] tmp;
	return buffer;
}
cgi_manager::operator <<(char* str)
{
	return Write(str);
}
char *cgi_manager::operator >>(char* str)
{
	return GetParam(str);
} 

