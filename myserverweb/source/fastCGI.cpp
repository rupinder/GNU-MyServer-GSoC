/*
*myServer
*Copyright (C) 2002 Giuseppe Scrivano
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
 
#include "../include/fastCGI.h"
#define MAX_FCGI_SERVERS	25
static struct sfCGIservers
{
	char path[MAX_PATH];/*exec path*/
	MYSERVER_SOCKET socket;
	int pid; /*process ID*/
	int port;/*IP port*/
}fCGIservers[MAX_FCGI_SERVERS];
static int fCGIserversN;
/*
*Entry-Point to manage a FastCGI request.
*/
int sendFASTCGI(httpThreadContext* td,LPCONNECTION connection,char* scriptpath,char* /*ext*/,char *cgipath)
{
	fCGIContext con;
	con.td=td;
	u_long nbr;
	FCGI_Header tHeader;
	strcpy(td->scriptPath,scriptpath);
	MYSERVER_FILE::splitPath(scriptpath,td->scriptDir,td->scriptFile);
	MYSERVER_FILE::splitPath(cgipath,td->cgiRoot,td->cgiFile);
	buildCGIEnvironmentString(td,td->buffer);
	int sizeEnvString=buildFASTCGIEnvironmentString(td,td->buffer,td->buffer2);

	int pID = FcgiConnect(&con,cgipath);
	if(pID<0)
		return raiseHTTPError(td,connection,e_501);

	int id=td->id;
	FCGI_BeginRequestBody tBody;
	tBody.roleB1 = ( FCGI_RESPONDER >> 8 ) & 0xff;
	tBody.roleB0 = ( FCGI_RESPONDER ) & 0xff;
	tBody.flags = 0;
	memset( tBody.reserved, 0, sizeof( tBody.reserved ) );

	if(sendFcgiBody(&con,(char*)&tBody,sizeof(tBody),FCGI_BEGIN_REQUEST,id))
	{
		con.sock.ms_closesocket();
		return raiseHTTPError(td,connection,e_501);
	}

	if(sendFcgiBody(&con,td->buffer2,sizeEnvString,FCGI_PARAMS,id))
	{
		con.sock.ms_closesocket();
		return raiseHTTPError(td,connection,e_501);
	}
    if(atoi(td->request.CONTENTS_DIM))
	{
		generateFcgiHeader( tHeader, FCGI_STDIN, id, atoi(td->request.CONTENTS_DIM) );
		fCGIservers[con.fcgiPID].socket.ms_send((char*)&tHeader,sizeof(tHeader),0);
		td->inputData.ms_setFilePointer(0);
		while(nbr=td->inputData.ms_ReadFromFile(td->buffer,td->buffersize,&nbr))
		{
			fCGIservers[con.fcgiPID].socket.ms_send(td->buffer,nbr,0);
		}
	}
	else
	{
		if(sendFcgiBody(&con,0,0,FCGI_STDIN,id))
		{
			con.sock.ms_closesocket();
			return raiseHTTPError(td,connection,e_501);
		}	
	}
	/*Now read the output*/
	int exit=0;
	do	
	{
		nbr=con.sock.ms_recv((char*)&tHeader,sizeof(FCGI_Header),0);
		if(nbr<sizeof(FCGI_Header))
		{
			con.sock.ms_closesocket();
			return raiseHTTPError(td,connection,e_501);
		}
		int dim=(tHeader.contentLengthB1<<8) + tHeader.contentLengthB0;
		int headerSize;
		switch(tHeader.type)
		{
			case FCGI_STDERR:
				return raiseHTTPError(td,connection,e_501);
			case FCGI_STDOUT:
				nbr=con.sock.ms_recv(td->buffer,td->buffersize,0);
	
				for(u_long i=0;i<nbr;i++)
				{
					if((td->buffer[i]=='\r')&&(td->buffer[i+1]=='\n')&&(td->buffer[i+2]=='\r')&&(td->buffer[i+3]=='\n'))
					{
						headerSize=i+4;
						break;
					}
				}
				sprintf(td->response.CONTENTS_DIM,"%u",dim-headerSize);
				buildHTTPResponseHeaderStruct(&td->response,td,td->buffer2);
				td->connection->socket.ms_send(td->buffer2,(int)strlen(td->buffer2), 0);
				td->connection->socket.ms_send((char*)(td->buffer2+headerSize),nbr-headerSize, 0);
				while(nbr=fCGIservers[con.fcgiPID].socket.ms_recv(td->buffer,td->buffersize,0))
				{
					td->connection->socket.ms_send((char*)(td->buffer),nbr, 0);
				}
				break;
			case FCGI_END_REQUEST:
				exit = 1;
				break;			
			case FCGI_GET_VALUES_RESULT:
			case FCGI_UNKNOWN_TYPE:
			default:
				break;
		}
	}while((!exit) && nbr);
	con.sock.ms_closesocket();
	return 1;
}

