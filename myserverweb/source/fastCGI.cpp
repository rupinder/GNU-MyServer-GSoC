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
 
/*
*To get more info about the FastCGI protocol please visit the official FastCGI site
*at: http://www.fastcgi.com, here you can find samples and all the languages supported
*/
#include "../include/fastCGI.h"
#define MAX_FCGI_SERVERS	25
/*
*Thi strucure is used to keep trace of a running fCGI server.
*/
static struct sfCGIservers
{
	char path[MAX_PATH*2];/*server executable path*/
	union 
	{
	    unsigned long fileHandle;
		SOCKET sock;
		unsigned int value;
	}DESCRIPTOR;
	MYSERVER_SOCKET socket;
	char host[128];
	int pid; /*process ID*/ 
	u_short port;/*IP port*/
}fCGIservers[MAX_FCGI_SERVERS];
static int fCGIserversN;/*Number of thread currently loaded*/
struct fourchar
{	
	union
	{
		unsigned int i;
		unsigned char c[4];
	};
};
/*
*Entry-Point to manage a FastCGI request.
*/
int sendFASTCGI(httpThreadContext* td,LPCONNECTION connection,char* scriptpath,char* /*ext*/,char *cgipath,int execute)
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
	char fullpath[MAX_PATH*2];
	if(execute)
	{
		if(cgipath[0])
			sprintf(fullpath,"%s \"%s\"",cgipath,td->filenamePath);
		else
			sprintf(fullpath,"%s",td->filenamePath);	
	}
	else
	{
		sprintf(fullpath,"%s",cgipath);
	}
	int sizeEnvString=buildFASTCGIEnvironmentString(td,td->buffer,td->buffer2);
	td->inputData.closeFile();
	td->inputData.openFile(td->inputDataPath,MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_IFEXISTS|MYSERVER_FILE_NO_INHERIT);

	int pID = FcgiConnect(&con,fullpath);
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
		con.sock.closesocket();
		return raiseHTTPError(td,connection,e_501);
	}

	if(sendFcgiBody(&con,td->buffer2,sizeEnvString,FCGI_PARAMS,id))
	{
		con.sock.closesocket();
		return raiseHTTPError(td,connection,e_501);
	}

	if(sendFcgiBody(&con,0,0,FCGI_PARAMS,id))
	{
		con.sock.closesocket();
		return raiseHTTPError(td,connection,e_501);
	}	
    if(atoi(td->request.CONTENT_LENGTH))
	{
		generateFcgiHeader( tHeader, FCGI_STDIN, id, atoi(td->request.CONTENT_LENGTH));
		con.sock.send((char*)&tHeader,sizeof(tHeader),0);
		td->inputData.setFilePointer(0);
		do
		{
			td->inputData.readFromFile(td->buffer,td->buffersize,&nbr);
			if(nbr)
				con.sock.send(td->buffer,nbr,0);
		}while(nbr==td->buffersize);
	}
	if(sendFcgiBody(&con,0,0,FCGI_STDIN,id))
	{
		con.sock.closesocket();
		return raiseHTTPError(td,connection,e_501);
	}	

	/*Now read the output*/
	int exit=0;
	const clock_t timeout= CLOCKS_PER_SEC * 20; // 20 seconds
	clock_t time1 = clock();
	td->outputData.closeFile();	
	td->outputData.openFile(td->inputDataPath,MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_CREATE_ALWAYS|MYSERVER_FILE_NO_INHERIT);
	do	
	{
		while(con.sock.bytesToRead()<sizeof(FCGI_Header))
		{
			if(clock()-time1>timeout)
				break;
		}
		if(con.sock.bytesToRead())
			nbr=con.sock.recv((char*)&tHeader,sizeof(FCGI_Header),0);
		else
		{
			sendFcgiBody(&con,0,0,FCGI_ABORT_REQUEST,id);
			con.sock.shutdown(2);
			con.sock.closesocket();
			break;
		}
		/*contentLengthB1 is the high word of the content length value
		*while contentLengthB0 is the low one.
		*To retrieve the value of content length push left contentLengthB1
		*of eight byte then do a or with contentLengthB0.
		*/
		int dim=(tHeader.contentLengthB1<<8) | tHeader.contentLengthB0;
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
					con.sock.closesocket();
					raiseHTTPError(td,connection,e_501);
					exit = 1;
					break;
				case FCGI_STDOUT:
					nbr=con.sock.recv(td->buffer,min(dim,td->buffersize),0);
					u_long nbw;
					td->outputData.writeToFile(td->buffer,nbr,&nbw);
					dataSent=nbw;
					if(dataSent==0)
					{
						exit = 1;
						break;
					}
					while(dataSent<dim)
					{
						if( con.sock.bytesToRead() )
							nbr=con.sock.recv(td->buffer,min(dim-dataSent,td->buffersize),0);
						else
						{
							exit = 1;
							break;
						}
						td->outputData.writeToFile((char*)(td->buffer),nbr,&nbw);
						dataSent+=nbw;
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
	int headerSize=0;
	td->outputData.setFilePointer(0);
	td->buffer[0]='\0';
	td->outputData.readFromFile(td->buffer,KB(5),&nbr);
	for(u_long i=0;i<nbr;i++)
	{
		if((td->buffer[i]=='\r')&&(td->buffer[i+1]=='\n')&&(td->buffer[i+2]=='\r')&&(td->buffer[i+3]=='\n'))
		{
			headerSize=i+4;
			break;
		}
	}
	sprintf(td->response.CONTENT_LENGTH,"%u",td->outputData.getFileSize()-headerSize);
	buildHTTPResponseHeaderStruct(&td->response,td,td->buffer);
	for(;;)
	{
		if(td->response.LOCATION[0])
		{
			char nURL[MAX_PATH];
			nURL[0]='\0';
			u_long j;
			j=0;
			int start=(int)strlen(nURL);
			while(td->response.LOCATION[j])
			{
				nURL[j+start]=td->response.LOCATION[j];
				nURL[j+start+1]='\0';
				j++;
			}
			sendHTTPRedirect(td,connection,nURL);
			break;
		}
		buildHTTPResponseHeader(td->buffer2,&td->response);
		if(td->connection->socket.send(td->buffer2,strlen(td->buffer2), 0)==0)
		{
			exit = 1;
			break;
		}
		if(td->connection->socket.send((char*)(td->buffer+headerSize),nbr-headerSize, 0)==0)
		{
			exit = 1;
			break;
		}
		do
		{
			td->outputData.readFromFile(td->buffer,td->buffersize,&nbr);
			if(td->connection->socket.send((char*)td->buffer,nbr, 0)==0)
			{
				break;
			}
		}while(nbr);
		break;
	}
	td->outputData.closeFile();
	MYSERVER_FILE::deleteFile(td->inputDataPath);
	con.sock.closesocket();
	return 1;
}
/*
*Send the buffer content over the FastCGI connection
*/
int sendFcgiBody(fCGIContext* con,char* buffer,int len,int type,int id)
{
	FCGI_Header tHeader;
	generateFcgiHeader( tHeader, type, id, len );
	
	if(con->sock.send((char*)&tHeader,sizeof(tHeader),0)==-1)
		return -1;
	if(con->sock.send(buffer,len,0)==-1)
		return -1;
	return 0;
}
/*
*Trasform from a standard environment string to the FastCGI environment string.
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
/*
*Fill the FCGI_Header structure
*/
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
/*
*Initialize the FastCGI protocol implementation
*/
int initializeFASTCGI()
{
	fCGIserversN=0;
	memset(&fCGIservers,0,sizeof(fCGIservers));
	return 1;
}
/*
*Clean the memory and the processes occuped by the FastCGI servers
*/
int cleanFASTCGI()
{
	for(int i=0;i<fCGIserversN;i++)
	{
		if(fCGIservers[i].path[0]=='@')/*If the server is a remote one do nothing*/
			continue;
		fCGIservers[i].socket.closesocket();
		terminateProcess(fCGIservers[i].pid);
	}
	return 1;
}
/*
*Return the position in the array of the server indicated by path.
*A negative value is returned when the server is not running.
*/
int isFcgiServerRunning(char* path)
{
	for(int i=0;i<fCGIserversN;i++)
	{
		if(!lstrcmpi(path,fCGIservers[i].path))
			return i;
	}
	return -1;
}
/*
*Get a client socket in the fCGI context structure
*/
int FcgiConnectSocket(fCGIContext* con,int pID)
{
	unsigned long pLong = 1L;
	MYSERVER_HOSTENT *hp=MYSERVER_SOCKET::gethostbyname(fCGIservers[pID].host);

	struct sockaddr_in sockAddr;
	int sockLen = sizeof(sockAddr);
    memset(&sockAddr, 0, sizeof(sockAddr));
    sockAddr.sin_family = AF_INET;
	memcpy(&sockAddr.sin_addr, hp->h_addr, hp->h_length);
	sockAddr.sin_port = htons(fCGIservers[pID].port);
	
	if(con->sock.socket(AF_INET, SOCK_STREAM, 0) == -1)/*Try to create the socket*/
	{
		return -1;
	}
	if(con->sock.connect((MYSERVER_SOCKADDR*)&sockAddr, sockLen) == -1)/*If the socket was created try to connect*/
	{
		con->sock.closesocket();
		return -1;
	}
	con->sock.ioctlsocket(FIONBIO, &pLong);
	con->fcgiPID=pID;

}
/*
*Get a connection to the FastCGI server.
*/
int FcgiConnect(fCGIContext* con,char* path)
{
	int pID;
	pID=runFcgiServer(con,path);
	/*
	*If we find a valid server try the connection to it.
	*/
	if(pID>=0)
	{
		/*
		*Connect to the FastCGI server.
		*/
		int ret=FcgiConnectSocket(con,pID);
		if(ret==-1)
			return -1;
	}
	return pID;
}
int runFcgiServer(fCGIContext *con,char* path)
{
	int localServer;/*Flag to identify a local server(running on localhost) from a remore one*/
	localServer=path[0]!='@';/*Path that init with @ are not local path*/
	int pID=isFcgiServerRunning(path);/*Get the server position in the array*/
	if(pID>=0)/*If the process was yet initialized return its position*/
		return pID;
	if(fCGIserversN==MAX_FCGI_SERVERS-2)
		return -1;
	static u_short port=3333;
	{
		/*Create the server socket*/
		if(localServer)
		{/*Initialize the local server*/
			strcpy(fCGIservers[fCGIserversN].host,"localhost");
			fCGIservers[fCGIserversN].port=port++;
			fCGIservers[fCGIserversN].socket.socket(AF_INET,SOCK_STREAM,0);
			if(fCGIservers[fCGIserversN].socket.getHandle()==INVALID_SOCKET)
				return -2;
			MYSERVER_SOCKADDRIN sock_inserverSocket;
			sock_inserverSocket.sin_family=AF_INET;
			sock_inserverSocket.sin_addr.s_addr=htonl(INADDR_ANY);
			sock_inserverSocket.sin_port=htons(fCGIservers[fCGIserversN].port);
			if(fCGIservers[fCGIserversN].socket.bind((sockaddr*)&sock_inserverSocket,sizeof(sock_inserverSocket)))
			{
				fCGIservers[fCGIserversN].socket.closesocket();
				return -2;
			}
			if(fCGIservers[fCGIserversN].socket.listen(SOMAXCONN))
			{
				fCGIservers[fCGIserversN].socket.closesocket();
				return -2;
			}
			fCGIservers[fCGIserversN].DESCRIPTOR.fileHandle=fCGIservers[fCGIserversN].socket.getHandle();
			START_PROC_INFO spi;
			spi.cwd=0;
			spi.envString=0; 
			spi.cmd=path;
			spi.stdIn = (MYSERVER_FILE_HANDLE)fCGIservers[fCGIserversN].DESCRIPTOR.fileHandle;
			spi.cmdLine=path;
			spi.arg = NULL; /* no argument so clear it */

			strcpy(fCGIservers[fCGIserversN].path,spi.cmd);
			spi.stdOut = spi.stdError =(MYSERVER_FILE_HANDLE) -1;

			fCGIservers[fCGIserversN].pid=execConcurrentProcess(&spi);

			if(!fCGIservers[fCGIserversN].pid)
			{
				fCGIservers[fCGIserversN].socket.closesocket();
				return -3;
			}
		}/*End local server initialization*/
		else
		{/*Fill the structure with a remote server*/
			strcpy(fCGIservers[fCGIserversN].path,path);
			int i=1;/*Do not consider the @ character*/
			memset(fCGIservers[fCGIserversN].host,0,128);
			/*
			*A remote server path has the form @hosttoconnect:porttouse
			*/
			while(path[i]!=':')
				fCGIservers[fCGIserversN].host[i-1]=path[i++];
			fCGIservers[fCGIserversN].port=atoi(&path[++i]);
		}
		
	}

	return fCGIserversN++;/*If we arrive here increase the number of running servers*/
}
