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
#include "StdAfx.h"
#include "cgi_manager.h"
#include "../include/Response_RequestStructs.h"
#include "../include/http.h"
#include "../include/mscgi.h"
#pragma comment(lib,"wsock32.lib")
static char* buffer;
static char* buffer2;
static struct HTTP_RESPONSE_HEADER *res;
static struct HTTP_REQUEST_HEADER *req;
static struct httpThreadContext* td;
static struct cgi_data* cgidata;
/*
*Initialize the globals variables.
*/
int initialize(httpThreadContext* td,LPCONNECTION s,cgi_data* data)
{
	buffer=td->buffer;
	buffer2=td->buffer2;
	::td=td;
	cgidata=data;
	td->buffer2[0]='\0';
	res=(HTTP_RESPONSE_HEADER*)&(td->response);
	req=(HTTP_REQUEST_HEADER*)&(td->request);
	return 1;
}
/*
*Write to the stdout.
*/
int cgi_manager::Write(char* str)
{
	strcat(buffer2,str);
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
	if(req->URIOPTS[0]=='\0')
		return NULL;
	static char lb[150];
	lb[0]='\0';
	char *c=&req->URIOPTS[0];
	for(;;)
	{
		while(strncmp(c,param,lstrlen(param)))c++;
		c+=lstrlen(param);
		if(*c=='=')
		{
			c++;
			break;
		}
	}
	while((*c) && (*c!='&'))
	{
		lb[lstrlen(lb)+1]='\0';
		lb[lstrlen(lb)]=*c;
		c++;
	}
	return &lb[0];
}

/*
*Returns the value of a param passed through a POST request.
*/
char* cgi_manager::PostParam(char* param)
{
	if(req->URIOPTSPTR==NULL)
		return NULL;
	static char lb[150];
	lb[0]='\0';
	char *c=req->URIOPTSPTR;
	for(;;)
	{
		while(strncmp(c,param,lstrlen(param)))c++;
		c+=lstrlen(param);
		if(*c=='=')
		{
			c++;
			break;
		}
	}
	while((*c) && (*c!='&'))
	{
		lb[lstrlen(lb)+1]='\0';
		lb[lstrlen(lb)]=*c;
		c++;
	}
	return &lb[0];

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
/*
*Get the name of an environment variable.
*/
void cgi_manager::getEnvVariable(char* lpszVariableName,char *lpvBuffer,unsigned int* lpdwSize)
{
	((char*)lpvBuffer)[0]='\0';
	char *localEnv=cgidata->envString;
	size_t variableNameLen=strlen(lpszVariableName);
	for(u_long i=0;;i+=(u_long)strlen(&localEnv[i])+1)
	{
		if(((localEnv[i+variableNameLen])== '=')&&(!strncmp(&localEnv[i],lpszVariableName,variableNameLen)))
		{
			strncpy((char*)lpvBuffer,&localEnv[i+variableNameLen+1],*lpdwSize);
			break;
		}
		else if((localEnv[i]=='\0') && (localEnv[i+1]=='\0'))
		{
			break;
		}
	}
	*lpdwSize=lstrlen((char*)lpvBuffer);
}
/*
*Get the input data file.
*/
MYSERVER_FILE_HANDLE cgi_manager::getInputDataFile()
{
	return td->inputData;
}