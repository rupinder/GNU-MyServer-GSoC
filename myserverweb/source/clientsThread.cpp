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



#include "..\stdafx.h"
#include "..\include\clientsTHREAD.h"
#include "..\include\cserver.h"

ClientsTHREAD::ClientsTHREAD()
{
	ZeroMemory(&request,sizeof(request));
	ZeroMemory(&response,sizeof(response));
	hImpersonation=0;
	err=0;
}
ClientsTHREAD::~ClientsTHREAD()
{
	clean();
}
BOOL ClientsTHREAD::sendDIRECTORY(LPCONNECTION s,char* folder)
{
	static char filename[MAX_PATH];
	static DWORD startChar=lstrlen(lserver->path)+1;
	
	if(getPathRecursionLevel(folder)<1)
	{
		raiseError(s,e_401);
		return 1;
	}
	ZeroMemory(buffer2,200);
	static HANDLE  ff;
	static WIN32_FIND_DATA fd;
	sprintf(filename,"%s/*.*",folder);
	sprintf(buffer2,"%s",lserver->msgFolderContents);
	lstrcat(buffer2," ");
	lstrcat(buffer2,&folder[startChar]);
	lstrcat(buffer2,"\\<P>\n<HR>");
	ff=FindFirstFile(filename,&fd);
	if(ff==INVALID_HANDLE_VALUE)
	{
		if(GetLastError()==ERROR_ACCESS_DENIED)
		{
			/*
			*If client have tried to post a login
			*and a password more times send error 
			*/
			if(s->nTries > 2)
			{
				raiseError(s,e_401);
			}
			else
			{
				s->nTries++;
				raiseError(s,e_401AUTH);
			}
		}
		else
		{
			raiseError(s,e_404);
		}
		return 1;
	}
	lstrcat(buffer2,"<TABLE  width=\"100%\"><TR><TD>File</TD><TD>Last modify</TD><TD>Dimension</TD></TR>");
	static char fileSize[10];
	static char fileTime[20];
	static SYSTEMTIME st;
	do
	{
		if(fd.cFileName[0]=='.')
			continue;
		lstrcat(buffer2,"<TR><TD><A HREF=");
		lstrcat(buffer2,&folder[startChar]);
		lstrcat(buffer2,"\\");
		lstrcat(buffer2,fd.cFileName);
		lstrcat(buffer2,">");
		lstrcat(buffer2,fd.cFileName);
		lstrcat(buffer2,"</TD><TD>");
		FileTimeToSystemTime(&fd.ftLastWriteTime,&st);
		sprintf(fileTime,"%u\\%u\\%u-%u:%u:%u System time",st.wDay,st.wMonth,st.wYear,st.wHour,st.wMinute,st.wSecond);
		lstrcat(buffer2,fileTime);

		lstrcat(buffer2,"</TD><TD>");
		if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			lstrcat(buffer2,"<dir>");
		}
		else
		{
			sprintf(fileSize,"%i bytes",fd.nFileSizeLow);
			lstrcat(buffer2,fileSize);
		}
		lstrcat(buffer2,"</TD></TR>\n");
	}while(FindNextFile(ff,&fd));
	lstrcat(buffer2,"</TABLE>\n<HR>");
	lstrcat(buffer2,lserver->msgRunOn);
	lstrcat(buffer2," myServer ");
	LoadString(lserver->hInst ,IDS_VERSIONAPP,&buffer2[lstrlen(buffer2)],lstrlen(buffer2));
	FindClose(ff);
	buildDefaultHttpResponseHeader(&response);	
	sprintf(response.CONTENTS_DIM,"%u",lstrlen(buffer2));
	buildHttpResponseHeader(buffer,&response);
	send(s->socket,buffer,lstrlen(buffer), 0);
	send(s->socket,buffer2,lstrlen(buffer2), 0);

	return 1;

}
BOOL ClientsTHREAD::sendFILE(LPCONNECTION s,char *filenamePath,BOOL OnlyHeader,int firstByte,int lastByte)
{
	static FILE* h;
	h=0;
	h=fopen(filenamePath,"rb");
	if(h==0)
	{	
		if(GetLastError()==ERROR_ACCESS_DENIED)
		{
			if(s->nTries > 2)
			{
				raiseError(s,e_401);
			}
			else
			{
				s->nTries++;
				raiseError(s,e_401AUTH);
			}
			return 1;
		}
		else
		{
			return 0;
		}
	}
	/*
	*If h!=0
	*/

	static DWORD filesize;
	getFileSize(&filesize,h);
	if(lastByte != -1)
	{
		lastByte=min((DWORD)lastByte,filesize);
		filesize-=(filesize-firstByte);
	}
	if(firstByte)
		filesize-=firstByte;
	if(fseek(h, firstByte, SEEK_SET))
	{
		raiseError(s,e_500);
		return 1;
	}


	ZeroMemory(buffer,300);

	sprintf(response.CONTENTS_DIM,"%u",filesize);
	buildHttpResponseHeader(buffer,&response);
	send(s->socket,buffer,lstrlen(buffer), 0);

	/*
	*If is requested only the header; HEAD request
	*/
	if(OnlyHeader)
		return 1;

	static DWORD bytesToSend;
	static DWORD bytesSent;
	bytesToSend=filesize;
	if(lserver->verbosity>2)
		fprintf(lserver->logFile,"%s %s\n",lserver->msgSending,filenamePath);
	for(;;)
	{
		fread(buffer,buffersize,1,h);
		bytesSent=send(s->socket,buffer,min(bytesToSend,buffersize), 0);
		if(bytesSent==SOCKET_ERROR)
		{
			break;
		}
		bytesToSend-=bytesSent;
		if(bytesToSend==0)
			break;
	}
	fclose(h);
	return 1;

}
BOOL ClientsTHREAD::sendCGI(LPCONNECTION s,char* filename,char* ext,char *exec)
{
	ext;
	static HMODULE hinstLib; 
    static CGIMAIN ProcMain;
	static CGIINIT ProcInit;
 
    hinstLib = LoadLibrary(exec); 
	buffer2[0]='\0';
	if (hinstLib) 
    { 
		ProcInit = (CGIINIT) GetProcAddress(hinstLib, "initialize");
		ProcMain = (CGIMAIN) GetProcAddress(hinstLib, "main"); 
		if(ProcInit)
			(ProcInit)((LPVOID)&buffer[0],(LPVOID)&buffer2[0],(LPVOID)&response,(LPVOID)&request);
		if(ProcMain)
			(ProcMain)(filename);
        FreeLibrary(hinstLib); 
    } 
	else
	{
		if(GetLastError()==ERROR_ACCESS_DENIED)
		{
			if(s->nTries > 2)
			{
				raiseError(s,e_403);
			}
			else
			{
				s->nTries++;
				raiseError(s,e_401AUTH);
			}
		}
		else
		{
			raiseError(s,e_404);
		}
		return 1;
	}
	static int len;
	len=lstrlen(buffer2);
	sprintf(response.CONTENTS_DIM,"%u",len);
	buildHttpResponseHeader(buffer,&response);
	send(s->socket,buffer,lstrlen(buffer), 0);
	send(s->socket,buffer2,len, 0);
	return 1;
}
BOOL ClientsTHREAD::sendRESOURCE(LPCONNECTION s,char *filename,BOOL systemrequest,BOOL OnlyHeader,int firstByte,int lastByte)
{
	buffer[0]='\0';
	buildDefaultHttpResponseHeader(&response);

	static char ext[MAX_PATH];
	static char data[MAX_PATH];
	/*
	*getMIME return TRUE if the ext is registered by a CGI
	*/
	if(getMIME(response.MIME,filename,ext,data))
	{
		if(sendCGI(s,filenamePath,ext,data))
			return 1;
	}
	getPath(filenamePath,filename,systemrequest);
	if(lstrlen(ext)==0)
	{
		static char defaultFilenamePath[MAX_PATH];
		getPath(defaultFilenamePath,filename,systemrequest);
		lstrcat(defaultFilenamePath,lserver->defaultFilename);
		if(sendFILE(s,defaultFilenamePath,OnlyHeader,firstByte,lastByte))
				return 1;

		if(sendDIRECTORY(s,filenamePath))
			return 1;	

		raiseError(s,e_404);
		return 1;
	}
	/*
	*myServer CGI format
	*/
	if(!lstrcmpi(ext,"mscgi"))
	{
		char *target;
		if(request.URIOPTSPTR)
			target=request.URIOPTSPTR;
		else
			target=(char*)&request.URIOPTS;
		if(sendMSCGI(s,filenamePath,target))
			return 1;
		raiseError(s,e_404);
		return 1;
	}

	if(sendFILE(s,filenamePath,OnlyHeader,firstByte,lastByte))
		return 1;
	raiseError(s,e_404);
	return 1;
}
BOOL ClientsTHREAD::sendMSCGI(LPCONNECTION s,char* exec,char* cmdLine)
{
	static HMODULE hinstLib; 
    static CGIMAIN ProcMain;
	static CGIINIT ProcInit;
 
    hinstLib = LoadLibrary(exec); 
	buffer2[0]='\0';
	if (hinstLib) 
    { 
		ProcInit = (CGIINIT) GetProcAddress(hinstLib, "initialize");
		ProcMain = (CGIMAIN) GetProcAddress(hinstLib, "main"); 
		if(ProcInit && ProcMain)
		{
			(ProcInit)((LPVOID)&buffer[0],(LPVOID)&buffer2[0],(LPVOID)&response,(LPVOID)&request);
			(ProcMain)(cmdLine);
		}
        FreeLibrary(hinstLib); 
    } 
	else
	{
		if(GetLastError()==ERROR_ACCESS_DENIED)
		{
			if(s->nTries > 2)
			{
				raiseError(s,e_403);
			}
			else
			{
				s->nTries++;
				raiseError(s,e_401AUTH);
			}
		}
		else
		{
			raiseError(s,e_404);
		}
		return 1;
	}
	static int len;
	len=lstrlen(buffer2);
	sprintf(response.CONTENTS_DIM,"%u",len);
	buildHttpResponseHeader(buffer,&response);
	send(s->socket,buffer,lstrlen(buffer), 0);
	send(s->socket,buffer2,len, 0);
	return 1;

}
void ClientsTHREAD::getPath(char *filenamePath,char *filename,BOOL systemrequest)
{
	if(systemrequest)
	{
		sprintf(filenamePath,"%s\\%s",lserver->systemPath,filename);
	}
	else
	{
		sprintf(filenamePath,"%s\\%s",lserver->path,filename);
	}
}
unsigned int WINAPI startClientsTHREAD(void* pParam)
{
	DWORD id=*((DWORD*)pParam);
	ClientsTHREAD *ct=&lserver->threads[id];
	ct->threadIsRunning=TRUE;
	ct->connections=0;
	ct->buffersize=lserver->buffersize;
	ct->buffersize2=lserver->buffersize2;
	ct->buffer=(char*)malloc(ct->buffersize);
	ct->buffer2=(char*)malloc(ct->buffersize2);
	ct->initialized=TRUE;
	ZeroMemory(ct->buffer,ct->buffersize);
	ZeroMemory(ct->buffer2,ct->buffersize2);
	char mutexname[20];
	sprintf(mutexname,"connectionMutex_%u",id);
	ct->connectionMutex=CreateMutex(NULL,FALSE,mutexname);
	while(ct->threadIsRunning)
	{
		ct->controlConnections();
	}
	_endthreadex( 0 );
	return 0;
}

