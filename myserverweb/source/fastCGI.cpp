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
	union 
	{
	    /*HANDLE*/unsigned long fileHandle;
		SOCKET sock;
		unsigned int value;
	}DESCRIPTOR;
	MYSERVER_SOCKET socket;
	int pid; /*process ID*/ 
	u_short port;/*IP port*/
}fCGIservers[MAX_FCGI_SERVERS];
struct fourchar
{	
	union
	{
		unsigned int i;
		unsigned char c[4];
	};
};
static int fCGIserversN;
/*
*Entry-Point to manage a FastCGI request.
*/
int sendFASTCGI(httpThreadContext* td,LPCONNECTION connection,char* scriptpath,char* /*ext*/,char *cgipath)
{
	fCGIContext con;
	con.td=td;
	u_long nbr=0;
	FCGI_Header tHeader;
	strcpy(td->scriptPath,scriptpath);
	MYSERVER_FILE::splitPath(scriptpath,td->scriptDir,td->scriptFile);
	MYSERVER_FILE::splitPath(cgipath,td->cgiRoot,td->cgiFile);
	td->buffer[0]='\0';
	buildCGIEnvironmentString(td,td->buffer);

	int sizeEnvString=buildFASTCGIEnvironmentString(td,td->buffer,td->buffer2);
	td->inputData.ms_CloseFile();
	td->inputData.ms_OpenFile(td->inputDataPath,MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_IFEXISTS|MYSERVER_FILE_NO_INHERIT);

	int pID = FcgiConnect(&con,cgipath);
	if(pID<0)
		return raiseHTTPError(td,connection,e_500);

	int id=td->id+1;
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

	if(sendFcgiBody(&con,0,0,FCGI_PARAMS,id))
	{
		con.sock.ms_closesocket();
		return raiseHTTPError(td,connection,e_501);
	}	
    if(atoi(td->request.CONTENT_LENGTH))
	{
		generateFcgiHeader( tHeader, FCGI_STDIN, id, atoi(td->request.CONTENT_LENGTH));
		con.sock.ms_send((char*)&tHeader,sizeof(tHeader),0);
		td->inputData.ms_setFilePointer(0);
		do
		{
			td->inputData.ms_ReadFromFile(td->buffer,td->buffersize,&nbr);
			if(nbr)
				con.sock.ms_send(td->buffer,nbr,0);
		}while(nbr==td->buffersize);
	}
	if(sendFcgiBody(&con,0,0,FCGI_STDIN,id))
	{
		con.sock.ms_closesocket();
		return raiseHTTPError(td,connection,e_501);
	}	

	/*Now read the output*/
	int exit=0;
	const clock_t timeout= CLOCKS_PER_SEC * 20; // 20 seconds
	clock_t time1 = clock();
	do	
	{
		while(!con.sock.ms_bytesToRead())
		{
			if(clock()-time1>timeout)
				break;
		}
		if(con.sock.ms_bytesToRead())
			nbr=con.sock.ms_recv((char*)&tHeader,sizeof(FCGI_Header),0);
		else
		{
			sendFcgiBody(&con,0,0,FCGI_ABORT_REQUEST,id);
			con.sock.ms_shutdown(2);
			con.sock.ms_closesocket();
 			return raiseHTTPError(td,connection,e_500);
		}
		if(nbr<sizeof(FCGI_Header))
		{
			sendFcgiBody(&con,0,0,FCGI_ABORT_REQUEST,id);
			con.sock.ms_shutdown(2);
			con.sock.ms_closesocket();
			return raiseHTTPError(td,connection,e_500);
		}
		int dim=(tHeader.contentLengthB1<<8) + tHeader.contentLengthB0;
		int headerSize=0;
		int dataSent=0;
		if(dim==0)
		{
			exit = 1;
		}
		else
		{
			switch(tHeader.type)
			{
				case FCGI_STDERR:
					con.sock.ms_closesocket();
					raiseHTTPError(td,connection,e_501);
					exit = 1;
					break;
				case FCGI_STDOUT:
					nbr=con.sock.ms_recv(td->buffer,min(dim,td->buffersize),0);
		
					for(u_long i=0;i<nbr;i++)
					{
						if((td->buffer[i]=='\r')&&(td->buffer[i+1]=='\n')&&(td->buffer[i+2]=='\r')&&(td->buffer[i+3]=='\n'))
						{
							headerSize=i+4;
							break;
						}
					}
					sprintf(td->response.CONTENT_LENGTH,"%u",dim-headerSize);
					buildHTTPResponseHeaderStruct(&td->response,td,td->buffer);
					if(td->response.LOCATION[0])
					{
						char nURL[MAX_PATH];
						nURL[0]='\0';
						u_long j;
						if(!td->request.uriEndsWithSlash)
						{
							int slashcount=0,slashcountnow=0;
							for(j=0;j<strlen(td->request.URI);j++)
								if(td->request.URI[j]=='/')
									slashcount++;

							for(j=0;j<strlen(td->request.URI);j++)
							{
								if(td->request.URI[j]=='/')
								{
									slashcountnow++;
									if(slashcountnow==slashcount-1)
									{
										j++;
										int start=0;
										while(td->request.URI[j]!='/')
										{
											nURL[start]=td->request.URI[j]; 
											nURL[start+1]='\0'; 
											j++;
											start++;
										}
										nURL[start]='/'; 
										nURL[start+1]='\0'; 
									}
								}
							}
						}
						j=0;

						int start=(int)strlen(nURL);
						while(td->response.LOCATION[j])
						{

							nURL[j+start]=td->response.LOCATION[j];
							nURL[j+start+1]='\0';
							j++;
						}

						sendHTTPRedirect(td,connection,nURL);
						exit = 1;
						break;
					}
					buildHTTPResponseHeader(td->buffer2,&td->response);
					if(td->connection->socket.ms_send(td->buffer2,strlen(td->buffer2), 0)==0)
					{
						exit = 1;
						break;
					}
					dataSent=td->connection->socket.ms_send((char*)(td->buffer+headerSize),nbr-headerSize, 0)+headerSize;
					if(dataSent==headerSize)
					{
						exit = 1;
						break;
					}
					while(dataSent<dim)
					{
						if( con.sock.ms_bytesToRead() )
							nbr=con.sock.ms_recv(td->buffer,min(dim-dataSent,td->buffersize),0);
						else
						{
							exit = 1;
							break;
						}
						td->connection->socket.ms_send(td->buffer2,nbr, 0);
						dataSent+=nbr;
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
		}
	}while((!exit) && nbr);
	td->inputData.ms_CloseFile();
	MYSERVER_FILE::ms_DeleteFile(td->inputDataPath);
	con.sock.ms_closesocket();
	return 1;
}

int sendFcgiBody(fCGIContext* con,char* buffer,int len,int type,int id)
{
	FCGI_Header tHeader;
	generateFcgiHeader( tHeader, type, id, len );
	
	if(con->sock.ms_send((char*)&tHeader,sizeof(tHeader),0)==-1)
		return -1;
	if(con->sock.ms_send(buffer,len,0)==-1)
		return -1;
	return 0;
}
/*
*Trasform from an environment string to the FastCGI environment string.
*/
int buildFASTCGIEnvironmentString(httpThreadContext* td,char* sp,char* ep)
{
	char *ptr=ep;
	char *sptr=sp;
	char varName[100];
	char varValue[2500];
	int i;
	for(;;)
	{
		fourchar varNameLen;
		fourchar varValueLen;

		varNameLen.i=varValueLen.i=0;
		varName[0]='\0';
		varValue[0]='\0';
		while(*sptr != '=')
		{
			varName[varNameLen.i++]=*sptr++;
			varName[varNameLen.i]='\0';
		}
		sptr++;
		while(*sptr != '\0')
		{
			varValue[varValueLen.i++]=*sptr++;
			varValue[varValueLen.i]='\0';
		}
		if(varNameLen.i > 127)
		{
			unsigned char fb=varValueLen.c[3]|0x80;
			*ptr++=fb;
			*ptr++=varNameLen.c[2];
			*ptr++=varNameLen.c[1];
			*ptr++=varNameLen.c[0];
		}
		else
		{
			*ptr++=varNameLen.c[0];
		}

		if(varValueLen.i > 127)
		{
			unsigned char fb=varValueLen.c[3]|0x80;
			*ptr++=fb;
			*ptr++=varValueLen.c[2];
			*ptr++=varValueLen.c[1];
			*ptr++=varValueLen.c[0];
		}
		else
		{
			*ptr++=varValueLen.c[0];
		}



		for(i=0;i<varNameLen.i;i++)
			*ptr++=varName[i];
		for(i=0;i<varValueLen.i;i++)
			*ptr++=varValue[i];
		if(*(++sptr)=='\0')
			break;
	}
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
	tHeader.reserved = 0;
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
		if(!_stricmp(path,fCGIservers[i].path))
			return i;
	}
	return -1;
}
int FcgiConnect(fCGIContext* con,char* path)
{
	int pID;
	unsigned long pLong = 1L;
	if((pID=runFcgiServer(con,path))>=0)
	{
		MYSERVER_HOSTENT *hp=MYSERVER_SOCKET::ms_gethostbyname("localhost");

		struct sockaddr_in sockAddr;
		int sockLen = sizeof(sockAddr);
        memset(&sockAddr, 0, sizeof(sockAddr));
        sockAddr.sin_family = AF_INET;
	    memcpy(&sockAddr.sin_addr, hp->h_addr, hp->h_length);
	    sockAddr.sin_port = htons(fCGIservers[pID].port);
		
		if(con->sock.ms_socket(AF_INET, SOCK_STREAM, 0) == -1)
		{
			return -1;
		}
		if(con->sock.ms_connect((MYSERVER_SOCKADDR*)&sockAddr, sockLen) == -1)
		{
			con->sock.ms_closesocket();
			return -1;
		}
		con->sock.ms_ioctlsocket(FIONBIO, &pLong);
		con->fcgiPID=pID;
	}
	return pID;
}
int runFcgiServer(fCGIContext *con,char* path)
{
	int pID=isFcgiServerRunning(path);
	if(pID>=0)
		return pID;
	if(fCGIserversN==MAX_FCGI_SERVERS-2)
		return -1;
	static u_short port=3333;
	{
		/*SERVER SOCKET CREATION*/
		memset(&fCGIservers[fCGIserversN],0,sizeof(fCGIservers[fCGIserversN]));
		fCGIservers[fCGIserversN].socket.ms_socket(AF_INET,SOCK_STREAM,0);
		if(fCGIservers[fCGIserversN].socket.ms_getHandle()==INVALID_SOCKET)
			return -2;
		MYSERVER_SOCKADDRIN sock_inserverSocket;
		sock_inserverSocket.sin_family=AF_INET;
		sock_inserverSocket.sin_addr.s_addr=htonl(INADDR_ANY);
		fCGIservers[fCGIserversN].port=port+fCGIserversN;
		sock_inserverSocket.sin_port=htons(fCGIservers[fCGIserversN].port);
		if(fCGIservers[fCGIserversN].socket.ms_bind((sockaddr*)&sock_inserverSocket,sizeof(sock_inserverSocket)))
		{
			fCGIservers[fCGIserversN].socket.ms_closesocket();
			return -2;
		}
		if(fCGIservers[fCGIserversN].socket.ms_listen(SOMAXCONN))
		{
			fCGIservers[fCGIserversN].socket.ms_closesocket();
			return -2;
		}
#ifdef WIN32
		fCGIservers[fCGIserversN].DESCRIPTOR.fileHandle=fCGIservers[fCGIserversN].socket.ms_getHandle();
#endif
	}
	START_PROC_INFO spi;
	memset(&spi,0,sizeof(spi));
	char cmd[MAX_PATH];
	spi.cmd=cmd;
	spi.stdIn = (MYSERVER_FILE_HANDLE)fCGIservers[fCGIserversN].DESCRIPTOR.fileHandle;
	spi.cmdLine=cmd;
	spi.arg = NULL; /* no argument so clear it */

	sprintf(spi.cmd,"%s%s",con->td->cgiRoot,con->td->cgiFile);
	strcpy(fCGIservers[fCGIserversN].path,spi.cmd);
	spi.stdOut = spi.stdError =(MYSERVER_FILE_HANDLE) -1;
	
	fCGIservers[fCGIserversN].pid=execConcurrentProcess(&spi);
    
	if(!fCGIservers[fCGIserversN].pid)
		return -3;

	return fCGIserversN++;
}
