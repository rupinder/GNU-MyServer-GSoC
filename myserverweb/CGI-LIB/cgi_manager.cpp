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
#include "../stdafx.h"
#include "../include/cXMLParser.h"
#include "cgi_manager.h"
#include "../include/Response_RequestStructs.h"
#include "../include/http.h"
#include "../include/mscgi.h"

#undef min
#undef max
#define min(a,b)		((a<b)?a:b)
#define max(a,b)		((a>b)?a:b)

#ifdef WIN32
#pragma comment(lib,"wsock32.lib")
#endif

/*!
 *Write to the stdout.
 */
int CgiManager::Write(char* str)
{
	if(str)
	{
		u_long nbw;
		cgidata->stdOut.writeToFile(str,(u_long)strlen(str),&nbw);
		return 1;
	}
	return 0;
}
/*!
 *Write binary to the stdout.
 */
int CgiManager::Write(void* data, int len)
{
	if(data)
	{
		u_long nbw;
		cgidata->stdOut.writeToFile((char*)data,(u_long)len,&nbw);
		return 1;
	}
	return 0;
}

/*!
 *Start the execution of the CGI.
 */
int CgiManager::Start(MsCgiData* data)
{
	cgidata=data;
	td=data->td;
	return 1;
}
/*!
 *Clean the memory allocated by the CGI.
 */
int CgiManager::Clean()
{
	return 1;
}
/*!
 *Set the HTTP error identifier.
 */
int CgiManager::setPageError(int ID)
{
	td->response.httpStatus=ID;
	return 1;
}
/*!
 *Raise an HTTP error
 */
int CgiManager::raiseError(int ID)
{
	cgidata->errorPage=ID;
	return 1;
}
/*!
 *Constructor of the class
 */
CgiManager::CgiManager(MsCgiData* data)
{
	Start(data);
}
/*!
 *Destructor of the class
 */
CgiManager::~CgiManager(void)
{
	Clean();
}
/*!
 *Returns the value of a param passed through the URL.
 */
char* CgiManager::GetParam(char* param)
{
	if(td->request.URIOPTS[0]=='\0')
		return 0;
	localbuffer[0]='\0';
	char *c=&(td->request.URIOPTS)[0];
	for(;;)
	{
		while((*c) && strncmp(c,param,min( strlen(param),strlen(c) )))c++;
		if(*c=='\0')
		{
			return &localbuffer[0];
		}
		c+=strlen(param);
		if(*c=='=')
		{
			c++;
			break;
		}
	}
	u_long len=0;
	while((c[len]) && (c[len]!='&') && (len < LOCAL_BUFFER_DIM-1 ))
	{
		localbuffer[len]=c[len];
		localbuffer[len+1]='\0';
		len++;
	}
	return localbuffer;
}
/*!
 *Returns the value of a param passed through a POST request.
 */
char* CgiManager::PostParam(char* param)
{
	char buffer[LOCAL_BUFFER_DIM+50];
	u_long nbr=0;
	char c[2];
	c[1]='\0';
	buffer[0]='\0';
	localbuffer[0]='\0';
	
	u_long toRead=td->inputData.getFileSize();
	
	if(!toRead)
		return 0;
	do
	{
		cgidata->td->inputData.readFromFile(&c[0],1,&nbr);
		if(nbr==0)
			break;
		if((c[0]=='&') |(toRead==1))
		{
			if(!strncmp(param,buffer,strlen(param)))
			{
				lstrcpyn(localbuffer,&buffer[strlen(param)],LOCAL_BUFFER_DIM);
				break;
			}
			buffer[0]='\0';
		}
		else
		{
			if(lstrlen(buffer)+1 < LOCAL_BUFFER_DIM + 50)
			{
				strcat(buffer,c);
			}
		}
	}while(--toRead);

	return localbuffer;

}
/*!
 *Write to stdout.
 */
int CgiManager::operator <<(char* str)
{
	return Write(str);
}
/*!
 *Read from the stdin.
 */
char *CgiManager::operator >>(char* str)
{
	/*!
   *If it is a POST request return a param from the POST values
   *else return a GET param.
   */
	if(td->request.URIOPTSPTR)
		return PostParam(str);
	else
		return GetParam(str);
} 
/*!
 *Get the value of an environment variable.
 */
void CgiManager::getenv(char* lpszVariableName,char *lpvBuffer,unsigned int* lpdwSize)
{
	((char*)lpvBuffer)[0]='\0';
	char *localEnv=cgidata->envString;
	size_t variableNameLen=strlen(lpszVariableName);
	for(u_long i=0;;i+=(u_long)strlen(&localEnv[i])+1)
	{
		if(((localEnv[i+variableNameLen])== '=')&&(!strncmp(&localEnv[i],lpszVariableName,variableNameLen)))
		{
			u_long j=0;
			u_long min_v=min(strlen(&localEnv[i+variableNameLen+1]),(u_long)(*lpvBuffer)-1); 
			for(j=0;j<min_v;j++)
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
/*!
 *Returns the CGI data structure. 
 *This structure is shared with the MyServer core so use it carefully!
 */
MsCgiData* CgiManager::getCgiData()
{
	return cgidata;
}
/*!
 *Specify the MIME type for the data.
 */
void CgiManager::setContentType(char * Type)
{
	strncpy(td->response.CONTENT_TYPE, Type, HTTP_RESPONSE_CONTENT_TYPE_DIM);
}