void ClientsTHREAD::controlConnections()
{
	WaitForSingleObject(connectionMutex,INFINITE);
	LPCONNECTION c=connections;
	BOOL logon;
	for(c; c ;c=c->Next)
	{
		ioctlsocket(c->socket,FIONREAD,&nBytesToRead);
		if(nBytesToRead)
		{
			if(lserver->useLogonOption)
			{
				if(c->login[0])
				{
					logon=LogonUser(c->login,NULL,c->password, LOGON32_LOGON_NETWORK, LOGON32_PROVIDER_DEFAULT, &hImpersonation);
				}
				else
				{
					logon=FALSE;
					hImpersonation=lserver->guestLoginHandle;
				}
				ImpersonateLoggedOnUser(hImpersonation);
			}
			else
			{
				logon=FALSE;
			}
			err=recv(c->socket,buffer,buffersize, 0);

			if((err==0) || (err==SOCKET_ERROR)||(err==WSAECONNABORTED)||(err==WSAENOTCONN))
			{
				raiseError(c,e_500);
				if(deleteConnection(c))
					continue;
			}
			if(err!=WSAETIMEDOUT)
			{
				switch(c->protocol)
				{
					case PROTOCOL_HTTP:
						if(!controlHTTPConnection(c))
							continue;
				}
			}
			c->timeout=getTime();
			if(lserver->useLogonOption)
			{
				RevertToSelf();
				if(logon)
				{
					CloseHandle(hImpersonation);
				}
			}
		}
		else
		{
			if( getTime()- c->timeout > lserver->connectionTimeout)
				if(deleteConnection(c))
					continue;
		}
	}
	ReleaseMutex(connectionMutex);
}
void ClientsTHREAD::stop()
{
	/*
	*Set the run flag to False
	*When the current thread find the threadIsRunning
	*flag setted to FALSE automatically destroy the
	*thread
	*/
	threadIsRunning=FALSE;
}
void ClientsTHREAD::clean()
{
	/*
	*Clean the memory used by the thread
	*/
	if(initialized==FALSE)
		return;
	if(connectionMutex)
		WaitForSingleObject(connectionMutex,INFINITE);
	if(connections)
	{
		clearAllConnections();
	}
	free(buffer);
	free(buffer2);
	initialized=FALSE;
	ReleaseMutex(connectionMutex);
	TerminateThread(threadHandle,0);
}
BOOL ClientsTHREAD::controlHTTPConnection(LPCONNECTION a)
{
	/*
	*In this function there is the HTTP protocol parse.
	*The REQUEST is mapped into a HTTP_REQUEST_HEADER structure
	*And at the end of this every command is treated
	*differently. We use this mode for parse the HTTP
	*because especially in the CGI is requested a continue
	*HTTP header read.
	*Before of mapping the header in the structure control
	*if this is a regular request.
	*The HTTP header is ended by a \r\n\r\n sequence.
	*/
	
	/*
	*Begin control of the HTTP header
	*/
	static DWORD i,j,max;
	static DWORD nLineChars;
	static BOOL isValidCommand,containOpts;
	isValidCommand=FALSE;
	nLineChars=0;
	buffer=buffer;
	static DWORD nLines;
	static DWORD maxTotChars=min(2048,buffersize-3);
		
	for(nLines=i=0;i<min(1024,buffersize-5);i++)
	{
		if(buffer[i]=='\n')
		{
			if(buffer[i+2]=='\n')
			{
				maxTotChars=i+3;
				isValidCommand=TRUE;
				break;
			}
			nLines++;
		}
		else
			nLineChars++;
		if(nLineChars>1024)
			break;
	}
	if(nLines<(DWORD)(nLineChars/100))

	isValidCommand=FALSE;
	if(lserver->verbosity>4)
	{
		if(lserver->verbosity>5)
			buffer[this->nBytesToRead]='\0';
		else
			buffer[maxTotChars]='\0';

	fprintf(lserver->logFile,"%s\n",buffer);
	}
	if(isValidCommand==FALSE)
	{
		raiseError(a,e_400);
		deleteConnection(a);
		return 0;
	}

	/*
	*End control of the HTTP header
	*/

	const int max_URI=MAX_PATH+200;
	const char seps[]   = " ,\t\n\r";
	const char cmdseps[]   = ": ,\t\n\r";

	static char *token=0;
	static char command[96];
	ZeroMemory(&request,sizeof(request));

	static int nLineControlled;
	nLineControlled=0;
	const int maxLineToControl=25;
	static BOOL lineControlled;
	token=buffer;

	token = strtok( token, cmdseps );
	controlAnotherLine:
	lineControlled=FALSE;
	lstrcpy(command,token);
	
	
	nLineControlled++;
	if(nLineControlled>maxLineToControl)
	{
		raiseError(a,e_413);
		deleteConnection(a);
		return 0;
	}

	if(nLineControlled==1)
	{
		/*
		Version of the protocol in the HTTP_REQUEST_HEADER
		*struct is leaved as a number.
		*For example HTTP/1.1 in the struct is 1.1
		*/


		/*GET*/
		if(!lstrcmpi(command,"GET"))
		{
			lineControlled=TRUE;
			lstrcpy(request.CMD,"GET");
		
			token = strtok( NULL, " ,\t\n\r" );
			max=lstrlen(token);
			containOpts=FALSE;
			for(i=0;i<max;i++)
			{
				if(token[i]=='?')
				{
					containOpts=TRUE;
					break;
				}
				request.URI[i]=token[i];				
			}
			request.URI[i]='\0';

			if(containOpts)
			{
				for(j=0;i<max;j++)
				{
					request.URIOPTS[j]=token[++i];
				}
			}
			token = strtok( NULL, seps );
			lstrcpy(request.VER,token);
			StrTrim(request.VER,"HTTP /");
			StrTrim(request.URI," /");
			StrTrim(request.URIOPTS," /");
			if(lstrlen(request.URI)>max_URI)
			{
				raiseError(a,e_414);
				deleteConnection(a);
				return 0;
			}
			else
			{
				max=lstrlen(request.URI);
				for(i=0;i<max;i++)
				{
					if(request.URI[i]=='/')
						request.URI[i]='\\';
				}
			}
			request.URIOPTSPTR=0;
		}
		/*POST*/
		if(!lstrcmpi(command,"POST"))
		{
			lineControlled=TRUE;
			lstrcpy(request.CMD,"POST");
		
			token = strtok( NULL, " ,\t\n\r" );
			max=lstrlen(token);
			containOpts=FALSE;
			for(i=0;i<max;i++)
			{

				if(token[i]=='?')
				{
					containOpts=TRUE;
					break;
				}
				request.URI[i]=token[i];				
			}
			request.URI[i]='\0';
			if(containOpts)
			{
				for(j=0;i<max;j++)
				{
					request.URIOPTS[j]=token[++i];
				}
			}
			request.URIOPTS[0]='\0';


			request.URIOPTSPTR=&buffer[maxTotChars];
			buffer[max(nBytesToRead,buffersize)]='\0';

			token = strtok( NULL, seps );
			lstrcpy(request.VER,token);
			StrTrim(request.VER,"HTTP /");
			StrTrim(request.URI," /");
			if(lstrlen(request.URI)>max_URI)
			{
				raiseError(a,e_414);
				deleteConnection(a);
				return 0;
			}
			else
			{
				max=lstrlen(request.URI);
				for(i=0;i<max;i++)
				{
					if(request.URI[i]=='/')
						request.URI[i]='\\';
				}
			}
		}
		/*HEAD*/
		if(!lstrcmpi(command,"HEAD"))
		{
			lineControlled=TRUE;
			lstrcpy(request.CMD,"HEAD");
		
			token = strtok( NULL, seps );
			lstrcpy(request.URI,token);
			token = strtok( NULL, seps );
			lstrcpy(request.VER,token);

			StrTrim(request.URI," /");
			if(lstrlen(request.URI)>max_URI)
			{
				raiseError(a,e_414);
				deleteConnection(a);
				return 0;
			}
		}

		if(!lstrcmpi(command,""))
		{
			raiseError(a,e_501);
			deleteConnection(a);
			return 0;
		}
	}
	/*User-Agent*/
	if(!lstrcmpi(command,"User-Agent"))
	{
		token = strtok( NULL, "\r\n" );
		lineControlled=TRUE;
		lstrcpy(request.USER_AGENT,token);
		StrTrim(request.USER_AGENT," ");
	}
	/*Authorization*/
	if(!lstrcmpi(command,"Authorization"))
	{
		token = strtok( NULL, "\r\n" );
		lineControlled=TRUE;		
		ZeroMemory(buffer2,300);
		/*
		*Basic authorization is
		*login:password in base64
		*Assume that it is Basic anyway
		*/
		int len=lstrlen(token);
		char *base64=base64Utils.Decode(&token[lstrlen("Basic: ")],&len);
		char* lbuffer2=base64;
		while(*lbuffer2!=':')
		{
			a->login[lstrlen(a->login)]=*lbuffer2++;
		}
		lbuffer2++;

		while(*lbuffer2)

		{

			a->password[lstrlen(a->password)]=*lbuffer2++;

		}

		free(base64);

	}

	/*Host*/

	if(!lstrcmpi(command,"Host"))

	{

		token = strtok( NULL, seps );

		lineControlled=TRUE;

		lstrcpy(request.HOST,token);

		StrTrim(request.HOST," ");

	}

	/*Accept*/

	if(!lstrcmpi(command,"Accept"))

	{

		token = strtok( NULL, "\n\r" );

		lineControlled=TRUE;

		lstrcat(request.ACCEPT,token);

		StrTrim(request.ACCEPT," ");

	}

	/*Accept-Language*/

	if(!lstrcmpi(command,"Accept-Language"))

	{

		token = strtok( NULL, "\n\r" );

		lineControlled=TRUE;

		lstrcpy(request.ACCEPTLAN,token);

		StrTrim(request.ACCEPTLAN," ");

	}

	/*Accept-Charset*/

	if(!lstrcmpi(command,"Accept-Charset"))

	{

		token = strtok( NULL, "\n\r" );

		lineControlled=TRUE;

		lstrcpy(request.ACCEPTCHARSET,token);

		StrTrim(request.ACCEPTCHARSET," ");

	}



	/*Accept-Encoding*/

	if(!lstrcmpi(command,"Accept-Encoding"))

	{

		token = strtok( NULL, "\n\r" );

		lineControlled=TRUE;

		lstrcpy(request.ACCEPTENC,token);

		StrTrim(request.ACCEPTENC," ");

	}

	/*Connection*/

	if(!lstrcmpi(command,"Connection"))

	{

		token = strtok( NULL, "\n\r" );

		lineControlled=TRUE;

		lstrcpy(request.CONNECTION,token);

		StrTrim(request.CONNECTION," ");

	}

	/*Range*/

	if(!lstrcmpi(command,"Range"))

	{

		ZeroMemory(request.RANGETYPE,30);

		ZeroMemory(request.RANGEBYTEBEGIN,30);

		ZeroMemory(request.RANGEBYTEEND,30);

		lineControlled=TRUE;

		token = strtok( NULL, "\r\n\t" );

		do

		{

			request.RANGETYPE[lstrlen(request.RANGETYPE)]=*token;

		}

		while(*token++ != '=');

		do

		{

			request.RANGEBYTEBEGIN[lstrlen(request.RANGEBYTEBEGIN)]=*token;

		}

		while(*token++ != '-');

		do

		{

			request.RANGEBYTEEND[lstrlen(request.RANGEBYTEEND)]=*token;

		}

		while(*token++);

		StrTrim(request.RANGETYPE,"= ");

		StrTrim(request.RANGEBYTEBEGIN,"- ");

		StrTrim(request.RANGEBYTEEND," ");

		

		if(lstrlen(request.RANGEBYTEBEGIN)==0)

			lstrcpy(request.RANGEBYTEBEGIN,"0");

		if(lstrlen(request.RANGEBYTEEND)==0)

			lstrcpy(request.RANGEBYTEEND,"-1");



	}

	/*Referer*/

	if(!lstrcmpi(command,"Referer"))

	{

		token = strtok( NULL, seps );

		lineControlled=TRUE;

		lstrcpy(request.REFERER,token);

		StrTrim(request.REFERER," ");

	}

	if(!lineControlled)

	{

		token = strtok( NULL, "\n" );

	}

	token = strtok( NULL, cmdseps );

	if((token-buffer)<maxTotChars)

		goto controlAnotherLine; 



	/*

	*END REQUEST STRUCTURE BUILD

	*/

	if(!lstrcmpi(request.CMD,"GET"))

	{

		if(lstrlen(request.HOST)==0)

		{

			raiseError(a,e_400);

			return 0;

		}

		if(!lstrcmpi(request.RANGETYPE,"bytes"))

			sendRESOURCE(a,request.URI,FALSE,FALSE,atoi(request.RANGEBYTEBEGIN),atoi(request.RANGEBYTEEND));

		else

			sendRESOURCE(a,request.URI);

	}else if(!lstrcmpi(request.CMD,"POST"))

	{

		if(lstrlen(request.HOST)==0)

		{

			raiseError(a,e_400);

			return 0;

		}

		if(!lstrcmpi(request.RANGETYPE,"bytes"))

			sendRESOURCE(a,request.URI,FALSE,FALSE,atoi(request.RANGEBYTEBEGIN),atoi(request.RANGEBYTEEND));

		else

			sendRESOURCE(a,request.URI);

	}else if(!lstrcmpi(request.CMD,"HEAD"))

	{

		if(lstrlen(request.HOST)==0)

		{

			raiseError(a,e_400);

			deleteConnection(a);

			return 0;

		}

		sendRESOURCE(a,request.URI,FALSE,TRUE);

	}

	else

	{

		raiseError(a,e_501);

		deleteConnection(a);

		return 0;

	}

	if(lstrcmpi(request.CONNECTION,"Keep-Alive"))

	{

		deleteConnection(a);

		return 0;

	}

	return 1;



}

