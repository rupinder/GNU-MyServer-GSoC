/*
*MyServer
*Copyright (C) 2002 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "../include/security.h"
#include "../include/AMMimeUtils.h"
#include "../include/filemanager.h"
#include "../include/sockets.h"
#include "../include/http_headers.h"
#include "../include/http.h"
#include "../include/HTTPmsg.h"
#include "../include/utility.h"
#include "../include/winCGI.h"
#define WINCGI_TIMEOUT	(10000)
extern "C" 
{
#ifdef WIN32
#include <direct.h>
#endif
#include <string.h>
}

int wincgi::sendWINCGI(httpThreadContext* td,LPCONNECTION s,char* filename)
{
#ifdef WIN32
	u_long nbr;
	char cmdLine[MAX_PATH + 120];
	char  dataFilePath[MAX_PATH], outFilePath[MAX_PATH];

	MYSERVER_FILE DataFileHandle, OutFileHandle;

	if(!MYSERVER_FILE::fileExists(filename))
		return ((http*)td->lhttp)->raiseHTTPError(td,s,e_404);

	char execname[MAX_PATH];
	char pathname[MAX_PATH];
	MYSERVER_FILE::splitPath(filename,pathname,execname);
	
	getdefaultwd(dataFilePath,MAX_PATH);
	GetShortPathName(dataFilePath,dataFilePath,MAX_PATH);
	sprintf(&dataFilePath[strlen(dataFilePath)],"/data_%u.ini",td->id);
	
	strcpy(outFilePath,td->outputDataPath);
	td->inputData.setFilePointer(0);
	/*!
	*The WinCGI protocol uses a .ini file for the communication between the processes.
	*/
	int ret=DataFileHandle.openFile(dataFilePath,MYSERVER_FILE_CREATE_ALWAYS|MYSERVER_FILE_OPEN_WRITE);
	if ((!ret) || (ret==-1)) 
	{
		strcpy(td->buffer,"Error creating WinCGI file\r\n");
		((vhost*)td->connection->host)->warningsLogWrite(td->buffer);
		return ((http*)td->lhttp)->raiseHTTPError(td,s,e_500);
	}


	strcpy(td->buffer2,"[CGI]\r\n");
	DataFileHandle.writeToFile(td->buffer2,7,&nbr);

	strcpy(td->buffer2,"CGI Version=CGI/1.3a WIN\r\n");
	DataFileHandle.writeToFile(td->buffer2,26,&nbr);

	sprintf(td->buffer2,"Server Admin=%s\r\n",lserver->getServerAdmin());
	DataFileHandle.writeToFile(td->buffer2,(u_long)strlen(td->buffer2),&nbr);

	if(strcmpi(td->request.CONNECTION,"Keep-Alive"))
	{
		strcpy(td->buffer2,"Request Keep-Alive=No\r\n");
		DataFileHandle.writeToFile(td->buffer2,23,&nbr);
	}
	else
	{
		strcpy(td->buffer2,"Request Keep-Alive=Yes\r\n");
		DataFileHandle.writeToFile(td->buffer2,24,&nbr);
	}

	sprintf(td->buffer2,"Request Method=%s\r\n",td->request.CMD);
	DataFileHandle.writeToFile(td->buffer2,(u_long)strlen(td->buffer2),&nbr);

	sprintf(td->buffer2 ,"Request Protocol=HTTP/%s\r\n",td->request.VER);
	DataFileHandle.writeToFile(td->buffer2,(u_long)strlen(td->buffer2),&nbr);

	sprintf(td->buffer2,"Executable Path=%s\r\n",execname);
	DataFileHandle.writeToFile(td->buffer2,(u_long)strlen(td->buffer2),&nbr);

	if(td->request.URIOPTS[0])
	{
		sprintf(td->buffer2,"Query String=%s\r\n",td->request.URIOPTS);
		DataFileHandle.writeToFile(td->buffer2,(u_long)strlen(td->buffer2),&nbr);
	}
	if(td->request.REFERER[0])
	{
		sprintf(td->buffer2,"Referer=%s\r\n",td->request.REFERER);
		DataFileHandle.writeToFile(td->buffer2,(u_long)strlen(td->buffer2),&nbr);
	}
	if(td->request.CONTENT_TYPE[0])
	{
		sprintf(td->buffer2,"Content Type=%s\r\n",td->request.CONTENT_TYPE);
		DataFileHandle.writeToFile(td->buffer2,(u_long)strlen(td->buffer2),&nbr);
	}

	if(td->request.USER_AGENT[0])
	{
		sprintf(td->buffer2,"User Agent=%s\r\n",td->request.USER_AGENT);
		DataFileHandle.writeToFile(td->buffer2,(u_long)strlen(td->buffer2),&nbr);
	}

	sprintf(td->buffer2,"Content File=%s\r\n",td->inputData.getFilename());
	DataFileHandle.writeToFile(td->buffer2,(u_long)strlen(td->buffer2),&nbr);

	if(td->request.CONTENT_LENGTH[0])
	{
		sprintf(td->buffer2,"Content Length=%s\r\n",td->request.CONTENT_LENGTH);
		DataFileHandle.writeToFile(td->buffer2,(u_long)strlen(td->buffer2),&nbr);
	}
	else
	{
		strcpy(td->buffer2,"Content Length=0\r\n");
		DataFileHandle.writeToFile(td->buffer2,18,&nbr);	
	}

	strcpy(td->buffer2,"Server Software=MyServer\r\n");
	DataFileHandle.writeToFile(td->buffer2,26,&nbr);

	sprintf(td->buffer2,"Remote Address=%s\r\n",td->connection->ipAddr);
	DataFileHandle.writeToFile(td->buffer2,(u_long)strlen(td->buffer2),&nbr);

	sprintf(td->buffer2,"Server Port=%u\r\n",td->connection->localPort);
	DataFileHandle.writeToFile(td->buffer2,(u_long)strlen(td->buffer2),&nbr);

	sprintf(td->buffer2,"Server Name=%s\r\n",td->request.HOST);
	DataFileHandle.writeToFile(td->buffer2,(u_long)strlen(td->buffer2),&nbr);

	strcpy(td->buffer2,"[System]\r\n");
	DataFileHandle.writeToFile(td->buffer2,10,&nbr);

	sprintf(td->buffer2,"Output File=%s\r\n",outFilePath);
	DataFileHandle.writeToFile(td->buffer2,(u_long)strlen(td->buffer2),&nbr);

	sprintf(td->buffer2,"Content File=%s\r\n",td->inputData.getFilename());
	DataFileHandle.writeToFile(td->buffer2,(u_long)strlen(td->buffer2),&nbr);
	/*!
	*Compute the local offset from the GMT time
	*/
	time_t ltime=100;
	int gmhour=gmtime( &ltime)->tm_hour;
	int bias=localtime(&ltime)->tm_hour-gmhour;

	sprintf(td->buffer2,"GMT Offset=%i\r\n",bias);
	DataFileHandle.writeToFile(td->buffer2,(u_long)strlen(td->buffer2),&nbr);

	sprintf(td->buffer2,"Debug Mode=No\r\n",bias);
	DataFileHandle.writeToFile(td->buffer2,15,&nbr);

	DataFileHandle.closeFile();

	/*!
	*Create the out file.
	*/
	if(!MYSERVER_FILE::fileExists(outFilePath))
	{
		ret = OutFileHandle.openFile(outFilePath,MYSERVER_FILE_CREATE_ALWAYS);
		if ((!ret) || (ret==-1)) 
		{
			sprintf(td->buffer,"Error creating WinCGI output file\r\n");
			((vhost*)td->connection->host)->warningsLogWrite(td->buffer);
			DataFileHandle.closeFile();
			MYSERVER_FILE::deleteFile(outFilePath);
			MYSERVER_FILE::deleteFile(dataFilePath);
			return ((http*)td->lhttp)->raiseHTTPError(td,s,e_500);
		}
	}
	OutFileHandle.closeFile();
	strcpy(cmdLine,"cmd /c \"");
	strcat(cmdLine, filename);
	strcat(cmdLine, "\" ");
	strcat(cmdLine, dataFilePath);
	START_PROC_INFO spi;
	memset(&spi,0,sizeof(spi));
	spi.cwd = pathname;
	spi.cmdLine = cmdLine;
	
	if (execHiddenProcess(&spi,WINCGI_TIMEOUT))
	{
		sprintf(td->buffer,"Error executing WinCGI process\r\n");
		((vhost*)td->connection->host)->warningsLogWrite(td->buffer);
		MYSERVER_FILE::deleteFile(outFilePath);
		MYSERVER_FILE::deleteFile(dataFilePath);
		return ((http*)td->lhttp)->raiseHTTPError(td,s,e_500);
	}

	ret=OutFileHandle.openFile(outFilePath,MYSERVER_FILE_OPEN_ALWAYS|MYSERVER_FILE_OPEN_READ);
	if ((!ret) || (ret==-1)) 
	{
		sprintf(td->buffer,"Error opening WinCGI output file\r\n");
		((vhost*)td->connection->host)->warningsLogWrite(td->buffer);
		return ((http*)td->lhttp)->raiseHTTPError(td,s,e_500);
	}
	u_long nBytesRead=0;
	OutFileHandle.readFromFile(td->buffer2,KB(5),&nBytesRead);
	if(nBytesRead==0)
	{
		sprintf(td->buffer,"Error zero bytes read from the WinCGI output file\r\n");
		((vhost*)td->connection->host)->warningsLogWrite(td->buffer);
		OutFileHandle.closeFile();
		MYSERVER_FILE::deleteFile(outFilePath);
		MYSERVER_FILE::deleteFile(dataFilePath);
		return ((http*)td->lhttp)->raiseHTTPError(td,s,e_500);
	}
	u_long headerSize=0;

	for(u_long i=0;i<nBytesRead;i++)
	{
		if((td->buffer2[i]=='\r')&&(td->buffer2[i+1]=='\n')&&(td->buffer2[i+2]=='\r')&&(td->buffer2[i+3]=='\n'))
		{
			/*!
			*The HTTP header ends with a \r\n\r\n sequence so 
			*determinate where it ends and set the header size
			*to i + 4.
			*/
			headerSize=i+4;
			break;
		}
	}
	buildHTTPResponseHeaderStruct(&td->response,td,td->buffer2);
	/*!
	*Always specify the size of the HTTP contents.
	*/
	sprintf(td->response.CONTENT_LENGTH,"%u",OutFileHandle.getFileSize()-headerSize);
	buildHTTPResponseHeader(td->buffer,&td->response);
	s->socket.send(td->buffer,(int)strlen(td->buffer), 0);
	s->socket.send((char*)(td->buffer2+headerSize),nBytesRead-headerSize, 0);
	while(OutFileHandle.readFromFile(td->buffer2,td->buffersize2,&nBytesRead))
	{
		if(nBytesRead)
			s->socket.send((char*)td->buffer2,nBytesRead, 0);
		else
			break;
	}
	
	OutFileHandle.closeFile();
	MYSERVER_FILE::deleteFile(outFilePath);
	MYSERVER_FILE::deleteFile(dataFilePath);
	return !strcmpi(td->request.CONNECTION,"Keep-Alive");
#endif
#ifdef __linux__
	sprintf(td->buffer,"Error WinCGI is not implemented\r\n");
	((vhost*)td->connection->host)->warningsLogWrite(td->buffer);
	return ((http*)td->lhttp)->raiseHTTPError(td,s,e_501);/*!WinCGI is not available under linux*/
#endif
}
