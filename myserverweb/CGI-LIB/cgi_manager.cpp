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
#include "../include/stringutils.h"
#pragma comment(lib,"wsock32.lib")
static char* buffer;
static char* buffer2;
static struct HTTP_RESPONSE_HEADER *res;
static struct HTTP_REQUEST_HEADER *req;

/*
*Initialize the globals variables.
*/
int initialize(void* p1,void* p2,void* p3,void* p4)
{
	buffer=(char*)p1;
	buffer2=(char*)p2;
	res=(HTTP_RESPONSE_HEADER*)p3;
	req=(HTTP_REQUEST_HEADER*)p4;
	return 1;
}
/*
*Write to the stdout.
*/
int cgi_manager::Write(char* str)
{
	lstrcat(buffer2,str);
	return 1;
}
/*
*Start the execution of the CGI.
*/
int cgi_manager::Start()
{

	return 1;
}
/*
*Clean the memory allocated by the CGI.
*/
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
/*
*Returns the value of a param passed through the URL.
*/
char* cgi_manager::GetParam(char* param)
{
	if(req->URIOPTS==NULL)
		return NULL;
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

/*
*Returns the value of a param passed through a POST request.
*/
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
/*
*Write to stdout.
*/
cgi_manager::operator <<(char* str)
{
	return Write(str);
}
/*
*Read from the stdin.
*/
char *cgi_manager::operator >>(char* str)
{
	/*
	*If is a POST request return a param from the POST values
	*else return a GET param.
	*/
	if(req->URIOPTS)
		return PostParam(str);
	else
		return GetParam(str);
} 