LPCONNECTION ClientsTHREAD::findConnection(SOCKET a)

{

	/*

	*Find a connection passing the socket that control it

	*/

	LPCONNECTION c;

	for(c=connections;c;c=c->Next)

	{

		if(c->socket==a)

			return c;

	}

	return NULL;

}

LPCONNECTION ClientsTHREAD::addConnection(SOCKET s,CONNECTION_PROTOCOL protID)

{

	/*

	*Add a new connection.

	*Connections are defined using a CONNECTION struct.

	*/

	WaitForSingleObject(connectionMutex,INFINITE);

	const int maxRcvBuffer=KB(5);

	const BOOL keepAlive=TRUE;

	setsockopt(s,SOL_SOCKET,SO_RCVBUF,(char*)&maxRcvBuffer,sizeof(maxRcvBuffer));

	setsockopt( s,SOL_SOCKET, SO_SNDTIMEO,(char *)&lserver->socketRcvTimeout,sizeof(lserver->socketRcvTimeout));

	setsockopt( s,SOL_SOCKET, SO_KEEPALIVE,(char *)&keepAlive,sizeof(keepAlive));



	LPCONNECTION nc=(CONNECTION*)malloc(sizeof(CONNECTION));

	ZeroMemory(nc,sizeof(CONNECTION));

	nc->socket=s;

	nc->protocol=protID;

	nc->Next=connections;

	connections=nc;

	nConnections++;

	ReleaseMutex(connectionMutex);

	return nc;

}

