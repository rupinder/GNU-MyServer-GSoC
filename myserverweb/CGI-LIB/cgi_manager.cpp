/*
*MyServer
*Copyright (C) 2002 The MyServer Team
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

#include "../source/filemanager.cpp"
#define LOCAL_BUFFER_DIM 150
/*
*Write to the stdout.
*/
int cgi_manager::Write(char* str)
{
	if(str)
	{
		u_long nbw;
		cgidata->stdOut.writeToFile(str,(u_long)strlen(str),&nbw);
		return 1;
	}
	return 0;
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
	static char lb[LOCAL_BUFFER_DIM];
	lb[0]='\0';
	char *c=&(td->request.URIOPTS)[0];
	for(;;)
	{
		while((*c) && strncmp(c,param,min(strlen(param),strlen(c))))c++;
		if(*c=='\0')
			return &lb[0];
		c+=strlen(param);
		if(*c=='=')
		{
			c++;
			break;
		}
	}
	u_long len=0;
	while((*c) && (*c!='&'))
	{
		if(LOCAL_BUFFER_DIM<++len)
		{
			lb[strlen(lb)+1]='\0';
			lb[strlen(lb)]=*c;
		}
		c++;
		
	}
	return &lb[0];
}
/*
*Returns the value of a param passed through a POST request.
*/
char* cgi_manager::PostParam(char* param)
{
	static char lb[LOCAL_BUFFER_DIM];
	char buffer[LOCAL_BUFFER_DIM+50];
	u_long nbr=0;
	char c[2];
	c[1]='\0';
	buffer[0]='\0';
	lb[0]='\0';
	
	u_long toRead=td->inputData.getFileSize();
	do
	{
		cgidata->td->inputData.readFromFile(&c[0],1,&nbr);
		if(nbr==0)
			break;
		if((c[0]=='&') |(toRead==1))
		{
			if(!strncmp(param,buffer,strlen(param)))
			{
				lstrcpyn(lb,&buffer[strlen(param)],LOCAL_BUFFER_DIM);
				break;
			}
			buffer[0]='\0';
		}
		else
		{
			strncat(buffer,c,LOCAL_BUFFER_DIM+lstrlen(buffer));
		}
	}while(--toRead);

	return &lb[0];

}
/*
*Write to stdout.
*/
int cgi_manager::operator <<(char* str)
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
			u_long j=0;
			u_long min=min(strlen(&localEnv[i+variableNameLen+1]),(u_long)(*lpvBuffer)-1); 
			for(j=0;j<min;j++)
				lpvBuffer[j]=localEnv[i+variableNameLen+1+j];
			lpvBuffer[j]='\0';
			break;
		}
		else if((localEnv[i]=='\0') && (localEnv[i+1]=='\0'))
		{
			break;
		}
	}
	*lpdwSize=(u_int)strlen((char*)lpvBuffer);
}

