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

#include "..\include\HTTP.h"
#include "..\include\cserver.h"
#include "..\include\security.h"
#include "..\include\AMMimeUtils.h"
#include "..\include\filemanager.h"
#include "..\include\sockets.h"
#include "..\include\utility.h"
#include <direct.h>

/*
*These variables are thread safe.
*/
char static Thread *buffer;
char static Thread  *buffer2;
int  static Thread buffersize;
int  static Thread buffersize2;
DWORD  Thread nBytesToRead;
HTTP_RESPONSE_HEADER  Thread response;
HTTP_REQUEST_HEADER  Thread request;
char Thread filenamePath[MAX_PATH];
LOGGEDUSERID Thread hImpersonation;

BOOL sendHTTPDIRECTORY(LPCONNECTION s,char* folder)
{
	/*
	*Send the content of a folder if there are not any default
	*file to send.
	*/
	static char filename[MAX_PATH];
	static DWORD startChar=lstrlen(lserver->getPath())+1;
	
	if(getPathRecursionLevel(folder)<1)
	{
		return raiseHTTPError(s,e_401);
	}
	ZeroMemory(buffer2,200);
	_finddata_t fd;
	sprintf(filename,"%s/*.*",folder);
	sprintf(buffer2,"%s",msgFolderContents);
	lstrcat(buffer2," ");
	lstrcat(buffer2,&folder[startChar]);
	lstrcat(buffer2,"\\<P>\n<HR>");
	__int64 ff;
	ff=_findfirst(filename,&fd);

	if(ff==-1)
	{
		if(GetLastError()==ERROR_ACCESS_DENIED)
		{
			/*
			*If client have tried to post a login
			*and a password more times send error 
			*/
			if(s->nTries > 2)
			{
				return raiseHTTPError(s,e_401);
			}
			else
			{
				s->nTries++;
				return raiseHTTPError(s,e_401AUTH);
			}
		}
		else
		{
			return raiseHTTPError(s,e_404);
		}
	}
	/*
	*With the current code we build the HTML TABLE that describe the files in the folder
	*/
	sprintf(buffer2+lstrlen(buffer2),"<TABLE><TR><TD>%s</TD><TD>%s</TD><TD>%s</TD></TR>",msgFile,msgLModify,msgSize);
	static char fileSize[10];
	static char fileTime[20];
	do
	{	
		if(fd.name[0]=='.')
			continue;
		request;
		lstrcat(buffer2,"<TR><TD><A HREF=\"");
		lstrcat(buffer2,&folder[startChar]);
		lstrcat(buffer2,"/");
		lstrcat(buffer2,fd.name);
		lstrcat(buffer2,"\">");
		lstrcat(buffer2,fd.name);
		lstrcat(buffer2,"</TD><TD>");
			
		tm *st=gmtime(&fd.time_write);

		sprintf(fileTime,"%u\\%u\\%u-%u:%u:%u System time",st->tm_wday,st->tm_mon,st->tm_year,st->tm_hour,st->tm_min,st->tm_sec);
		lstrcat(buffer2,fileTime);

		lstrcat(buffer2,"</TD><TD>");
		if(fd.attrib & FILE_ATTRIBUTE_DIRECTORY)
		{
			lstrcat(buffer2,"<dir>");
		}
		else
		{
			sprintf(fileSize,"%i bytes",fd.size);
			lstrcat(buffer2,fileSize);
		}
		lstrcat(buffer2,"</TD></TR>\n");
	}while(!_findnext(ff,&fd));
	lstrcat(buffer2,"</TABLE>\n<HR>");
	lstrcat(buffer2,msgRunOn);
	lstrcat(buffer2," myServer ");
	lstrcat(buffer2,versionOfSoftware);
	_findclose(ff);
	char *buffer2Loop=buffer2;
	while(*buffer2Loop++)
		if(*buffer2Loop=='\\')
			*buffer2Loop='/';
	buildDefaultHTTPResponseHeader(&response);	
	sprintf(response.CONTENTS_DIM,"%u",lstrlen(buffer2));
	buildHTTPResponseHeader(buffer,&response);
	ms_send(s->socket,buffer,lstrlen(buffer), 0);
	ms_send(s->socket,buffer2,lstrlen(buffer2), 0);

	return 1;

}
BOOL sendHTTPFILE(LPCONNECTION s,char *filenamePath,BOOL OnlyHeader,int firstByte,int lastByte)
{
	/*
	*With this code we send a file through the HTTP protocol
	*/
	MYSERVER_FILE_HANDLE h;
	h=ms_OpenFile(filenamePath,MYSERVER_FILE_OPEN_IFEXISTS|MYSERVER_FILE_OPEN_READ);

	if(h==0)
	{	
		if(GetLastError()==ERROR_ACCESS_DENIED)
		{
			if(s->nTries > 2)
			{
				return raiseHTTPError(s,e_401);
			}
			else
			{
				s->nTries++;
				return raiseHTTPError(s,e_401AUTH);
			}
		}
		else
		{
			return 0;
		}
	}
	/*
	*If h!=0
	*/

	DWORD filesize=getFileSize(h);
	if(lastByte != -1)
	{
		lastByte=min((DWORD)lastByte,filesize);
		filesize-=(filesize-firstByte);
	}
	if(firstByte)
		filesize-=firstByte;

	if(setFilePointer(h,firstByte))
	{
		return	raiseHTTPError(s,e_500);
	}


	ZeroMemory(buffer,300);

	sprintf(response.CONTENTS_DIM,"%u",filesize);
	buildHTTPResponseHeader(buffer,&response);
	ms_send(s->socket,buffer,lstrlen(buffer), 0);

	/*
	*If is requested only the header; HEAD request
	*/
	if(OnlyHeader)
		return 1;

	if(lserver->getVerbosity()>2)
	{
		char msg[500];
		sprintf(msg,"%s %s\n",msgSending,filenamePath);
		warningsLogWrite(msg);
	}
	for(;;)
	{
		DWORD nbr;
		ms_ReadFromFile(h,buffer,buffersize,&nbr);
		if(nbr==0)
			break;
		if(ms_send(s->socket,buffer,nbr, 0) == SOCKET_ERROR)
			break;
	}
	ms_CloseFile(h);
	return 1;

}