BOOL ClientsTHREAD::deleteConnection(LPCONNECTION s)

{

	/*

	*Delete a connection

	*/

	WaitForSingleObject(connectionMutex,INFINITE);

	BOOL ret=FALSE;

	shutdown(s->socket,SD_BOTH );

	do

	{

		err=recv(s->socket,buffer,buffersize,0);

	}while(err && (err!=SOCKET_ERROR));

	closesocket(s->socket); 



	LPCONNECTION prev=0;

	for(LPCONNECTION i=connections;i;i=i->Next)

	{

		if(i->socket == s->socket)

		{

			if(prev)

				prev->Next=i->Next;

			else

				connections=i->Next;

			free(i);

			ret=TRUE;

			break;

		}

		prev=i;

	}

	nConnections--;

	ReleaseMutex(connectionMutex);

	return ret;

}

void ClientsTHREAD::clearAllConnections()

{

	WaitForSingleObject(connectionMutex,INFINITE);

	LPCONNECTION c=connections;

	for(;c;c=c->Next)

	{

		deleteConnection(c);

	}

	connections=NULL;

	nConnections=0;

	ReleaseMutex(connectionMutex);

}





void ClientsTHREAD::buildHttpResponseHeader(char *str,HTTP_RESPONSE_HEADER *response)

