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
#include "..\include\http.h"
#include "..\include\cserver.h"
#include "..\include\security.h"

char *buffer;
char *buffer2;
int buffersize;
int buffersize2;
DWORD nBytesToRead;
HTTP_RESPONSE_HEADER response;
HTTP_REQUEST_HEADER request;
char filenamePath[MAX_PATH];
HANDLE tmpBufferFile;
HANDLE *hImpersonation;
BOOL sendHTTPDIRECTORY(LPCONNECTION s,char* folder)
{
	static char filename[MAX_PATH];
	static DWORD startChar=lstrlen(lserver->getPath())+1;
	
	if(getPathRecursionLevel(folder)<1)
	{
		raiseHTTPError(s,e_401);
		return 1;
	}
	ZeroMemory(buffer2,200);
	static HANDLE  ff;
	static WIN32_FIND_DATA fd;
	sprintf(filename,"%s/*.*",folder);
	sprintf(buffer2,"%s",msgFolderContents);
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
				raiseHTTPError(s,e_401);
			}
			else
			{
				s->nTries++;
				raiseHTTPError(s,e_401AUTH);
			}
		}
		else
		{
			raiseHTTPError(s,e_404);
		}
		return 1;
	}
	sprintf(buffer2+lstrlen(buffer2),"<TABLE><TR><TD>%s</TD><TD>%s</TD><TD>%s</TD></TR>",msgFile,msgLModify,msgSize);
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
	lstrcat(buffer2,msgRunOn);
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
BOOL sendHTTPFILE(LPCONNECTION s,char *filenamePath,BOOL OnlyHeader,int firstByte,int lastByte)
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
				raiseHTTPError(s,e_401);
			}
			else
			{
				s->nTries++;
				raiseHTTPError(s,e_401AUTH);
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
		raiseHTTPError(s,e_500);
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
	if(lserver->getVerbosity()>2)
		fprintf(lserver->logFile,"%s %s\n",msgSending,filenamePath);
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
BOOL sendHTTPRESOURCE(LPCONNECTION s,char *filename,BOOL systemrequest,BOOL OnlyHeader,int firstByte,int lastByte)
{
	char *c=filename;
	while(*c)
	{
		if(*c=='\\')
			*c='/';
		c++;
	}
	buffer[0]='\0';
	buildDefaultHttpResponseHeader(&response);

	static char ext[MAX_PATH];
	static char data[MAX_PATH];
	/*
	*getMIME return TRUE if the ext is registered by a CGI
	*/
	if(getMIME(response.MIME,filename,ext,data))
	{
		if(sendCGI(s,filename,ext,data))
			return 1;
	}
	getPath(filenamePath,filename,systemrequest);
	if(lstrlen(ext)==0)
	{
		static char fileName[MAX_PATH];
		sprintf(fileName,"%s\\%s",filenamePath,lserver->getDefaultFilenamePath());
		if(sendHTTPFILE(s,fileName,OnlyHeader,firstByte,lastByte))
			return 1;

		if(sendHTTPDIRECTORY(s,filenamePath))
			return 1;	

		raiseHTTPError(s,e_404);
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
		raiseHTTPError(s,e_404);
		return 1;
	}

	if(sendHTTPFILE(s,filenamePath,OnlyHeader,firstByte,lastByte))
		return 1;
	raiseHTTPError(s,e_404);
	return 1;
}
BOOL sendMSCGI(LPCONNECTION s,char* exec,char* cmdLine)
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
				raiseHTTPError(s,e_403);
			}
			else
			{
				s->nTries++;
				raiseHTTPError(s,e_401AUTH);
			}
		}
		else
		{
			raiseHTTPError(s,e_404);
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
BOOL controlHTTPConnection(LPCONNECTION a,char *b1,char *b2,int bs1,int bs2,DWORD nbtr,HANDLE *imp)
{
	buffer=b1;
	buffer2=b2;
	buffersize=bs1;
	buffersize2=bs2;
	nBytesToRead=nbtr;
	hImpersonation=imp;
	/*
	*In this function there is the HTTP protocol parse.
	*The REQUEST is mapped into a HTTP_REQUEST_HEADER structure
	*And at the end of this every command is treated
	*differently. We use this mode for parse the HTTP
	*because especially in the CGI is requested a continue
	*HTTP header read.
	*Before of mapping the header in the structure control
	*if this is a regular request.
	*The HTTP header ends with a \r\n\r\n sequence.
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
	if(lserver->getVerbosity()>4)
	{
		if(lserver->getVerbosity()>5)
			buffer[nBytesToRead]='\0';
		else
			buffer[maxTotChars]='\0';

	fprintf(lserver->logFile,"%s\n",buffer);
	}
	if(isValidCommand==FALSE)
	{
		raiseHTTPError(a,e_400);
		
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
		raiseHTTPError(a,e_413);
		
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
				raiseHTTPError(a,e_414);
				
				return 0;
			}
			else
			{
				max=lstrlen(request.URI);
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
				raiseHTTPError(a,e_414);
				
				return 0;
			}
			else
			{
				max=lstrlen(request.URI);
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
				raiseHTTPError(a,e_414);
				
				return 0;
			}
		}

		if(!lstrcmpi(command,""))
		{
			raiseHTTPError(a,e_501);
			
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
			raiseHTTPError(a,e_400);
			return 0;
		}
		if(!lstrcmpi(request.RANGETYPE,"bytes"))
			sendHTTPRESOURCE(a,request.URI,FALSE,FALSE,atoi(request.RANGEBYTEBEGIN),atoi(request.RANGEBYTEEND));
		else
			sendHTTPRESOURCE(a,request.URI);
	}
	else if(!lstrcmpi(request.CMD,"POST"))
	{
		if(lstrlen(request.HOST)==0)
		{
			raiseHTTPError(a,e_400);
			return 0;
		}
		if(!lstrcmpi(request.RANGETYPE,"bytes"))
			sendHTTPRESOURCE(a,request.URI,FALSE,FALSE,atoi(request.RANGEBYTEBEGIN),atoi(request.RANGEBYTEEND));
		else
			sendHTTPRESOURCE(a,request.URI);
	}
	else if(!lstrcmpi(request.CMD,"HEAD"))
	{
		if(lstrlen(request.HOST)==0)
		{
			raiseHTTPError(a,e_400);
			
			return 0;
		}
		sendHTTPRESOURCE(a,request.URI,FALSE,TRUE);
	}
	else
	{
		raiseHTTPError(a,e_501);
		
		return 0;
	}
	if(lstrcmpi(request.CONNECTION,"Keep-Alive"))
	{
		
		return 0;
	}
	return 1;
}
void buildHttpResponseHeader(char *str,HTTP_RESPONSE_HEADER *response)
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
void buildDefaultHttpResponseHeader(HTTP_RESPONSE_HEADER* response)
{
	ZeroMemory(response,sizeof(HTTP_RESPONSE_HEADER));
	lstrcpy(response->MIME,"text/html");
	lstrcpy(response->VER,"1.1");
	lstrcpy(response->DATE,getHTTPFormattedTime());
	lstrcpy(response->DATEEXP,getHTTPFormattedTime());
	lstrcpy(response->SERVER_NAME,"MyServer");
}

void raiseHTTPError(LPCONNECTION a,int ID)
{
	static HTTP_RESPONSE_HEADER response;
	if(ID==e_401AUTH)
	{
		sprintf(buffer2,"HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic\r\nServer: %s\r\nDate: %s\r\nContent-type: text/html\r\nContent-length: 0\r\n\r\n",lserver->getServerName(),getHTTPFormattedTime());
		send(a->socket,buffer2,lstrlen(buffer2),0);
		return;
	}
	if(lserver->mustUseMessagesFiles())
	{
		sendHTTPRESOURCE(a,HTTP_ERROR_HTMLS[ID],TRUE);
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


BOOL sendCGI(LPCONNECTION s,char* filename,char* ext,char *exec)
{
	/*
	*Change the owner of the thread to the creator of the process.
	*This because anonymous users cannot go through our files.
	*/
	if(lserver->mustUseLogonOption())
		revertToSelf();
	static int len;
	char cmdLine[MAX_PATH*2];
	
	sprintf(cmdLine,"%s \"%s%s\"",exec,lserver->getPath(),filename);

    SECURITY_ATTRIBUTES sa = {0};  
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;
    /*
    *Use a temporary file to store CGI output.
    *Every thread has it own tmp file name(tmpBufferFilePath),
    *so use this name for the file that is going to be
    *created because more threads can access more CGI in the same time.
    */

	char currentpath[MAX_PATH];
	char tmpBufferFilePath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH,currentpath);
	static DWORD id=0;
	id++;
	sprintf(tmpBufferFilePath,"%s/%tmpbuffer_%u",currentpath,id);
	

	tmpBufferFile = CreateFile (tmpBufferFilePath, GENERIC_READ | GENERIC_WRITE,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                      &sa, OPEN_ALWAYS,FILE_ATTRIBUTE_TEMPORARY|FILE_ATTRIBUTE_HIDDEN, NULL);
    /*
    *Set the standard output values for the CGI process
    */
    STARTUPINFO si;
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    si.hStdInput = 0;
    si.hStdOutput = tmpBufferFile;
    si.hStdError= 0;
    si.dwFlags=STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi;
    ZeroMemory( &pi, sizeof(pi) );
	buffer2[0]='\0';
	/*
	*To set the CGI path modify the MIMEtypes file in the bin folder
	*/
    CreateProcess(NULL, cmdLine, NULL, NULL, TRUE,CREATE_SEPARATE_WOW_VDM|CREATE_NEW_CONSOLE,NULL,NULL,&si, &pi);
	/*
	*Wait until it's ending by itself
	*/
	WaitForSingleObject( pi.hProcess, INFINITE );

    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );

	
	DWORD nBytesRead;
	SetFilePointer(tmpBufferFile,0,0,SEEK_SET);
	ReadFile(tmpBufferFile,buffer2,buffersize2,&nBytesRead,NULL);
	len=nBytesRead;
	buffer2[len]='\0';
	/*
	*Standards CGI can include an extra HTTP header
	*so don't terminate with \r\n myServer header.
	*/	
	DWORD headerSize;
	for(headerSize=0;headerSize<len;headerSize++)
	{
		if(buffer2[headerSize]=='\r')
			if(buffer2[headerSize+1]=='\n')
				if(buffer2[headerSize+2]=='\r')
					if(buffer2[headerSize+3]=='\n')			
						break;
	}
	headerSize+=4;
	len=nBytesRead-headerSize;

	sprintf(response.CONTENTS_DIM,"%u",len);
	buildHttpResponseHeader(buffer,&response);

	/*
	*Send lstrlen(buffer)-2 because last two characters
	*are \r\n that terminating the HTTP header
	*/
	send(s->socket,buffer,lstrlen(buffer)-2, 0);
	/*
	*In buffer2 there are the CGI HTTP header and the 
	*contents of the page requested through the CGI
	*/
	send(s->socket,buffer2,nBytesRead, 0);

	CloseHandle(tmpBufferFile);
	DeleteFile(tmpBufferFilePath);

	/*
	*Restore security on the current thread
	*/
	if(lserver->mustUseLogonOption())
		impersonateLogonUser(*hImpersonation);
		
	return 1;
}
BOOL getMIME(char *MIME,char *filename,char *dest,char *dest2)
{
	getFileExt(dest,filename);
	/*
	*Return true if file is registered by a CGI
	*/
	return lserver->mimeManager.getMIME(dest,MIME,dest2);
}
void getPath(char *filenamePath,char *filename,BOOL systemrequest)
{
	if(systemrequest)
	{
		sprintf(filenamePath,"%s\\%s",lserver->getSystemPath(),filename);
	}
	else
	{
		sprintf(filenamePath,"%s\\%s",lserver->getPath(),filename);
	}
}