/*
*Main function to send a resource to a client
*/
BOOL sendHTTPRESOURCE(LPCONNECTION s,char *filename,BOOL systemrequest,BOOL OnlyHeader,int firstByte,int lastByte)
{
	/*
	*With this code we manage a request of a file or a folder or anything that we must send
	*over the HTTP
	*/
	buffer[0]='\0';
	buildDefaultHTTPResponseHeader(&response);

	static char ext[MAX_PATH];
	static char data[MAX_PATH];
	getPath(filenamePath,filename,systemrequest);
	/*
	*getMIME return TRUE if the ext is registered by a CGI
	*/

	if(getMIME(response.MIME,filename,ext,data))
	{
		if(ms_FileExists(filenamePath))
			if(sendCGI(s,filenamePath,ext,data))
				return 1;
		return raiseHTTPError(s,e_404);
	}

	char *c=filenamePath;
	while(*c)
	{
		if(*c=='\\')
			*c='/';
		c++;
	}
	if((*(c-1))=='/')(*(c-1))='\0';
	/*
	*If there are not any extension then we do one of this in order:
	1)We send the default file in the folder
	2)We send the folder content
	3)We send an error
	*/
	if(ms_IsFolder(filenamePath))
	{
		static char defaultFileName[MAX_PATH];
		sprintf(defaultFileName,"%s%s",filenamePath,lserver->getDefaultFilenamePath());

		if(sendHTTPFILE(s,defaultFileName,OnlyHeader,firstByte,lastByte))
			return 1;

		if(sendHTTPDIRECTORY(s,filenamePath))
			return 1;	

		return raiseHTTPError(s,e_404);
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
		return raiseHTTPError(s,e_404);
	}
	if(ms_IsFolder(filenamePath))
	{
		if(sendHTTPDIRECTORY(s,filenamePath))
			return 1;	
	}
	else
	{
		if(sendHTTPFILE(s,filenamePath,OnlyHeader,firstByte,lastByte))
			return 1;
	}
	return raiseHTTPError(s,e_404);
}
/*
*Sends the myServer CGI; differently form standard CGI this don't need a new process to run
*so it is faster
*/
BOOL sendMSCGI(LPCONNECTION s,char* exec,char* cmdLine)
{
	/*
	*This is the code for manage a .mscgi file.
	*This files differently from standard CGI don't need a new process to run
	*but are allocated in the caller process virtual space.
	*Usually these files are faster than standard CGI.
	*Actually myServerCGI(.mscgi) are only at an alpha status.
	*/
#ifdef WIN32
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
				return raiseHTTPError(s,e_403);
			}
			else
			{
				s->nTries++;
				return raiseHTTPError(s,e_401AUTH);
			}
		}
		else
		{
			return raiseHTTPError(s,e_404);
		}
	}
	static int len;
	len=lstrlen(buffer2);
	sprintf(response.CONTENTS_DIM,"%u",len);
	buildHTTPResponseHeader(buffer,&response);
	ms_send(s->socket,buffer,lstrlen(buffer), 0);
	ms_send(s->socket,buffer2,len, 0);
	return 1;