{

	/*

	*Here is builded the HEADER of a HTTP response.

	*Passing a HTTP_RESPONSE_HEADER struct this build

	*a header string

    */

	if(response->isError)

		sprintf(str,"HTTP/%s %s\r\nServer:%s\r\nContent-Type:%s\r\nContent-Length: %s\r\nStatus: \r\n",response->VER,response->ERROR_TYPE,response->SERVER_NAME,response->MIME,response->CONTENTS_DIM,response->ERROR_TYPE);

	else

		sprintf(str,"HTTP/%s 200 OK\r\nServer:%s\r\nContent-Type:%s\r\nContent-Length: %s\r\n",response->VER,response->SERVER_NAME,response->MIME,response->CONTENTS_DIM);



	if(lstrlen(response->DATE)>5)

	{

		lstrcat(str,"Date:");

		lstrcat(str,response->DATE);

		lstrcat(str,"\r\n");

	}

	if(lstrlen(response->DATEEXP)>5)

	{

		lstrcat(str,"Expires:");

		lstrcat(str,response->DATEEXP);

		lstrcat(str,"\r\n");

	}

	if(lstrlen(response->OTHER)>5)

	{

		lstrcat(str,response->OTHER);

		lstrcat(str,"\r\n");

	}

	lstrcat(str,"Accept-Ranges: bytes\r\n");

	lstrcat(str,"\n");



}

