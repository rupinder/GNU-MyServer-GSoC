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
#include "../include/cserver.h"
#include "../include/security.h"
#include "../include/AMMimeUtils.h"
#include "../include/filemanager.h"
#include "../include/sockets.h"
#include "../include/utility.h"
#define WINCGI_TIMEOUT	(10000)
extern "C" {
#ifdef WIN32
#include <direct.h>
#endif
#include <string.h>
}

int sendWINCGI(httpThreadContext* td,LPCONNECTION s,char* filename)
{
#ifdef WIN32
	u_long nbr;
	char cmdLine[MAX_PATH + 120];
	char conFilePath[MAX_PATH], dataFilePath[MAX_PATH], outFilePath[MAX_PATH];

	MYSERVER_FILE_HANDLE ConFileHandle, DataFileHandle, OutFileHandle;

	if(!ms_FileExists(filename))
		return raiseHTTPError(td,s,e_404);

	ms_getdefaultwd(dataFilePath,MAX_PATH);
	sprintf(&dataFilePath[lstrlen(dataFilePath)],"/dataFilePath__%u.ini",td->id);

	ms_getdefaultwd(conFilePath,MAX_PATH);
	sprintf(&conFilePath[lstrlen(conFilePath)],"/conFilePath__%u",td->id);

	ms_getdefaultwd(outFilePath,MAX_PATH);
	sprintf(&outFilePath[lstrlen(outFilePath)],"/outFilePath__%u",td->id);

	ConFileHandle = ms_OpenFile(conFilePath,MYSERVER_FILE_CREATE_ALWAYS|MYSERVER_FILE_OPEN_WRITE);

	if ((!ConFileHandle)|((int)ConFileHandle == -1))
	{
		return raiseHTTPError(td,s,e_501);
	}
	if(td->request.URIOPTSPTR)
	{
		ms_WriteToFile(ConFileHandle,td->request.URIOPTSPTR,strlen(td->request.URIOPTSPTR),&nbr);
	}
	else if(td->inputData)
	{
		ms_setFilePointer(td->inputData,0);
		while(ms_ReadFromFile(td->inputData,td->buffer2,td->buffersize2,&nbr))
		{
			ms_WriteToFile(ConFileHandle,td->buffer2,nbr,&nbr);		
		}
	}

	ms_CloseFile(ConFileHandle);


	DataFileHandle = ms_OpenFile(dataFilePath,MYSERVER_FILE_CREATE_ALWAYS|MYSERVER_FILE_OPEN_WRITE);
	if ((!DataFileHandle) || ((int)DataFileHandle==-1)) 
	{
		return raiseHTTPError(td,s,e_501);
	}


	strcpy(td->buffer2,"[CGI]\r\n");
	ms_WriteToFile(DataFileHandle,td->buffer2,7,&nbr);

	strcpy(td->buffer2,"CGI Version=CGI/1.3a WIN\r\n");
	ms_WriteToFile(DataFileHandle,td->buffer2,26,&nbr);

	if(strcmp(td->request.CONNECTION,"Keep-Alive"))
	{
		strcpy(td->buffer2,"Request Keep-Alive=No\r\n");
		ms_WriteToFile(DataFileHandle,td->buffer2,23,&nbr);
	}
	else
	{
		strcpy(td->buffer2,"Request Keep-Alive=Yes\r\n");
		ms_WriteToFile(DataFileHandle,td->buffer2,24,&nbr);
	}

	sprintf(td->buffer2,"Request Method=%s\r\n",td->request.CMD);
	ms_WriteToFile(DataFileHandle,td->buffer2,strlen(td->buffer2),&nbr);

	sprintf(td->buffer2 ,"Request Protocol=HTTP/%s\r\n",td->request.VER);
	ms_WriteToFile(DataFileHandle,td->buffer2,strlen(td->buffer2),&nbr);

	sprintf(td->buffer2,"Executable Path=%s\r\n",filename);
	ms_WriteToFile(DataFileHandle,td->buffer2,strlen(td->buffer2),&nbr);

	if(td->request.URIOPTS[0])
	{
		sprintf(td->buffer2,"Query String=%s\r\n",td->request.URIOPTS);
		ms_WriteToFile(DataFileHandle,td->buffer2,strlen(td->buffer2),&nbr);
	}
	if(td->request.REFERER[0])
	{
		sprintf(td->buffer2,"Referer=%s\r\n",td->request.REFERER);
		ms_WriteToFile(DataFileHandle,td->buffer2,strlen(td->buffer2),&nbr);
	}
	if(td->request.CONTENTS_TYPE[0])
	{
		sprintf(td->buffer2,"Content Type=%s\r\n",td->request.CONTENTS_TYPE);
		ms_WriteToFile(DataFileHandle,td->buffer2,strlen(td->buffer2),&nbr);
	}

	if(td->request.USER_AGENT[0])
	{
		sprintf(td->buffer2,"User Agent=%s\r\n",td->request.USER_AGENT);
		ms_WriteToFile(DataFileHandle,td->buffer2,strlen(td->buffer2),&nbr);
	}

	sprintf(td->buffer2,"Content File=%s\r\n",conFilePath);
	ms_WriteToFile(DataFileHandle,td->buffer2,strlen(td->buffer2),&nbr);

	if(td->request.CONTENTS_DIM[0])
	{
		sprintf(td->buffer2,"Content Length=%s\r\n",td->request.CONTENTS_DIM);
		ms_WriteToFile(DataFileHandle,td->buffer2,strlen(td->buffer2),&nbr);
	}
	else
	{
		strcpy(td->buffer2,"Content Length=0\r\n");
		ms_WriteToFile(DataFileHandle,td->buffer2,18,&nbr);	
	}

	strcpy(td->buffer2,"Server Software=myServer\r\n");
	ms_WriteToFile(DataFileHandle,td->buffer2,26,&nbr);

	sprintf(td->buffer2,"Remote Address=%s\r\n",td->connection->ipAddr);
	ms_WriteToFile(DataFileHandle,td->buffer2,strlen(td->buffer2),&nbr);

	sprintf(td->buffer2,"Server Port=%u\r\n",td->connection->localPort);
	ms_WriteToFile(DataFileHandle,td->buffer2,strlen(td->buffer2),&nbr);

	sprintf(td->buffer2,"Server Name=%s\r\n",lserver->getServerName());
	ms_WriteToFile(DataFileHandle,td->buffer2,strlen(td->buffer2),&nbr);

	strcpy(td->buffer2,"[System]");
	ms_WriteToFile(DataFileHandle,td->buffer2,8,&nbr);

	sprintf(td->buffer2,"Output File=%s\r\n",outFilePath);
	ms_WriteToFile(DataFileHandle,td->buffer2,strlen(td->buffer2),&nbr);

	sprintf(td->buffer2,"Content File=%s\r\n",conFilePath);
	ms_WriteToFile(DataFileHandle,td->buffer2,strlen(td->buffer2),&nbr);

	time_t ltime=0;
	int bias=localtime(&ltime)->tm_hour-gmtime( &ltime)->tm_hour;

	sprintf(td->buffer2,"GMT Offset=%i\r\n",bias);
	ms_WriteToFile(DataFileHandle,td->buffer2,strlen(td->buffer2),&nbr);

	sprintf(td->buffer2,"Debug Mode=No\r\n",bias);
	ms_WriteToFile(DataFileHandle,td->buffer2,15,&nbr);


	ms_CloseFile(DataFileHandle);


	OutFileHandle = ms_OpenFile(outFilePath,MYSERVER_FILE_CREATE_ALWAYS|MYSERVER_FILE_OPEN_WRITE);
	if ((!OutFileHandle) || ((int)OutFileHandle==-1)) 
	{
		return raiseHTTPError(td,s,e_501);
	}
	ms_CloseFile(OutFileHandle);

	strcpy(cmdLine, filename);
	strcat(cmdLine, " ");
	strcat(cmdLine, dataFilePath);
	START_PROC_INFO spi;
	memset(&spi,sizeof(spi),0);
	spi.cmdLine = cmdLine;
	
	if (execHiddenProcess(&spi,WINCGI_TIMEOUT) )
	{
		return raiseHTTPError(td,s,e_501);
	}
	sendHTTPFILE(td,s,outFilePath);
	ms_DeleteFile(outFilePath);
	ms_DeleteFile(conFilePath);
	ms_DeleteFile(dataFilePath);
	return 1;
#else
	return raiseHTTPError(td,s,e_501);
#endif
}