int sendFcgiBody(fCGIContext* con,char* buffer,int len,int type,int id)
{
	FCGI_Header tHeader;
	generateFcgiHeader( tHeader, type, id, len );
	
	con->sock.ms_send((char*)&tHeader,sizeof(tHeader),0);
	con->sock.ms_send(buffer,len,0);
	return 0;
}
/*
*Trasform from an environment string to the FastCGI environment string.
*/
int buildFASTCGIEnvironmentString(httpThreadContext* td,char* sp,char* ep)
{
	int j=0;
	char *ptr=ep;
	char *sptr=sp;
	int xx=0;
	char varName[100];
	char varValue[300];
	char varBuffer[400];
	for(;ptr-ep<800;)
	{
		if(*(sptr+1)=='\0')
			break;
		unsigned char varBufferLen=0,varNameLen=0,varValueLen=0;
		varName[0]='\0';
		varBuffer[0]='\0';
		varValue[0]='\0';
		while(*sptr != '\0')
		{
			varBuffer[varBufferLen++]=*sptr;
			varBuffer[varBufferLen]='\0';
			sptr++;
		}
		sptr++;
		j=0;
		while((varBuffer[j] != '=') && (j<300))
		{
			varName[varNameLen++]=varBuffer[j];
			varName[varNameLen]='\0';
			j++;
		}
		j++;
		while(varBuffer[j] != '\0')
		{
			varValue[varValueLen++]=varBuffer[j];
			varValue[varValueLen]='\0';
			j++;
		}
		memcpy(ptr,(void*)&varNameLen,sizeof(varNameLen));
		ptr+=sizeof(varNameLen);
		memcpy(ptr,&varValueLen,sizeof(varValueLen));
		ptr+=sizeof(varValueLen);
		memcpy(ptr,varName,varNameLen);
		ptr+=varNameLen;
		memcpy(ptr,varValue,varValueLen);
		ptr+=varValueLen;

	}
	memcpy(ptr,"\0",1);
	return (int)(ptr-ep);
}
void generateFcgiHeader( FCGI_Header &tHeader, int iType,int iRequestId, int iContentLength )
{
	tHeader.version = FCGI_VERSION_1;
	tHeader.type = iType;
	tHeader.requestIdB1 = (iRequestId >> 8 ) & 0xff;
	tHeader.requestIdB0 = (iRequestId ) & 0xff;
	tHeader.contentLengthB1 = (iContentLength >> 8 ) & 0xff;
	tHeader.contentLengthB0 = (iContentLength ) & 0xff;
	tHeader.paddingLength = 0;
};

int initializeFASTCGI()
{
	fCGIserversN=0;

	return 1;
}
int cleanFASTCGI()
{
	for(int i=0;i<fCGIserversN;i++)
	{
		fCGIservers[i].socket.ms_closesocket();
		terminateProcess(fCGIservers[i].pid);
	}
	return 1;
}
int isFcgiServerRunning(char* path)
{
	for(int i=0;i<fCGIserversN;i++)
	{
#ifdef WIN32
		if(_stricmp(path,fCGIservers[i].path))
#endif
#ifdef __linux__
		if(stricmp(path,fCGIservers[i].path))
#endif
			return i;
	}
	return 0;
}
int FcgiConnect(fCGIContext* con,char* path)
{
	int pID;
	unsigned long pLong = 1L;
	if(pID=runFcgiServer(con,path)>=0)
	{
		fCGIservers[pID].socket.ms_getHandle();
		MYSERVER_HOSTENT *hp=MYSERVER_SOCKET::ms_gethostbyname("localhost");

		struct sockaddr_in sockAddr;
		int sockLen = sizeof(sockAddr);
        memset(&sockAddr, 0, sizeof(sockAddr));
        sockAddr.sin_family = AF_INET;
	    memcpy(&sockAddr.sin_addr, hp->h_addr, hp->h_length);
	    sockAddr.sin_port = htons(fCGIservers[pID].port);
		con->sock.ms_socket(AF_INET, SOCK_STREAM, 0);
		con->sock.ms_connect((MYSERVER_SOCKADDR*)&sockAddr, sockLen);
		con->sock.ms_ioctlsocket(FIONBIO, &pLong);
		con->fcgiPID=pID;
	}
	return pID;
}
int runFcgiServer(fCGIContext *con,char* path)
{
	int pID;
	if(pID=isFcgiServerRunning(path))
		return pID;
	if(fCGIserversN==MAX_FCGI_SERVERS-2)
		return -1;
	static int port=3333;
	{
		/*SERVER SOCKET CREATION*/
		fCGIservers[fCGIserversN].socket.ms_socket(AF_INET,SOCK_STREAM,0);
		MYSERVER_SOCKADDRIN sock_inserverSocket;
		sock_inserverSocket.sin_family=AF_INET;
		sock_inserverSocket.sin_addr.s_addr=htonl(INADDR_ANY);
		fCGIservers[fCGIserversN].port=port;
		sock_inserverSocket.sin_port=htons((u_short)port++);
		if(fCGIservers[fCGIserversN].socket.ms_bind((sockaddr*)&sock_inserverSocket,sizeof(sock_inserverSocket))!=0)
		{
			fCGIservers[fCGIserversN].socket.ms_closesocket();
			return -2;
		}
		fCGIservers[fCGIserversN].socket.ms_listen(SOMAXCONN);
	}
	START_PROC_INFO spi;
	memset(&spi,0,sizeof(spi));
	char cmd[MAX_PATH];
	spi.cmd=cmd;
	spi.stdIn = (MYSERVER_FILE_HANDLE)fCGIservers[fCGIserversN].socket.ms_getHandle();
	spi.cwd=con->td->cgiRoot;
	spi.arg=con->td->buffer2;
	spi.cmdLine=cmd;
	sprintf(spi.cmd,"%s/%s",con->td->cgiRoot,con->td->cgiFile);
	spi.stdOut = spi.stdError =(MYSERVER_FILE_HANDLE) -1;
	fCGIservers[fCGIserversN].pid=execConcurrentProcess(&spi);
    
	strcpy(fCGIservers[fCGIserversN].path,spi.cmd);
	
	if(fCGIservers[fCGIserversN].pid)
		fCGIserversN++;
	else
		return -3;

	return fCGIserversN;
}