#endif
	return 0;
}

/*
*This is the HTTP protocol parser
*/
BOOL controlHTTPConnection(LPCONNECTION a,char *b1,char *b2,int bs1,int bs2,DWORD nbtr,LOGGEDUSERID *imp)
{
	buffer=b1;
	buffer2=b2;
	buffersize=bs1;
	buffersize2=bs2;
	nBytesToRead=nbtr;
	hImpersonation=*imp;
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
		buffer[nBytesToRead]='\n';
		buffer[nBytesToRead+1]='\0';
		warningsLogWrite(buffer);
	}
	if(isValidCommand==FALSE)
	{
		raiseHTTPError(a,e_400);
		/*
		*Returning Zero we remove the connection from the connections list
		*/
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

			/*
			*URIOPTSPTR points to the first character in the buffer that are data send by the
            *client.
			*/
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

	/*
	*Record the request in the log file
	*/
	accessesLogWrite(a->ipAddr);
	accessesLogWrite(":");
	accessesLogWrite(request.CMD);
	accessesLogWrite(" ");
	accessesLogWrite(request.URI);
	if(request.URIOPTS[0])
	{
		accessesLogWrite("?");
		accessesLogWrite(request.URIOPTS);
	}
	accessesLogWrite("\r\n");
	/*
	*End of record the request in the structure
	*/
	
	/*
	*Here we control all the HTTP commands
	*/
	if(!lstrcmpi(request.CMD,"GET"))
	{
		/*
		*How is expressly said in the rfc2616 a client that sends an 
		*HTTP/1.1 request MUST send a Host header.
		*Servers MUST report a 400 (Bad Request) error if an HTTP/1.1
        *request does not include a Host request-header.
		*/
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
/*
*Builds an HTTP header string starting from an HTTP_RESPONSE_HEADER structure
*/
void buildHTTPResponseHeader(char *str,HTTP_RESPONSE_HEADER *response)
{
	/*
	*Here is builded the HEADER of a HTTP response.
	*Passing a HTTP_RESPONSE_HEADER struct this builds
	*a header string
	*Every directive ends with a \r\n sequence
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
	/*
	*The HTTP header ends with a \r\n sequence
	*/
	lstrcat(str,"\r\n");

}
/*
*Set the defaults value for a HTTP_RESPONSE_HEADER structure
*/
void buildDefaultHTTPResponseHeader(HTTP_RESPONSE_HEADER* response)
{
	ZeroMemory(response,sizeof(HTTP_RESPONSE_HEADER));
	/*
	*By default use:
	*1) the MIME type of the page equal to text/html
	*2) the version of the HTTP protocol to 1.1
	*3) the date of the page and the expiration date to the current time
	*4) then set the name of the server
	*5) set the page that it is not an error page
	*/
	lstrcpy(response->MIME,"text/html");
	lstrcpy(response->VER,"1.1");
	response->isError=FALSE;
	lstrcpy(response->DATE,getHTTPFormattedTime());
	lstrcpy(response->DATEEXP,getHTTPFormattedTime());
	sprintf(response->SERVER_NAME,"MyServer %s",versionOfSoftware);
}
/*
*Sends an error page to the client described by the connection
*/
BOOL raiseHTTPError(LPCONNECTION a,int ID)
{
	static HTTP_RESPONSE_HEADER response;
	if(ID==e_401AUTH)
	{
		sprintf(buffer2,"HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic\r\nServer: %s\r\nDate: %s\r\nContent-type: text/html\r\nContent-length: 0\r\n\r\n",lserver->getServerName(),getHTTPFormattedTime());
		ms_send(a->socket,buffer2,lstrlen(buffer2),0);
		return 1;
	}
	if(lserver->mustUseMessagesFiles())
	{
		 return sendHTTPRESOURCE(a,HTTP_ERROR_HTMLS[ID],TRUE);
		
	}
	buildDefaultHTTPResponseHeader(&response);

	response.isError=TRUE;
	lstrcpy(response.ERROR_TYPE,HTTP_ERROR_MSGS[ID]);
	sprintf(response.CONTENTS_DIM,"%i",lstrlen(HTTP_ERROR_MSGS[ID]));
	buildHTTPResponseHeader(buffer,&response);
	lstrcat(buffer,HTTP_ERROR_MSGS[ID]);
	ms_send(a->socket,buffer,lstrlen(buffer), 0);
	return 1;
}

/*
*Sends the standard CGI to a client
*/
BOOL sendCGI(LPCONNECTION s,char* filename,char* ext,char *exec)
{
	/*
	*Change the owner of the thread to the creator of the process.
	*This because anonymous users cannot go through our files.
	*/
	if(lserver->mustUseLogonOption())
		revertToSelf();
	char cmdLine[MAX_PATH*2];
	
	sprintf(cmdLine,"%s \"%s\"",exec,filename);

    /*
    *Use a temporary file to store CGI output.
    *Every thread has it own tmp file name(stdOutFilePath),
    *so use this name for the file that is going to be
    *created because more threads can access more CGI in the same time.
    */

	char currentpath[MAX_PATH];
	char stdOutFilePath[MAX_PATH];
	char stdInFilePath[MAX_PATH];
	_getcwd(currentpath,MAX_PATH);
	static DWORD id=0;
	id++;
	sprintf(stdOutFilePath,"%s/stdOutFile_%u",currentpath,id);
	sprintf(stdInFilePath,"%s/stdInFile_%u",currentpath,id);
	buffer2[0]='\0';
	
	/*
	*Standard CGI uses standard output to output the result and the standard 
	*input to get other params like in a POST request
	*/

	MYSERVER_FILE_HANDLE stdOutFile = ms_CreateTemporaryFile(stdOutFilePath);
	MYSERVER_FILE_HANDLE stdInFile = ms_CreateTemporaryFile(stdInFilePath);
	
	DWORD nbw;
	if(request.URIOPTSPTR)
		ms_WriteToFile(stdInFile,request.URIOPTSPTR,atoi(request.CONTENTS_DIM),&nbw);
	char *endFileStr="\r\n\r\n\0";
	ms_WriteToFile(stdInFile,endFileStr,lstrlen(endFileStr),&nbw);
	setFilePointer(stdInFile,0);

	/*
	*With this code we execute the CGI process
	*/
	START_PROC_INFO spi;
	spi.cmdLine = cmdLine;
	spi.stdError = (MYSERVER_FILE_HANDLE)0;
	spi.stdIn = (MYSERVER_FILE_HANDLE)stdInFile;
	spi.stdOut = (MYSERVER_FILE_HANDLE)stdOutFile;
	execHiddenProcess(&spi);
	
	/*
	*Read the CGI output
	*/
	DWORD nBytesRead;
	setFilePointer(stdOutFile,0);
	ms_ReadFromFile(stdOutFile,buffer2,buffersize2,&nBytesRead);
	buffer2[max(buffersize2,nBytesRead)]='\0';

	/*
	*Standard CGI can include an extra HTTP header
	*so don't terminate with \r\n the default myServer header.
	*/	
	DWORD headerSize=0;
	for(DWORD i=0;i<nBytesRead;i++)
	{
		if(buffer2[i]=='\r')
			if(buffer2[i+1]=='\n')
				if(buffer2[i+2]=='\r')
					if(buffer2[i+3]=='\n')
					{
						headerSize=i+4;
						break;
					}
	}
	int len=nBytesRead-headerSize;

	sprintf(response.CONTENTS_DIM,"%u",len);
	buildHTTPResponseHeader(buffer,&response);

	/*
	*If there is an extra header, send lstrlen(buffer)-2 because the
	*last two characters are \r\n that terminating the HTTP header
	*/
	if(headerSize)
		ms_send(s->socket,buffer,lstrlen(buffer)-2, 0);
	else
		ms_send(s->socket,buffer,lstrlen(buffer), 0);

	/*
	*In buffer2 there are the CGI HTTP header and the 
	*contents of the page requested through the CGI
	*/
	ms_send(s->socket,buffer2,nBytesRead, 0);

	ms_CloseFile(stdOutFile);
	ms_DeleteFile(stdOutFilePath);
	ms_CloseFile(stdInFile);
	ms_DeleteFile(stdInFilePath);

	/*
	*Restore security on the current thread
	*/
	if(lserver->mustUseLogonOption())
		impersonateLogonUser(&hImpersonation);
		
	return 1;
}
/*
*Returns the MIME type passing its extension
*/
BOOL getMIME(char *MIME,char *filename,char *dest,char *dest2)
{
	getFileExt(dest,filename);
	/*
	*Returns true if file is registered by a CGI
	*/
	return lserver->mimeManager.getMIME(dest,MIME,dest2);
}
/*
*Map an URL to the machine file system
*/
void getPath(char *filenamePath,char *filename,BOOL systemrequest)
{
	if(systemrequest)
	{
		sprintf(filenamePath,"%s/%s",lserver->getSystemPath(),filename);
	}
	else
	{
		sprintf(filenamePath,"%s/%s",lserver->getPath(),filename);
	}
}