void ClientsTHREAD::buildDefaultHttpResponseHeader(HTTP_RESPONSE_HEADER* response)

{

	ZeroMemory(response,sizeof(HTTP_RESPONSE_HEADER));

	lstrcpy(response->MIME,"text/html");

	lstrcpy(response->VER,"1.1");

	lstrcpy(response->DATE,getHTTPFormattedTime());



	lstrcpy(response->DATEEXP,getHTTPFormattedTime());

	lstrcpy(response->SERVER_NAME,"MyServer");

}



BOOL ClientsTHREAD::getMIME(char *MIME,char *filename,char *dest,char *dest2)

{

	getFileExt(dest,filename);

	/*

	*Return true if file is registered by a CGI

	*/

	return lserver->mimeManager.getMIME(dest,MIME,dest2);

}

void ClientsTHREAD::raiseError(LPCONNECTION a,int ID)

{

	static HTTP_RESPONSE_HEADER response;

	if(ID==e_401AUTH)

	{

		sprintf(buffer2,"HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic\r\nServer: %s\r\nDate: %s\r\nContent-type: text/html\r\nContent-length: 0\r\n\r\n",lserver->serverName,getHTTPFormattedTime());

		send(a->socket,buffer2,lstrlen(buffer2),0);

		return;

	}

	if(lserver->useMessagesFiles)

	{

		sendRESOURCE(a,HTTP_ERROR_HTMLS[ID],TRUE);

		return;

	}

	buildDefaultHttpResponseHeader(&response);



	response.isError=TRUE;

	lstrcpy(response.ERROR_TYPE,HTTP_ERROR_MSGS[ID]);

	sprintf(response.CONTENTS_DIM,"%i",lstrlen(HTTP_ERROR_MSGS[ID]));

	buildHttpResponseHeader(buffer,&response);

	lstrcat(buffer,HTTP_ERROR_MSGS[ID]);

	send(a->socket,buffer,lstrlen(buffer), 0);

}



