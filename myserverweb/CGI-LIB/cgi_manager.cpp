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

/*
*Write to the stdout.
*/
int cgi_manager::Write(char* str)
{
	strcat(td->buffer2,str);
	return 1;
}
/*
*Start the execution of the CGI.
*/
int cgi_manager::Start(cgi_data* data)
{
	cgidata=data;
	td=data->td;
	td->buffer2[0]='\0';
	return 1;
}
/*
*Clean the memory allocated by the CGI.
*/
int cgi_manager::Clean()
{
	return 1;
}
int cgi_manager::setPageError(int ID)
{
	td->response.httpStatus=ID;
	return 1;
}
int cgi_manager::raiseError(int ID)
{
	cgidata->errorPage=ID;
	return 1;
}
cgi_manager::cgi_manager(cgi_data* data)
{
	Start(data);
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
	if(td->request.URIOPTS[0]=='\0')
		return NULL;
	static char lb[150];
	lb[0]='\0';
	char *c=&(td->request.URIOPTS)[0];
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
	if(td->request.URIOPTSPTR==NULL)
		return NULL;
	static char lb[150];
	lb[0]='\0';
	char *c=td->request.URIOPTSPTR;
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
	*If it is a POST request return a param from the POST values
	*else return a GET param.
	*/
	if(td->request.URIOPTS)
		return PostParam(str);
	else
		return GetParam(str);
} 
/*
*Get the value of an environment variable.
*/
void cgi_manager::getenv(char* lpszVariableName,char *lpvBuffer,unsigned int* lpdwSize)
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

