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

#include "../include/http.h"
#include "../include/cserver.h"
#include "../include/security.h"
#include "../include/AMMimeUtils.h"
#include "../include/filemanager.h"
#include "../include/sockets.h"
#include "../include/utility.h"
#include "../include/isapi.h"
extern "C" {
#ifdef WIN32
#include <direct.h>
#include <errno.h>
#else
#include <string.h>
#endif
}

#ifndef WIN32
#include "../include/lfind.h"

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define lstrcmpi strcmp
#define lstrcpy strcpy
#define lstrcat strcat
#define lstrlen strlen
#endif

// Bloodshed Dev-C++ Helper
#ifndef intptr_t
#define intptr_t int
#endif

int sendHTTPDIRECTORY(httpThreadContext* td,LPCONNECTION s,char* folder)
{
	/*
	*Send the folder content.
	*/
	static char filename[MAX_PATH];
	int startchar=0;
	int nDirectories=0;
	int i;
	for(i=0;td->request.URI[i];i++)
	{
		if(td->request.URI[i]=='/')
			nDirectories++;
	}
	for(startchar=0,i=0;td->request.URI[i];i++)
	{
		if(td->request.URI[i]=='/')
		{
			startchar++;
			if(startchar==nDirectories)
			{
				/*
				*At the end of the loop set startchar to te real value.
				*startchar indicates the initial position in td->request.URI 
				*of the file path.
				*/
				startchar=i+1;
				break;
			}
		}
	}
	td->buffer2[0]='\0';
	_finddata_t fd;
	sprintf(td->buffer2,"<HTML><HEAD><TITLE>%s</TITLE></HEAD><BASE>",td->filenamePath);
	/*
	*If it is defined a CSS file for the graphic layout of the browse folder insert it in the page.
	*/
	if(lserver->getBrowseDirCSS()[0])
	{
		MYSERVER_FILE_HANDLE cssHandle=ms_OpenFile(lserver->getBrowseDirCSS(),MYSERVER_FILE_OPEN_IFEXISTS|MYSERVER_FILE_OPEN_READ);
		u_long nbr;
		ms_ReadFromFile(cssHandle,td->buffer,td->buffersize,&nbr);
		lstrcat(td->buffer2,"<STYLE><!--");
		lstrcat(td->buffer2,td->buffer);
		lstrcat(td->buffer2,"--></STYLE>");
		ms_CloseFile(cssHandle);
	}

#ifdef WIN32
	sprintf(filename,"%s/*.*",folder);
#else
	sprintf(filename,"%s/",folder);
#endif
	lstrcat(td->buffer2,"<BODY>");
	lstrcat(td->buffer2,msgFolderContents);
	lstrcat(td->buffer2," ");
	lstrcat(td->buffer2,&td->request.URI[startchar]);
	lstrcat(td->buffer2,"\\<P>\n<HR>");
	intptr_t ff;
	ff=_findfirst(filename,&fd);

#ifdef WIN32
	if(ff==-1)
#else
	if((int)ff==-1)
#endif
	{
		if(errno==EACCES)
		{
			/*
			*If client have tried to post a login
			*and a password more times send error 401.
			*/
			if(s->nTries > 2)
			{
				return raiseHTTPError(td,s,e_401);
			}
			else
			{	
				s->nTries++;
				return raiseHTTPError(td,s,e_401AUTH);
			}
		}
		else
		{
			return raiseHTTPError(td,s,e_404);
		}
	}
	/*
	*With the current code we build the HTML TABLE that describes the files in the folder.
	*/
	sprintf(td->buffer2+lstrlen(td->buffer2),"<TABLE><TR><TD>%s</TD><TD>%s</TD><TD>%s</TD></TR>",msgFile,msgLModify,msgSize);
	static char fileSize[10];
	static char fileTime[20];
	do
	{	
		if(fd.name[0]=='.')
			continue;
		lstrcat(td->buffer2,"<TR><TD><A HREF=\"");
		if(!td->request.uriEndsWithSlash)
		{
			lstrcat(td->buffer2,&td->request.URI[startchar]);
			lstrcat(td->buffer2,"/");
		}
		lstrcat(td->buffer2,fd.name);
		lstrcat(td->buffer2,"\">");
		lstrcat(td->buffer2,fd.name);
		lstrcat(td->buffer2,"</TD><TD>");
			
		tm *st=gmtime(&fd.time_write);

		sprintf(fileTime,"%u\\%u\\%u-%u:%u:%u System time",st->tm_wday,st->tm_mon,st->tm_year,st->tm_hour,st->tm_min,st->tm_sec);
		lstrcat(td->buffer2,fileTime);

		lstrcat(td->buffer2,"</TD><TD>");
		if(fd.attrib & FILE_ATTRIBUTE_DIRECTORY)
		{
			lstrcat(td->buffer2,"[dir]");
		}
		else
		{
			sprintf(fileSize,"%i bytes",fd.size);
			lstrcat(td->buffer2,fileSize);
		}
		lstrcat(td->buffer2,"</TD></TR>\n");
	}while(!_findnext(ff,&fd));
	lstrcat(td->buffer2,"</TABLE>\n<HR>");
	lstrcat(td->buffer2,msgRunOn);
	lstrcat(td->buffer2," myServer ");
	lstrcat(td->buffer2,versionOfSoftware);
	lstrcat(td->buffer2,"</BODY></HTML>");
	_findclose(ff);
	char *buffer2Loop=td->buffer2;
	while(*buffer2Loop++)
		if(*buffer2Loop=='\\')
			*buffer2Loop='/';
	buildDefaultHTTPResponseHeader(&(td->response));
	sprintf(td->response.CONTENTS_DIM,"%u",lstrlen(td->buffer2));
	buildHTTPResponseHeader(td->buffer,&(td->response));
	ms_send(s->socket,td->buffer,lstrlen(td->buffer), 0);
	ms_send(s->socket,td->buffer2,lstrlen(td->buffer2), 0);
	return 1;

}
int sendHTTPFILE(httpThreadContext* td,LPCONNECTION s,char *filenamePath,int OnlyHeader,int firstByte,int lastByte)
{
	/*
	*With this routine we send a file through the HTTP protocol.
	*Open the file and save its handle.
	*/
	MYSERVER_FILE_HANDLE h=ms_OpenFile(filenamePath,MYSERVER_FILE_OPEN_IFEXISTS|MYSERVER_FILE_OPEN_READ);

	if(h==0)
	{	
		return 0;
	}
	else if(h==(MYSERVER_FILE_HANDLE)-1)
	{
		if(s->nTries > 2)
		{
			return raiseHTTPError(td,s,e_401);
		}
		else
		{
			s->nTries++;
			return raiseHTTPError(td,s,e_401AUTH);
		}
	}
	/*
	*If the file is a valid handle.
	*/
	u_long bytesToSend=ms_getFileSize(h);
	if(lastByte == -1)
	{
		lastByte=bytesToSend;
	}
	else/*If the client use ranges set the right value for the last byte number*/
	{
		lastByte=min((u_long)lastByte,bytesToSend);
	}
	/*
	*bytesToSend is the interval between the first and the last byte.
	*/
	bytesToSend=lastByte-firstByte;

	/*
	*If failed to set the file pointer returns an internal server error.
	*/
	if(ms_setFilePointer(h,firstByte))
	{
		return raiseHTTPError(td,s,e_500);
	}

	td->buffer[0]='\0';

	sprintf(td->response.CONTENTS_DIM,"%u",bytesToSend);
	buildHTTPResponseHeader(td->buffer,&td->response);
	ms_send(s->socket,td->buffer,lstrlen(td->buffer), 0);

	/*
	*If is requested only the header exit from the function; used by the HEAD request.
	*/
	if(OnlyHeader)
		return 1;

	if(lserver->getVerbosity()>2)
	{
		char msg[500];
		sprintf(msg,"%s %s\n",msgSending,filenamePath);
		ms_warningsLogWrite(msg);
	}
	for(;;)
	{
		u_long nbr;
		/*
		*Read from the file the bytes to sent.
		*/
		ms_ReadFromFile(h,td->buffer,min(bytesToSend,td->buffersize),&nbr);
		/*
		*When the bytes read from the file are zero, stop to send the file.
		*/
		if(nbr==0)
			break;
		/*
		*If there are bytes to send, send them.
		*/
		if(ms_send(s->socket,td->buffer,nbr, 0) == SOCKET_ERROR)
			break;
	}
	ms_CloseFile(h);
	return 1;

}

/*
*Main function to send a resource to a client.
*/
int sendHTTPRESOURCE(httpThreadContext* td,LPCONNECTION s,char *filename,int systemrequest,int OnlyHeader,int firstByte,int lastByte,int yetmapped)
{
	/*
	*With this code we manage a request of a file or a folder or anything that we must send
	*over the HTTP.
	*/
	td->buffer[0]='\0';
	buildDefaultHTTPResponseHeader(&td->response);

	static char ext[10];
	static char data[MAX_PATH];
	/*
	*td->filenamePath is the file system mapped path while filename is the URI requested.
	*systemrequest is true if the file is in the system folder.
	*If filename is already mapped on the file system don't map it again.
	*/
	if(yetmapped)
		lstrcpy((td->filenamePath),filename);
	else
		getPath((td->filenamePath),filename,systemrequest);

	/*
	*Get the PATH_INFO value.
	*Use dirscan as a buffer for put temporary directory scan.
	*When an '/' character is present check if the path up to '/' character
	*is a file. If it is a file send the rest of the URI as PATH_INFO.
	*/
	char dirscan[MAX_PATH];
	//dirscan[0]='\0';
	memset(dirscan, 0, MAX_PATH);
	td->pathInfo[0]='\0';
	td->pathTranslated[0]='\0';
	for(int i=0;;i++)
	{
		/*
		*http://127.0.0.1/uri/filetosend.php/PATH_INFO_VALUE?QUERY_INFO_VALUE
		*When a request has this form send the file filetosend.php with the
		*environment string PATH_INFO setted to PATH_INFO_VALUE and QUERY_INFO
		*to QUERY_INFO_VALUE.
		*/
		int len=lstrlen(dirscan);
		if((td->filenamePath)[i]==0)/*If we are at the end of the string break the loop*/
			break;
        else if((td->filenamePath)[i]!='/' || i==0)/*If there is a character different from '/'*/
		{
			dirscan[len]=(td->filenamePath)[i];
			dirscan[len+1]='\0';
		}
		else/*There is the '/' character check if dirscan is a file*/
		{
			if(!ms_IsFolder(dirscan))
			{
				/*
				*Yes it is a file.
				*/
				lstrcpy(td->pathInfo,&((td->filenamePath)[i]));
				lstrcpy(td->filenamePath,dirscan);
				break;
			}
			/*
			*If it is not a file put a the end of dirscan the '/' character for the next scansion.
			*/
			dirscan[len]=(td->filenamePath)[i];
			dirscan[len+1]='\0';
		}
	}
	
	/*
	*If there is a PATH_INFO value the get the PATH_TRANSLATED too.
	*PATH_TRANSLATED is the mapped to the local filesystem version of PATH_INFO.
	*/
	if(td->pathInfo[0])
	{
        td->pathTranslated[0]='\0';
		/*
		*Start from the second character because the first is a slash character.
		*/
		getPath((td->pathTranslated),&((td->pathInfo)[1]),false);
	}
	else
	{
        td->pathTranslated[0]='\0';
	}
	/*
	*If the client try to access files that aren't in the web folder send a 401 error.
	*/
	if((ms_getPathRecursionLevel(filename)<1) && filename[0] != '\0')
	{
		return raiseHTTPError(td,s,e_401);
	}

	/*
	*If there are not any extension then we do one of this in order:
	1)We send the default file in the folder.
	2)We send the folder content.
	3)We send an error.
	*/
	if(ms_IsFolder((char *)(td->filenamePath)))
	{
		static char defaultFileName[MAX_PATH];
		sprintf(defaultFileName,"%s/%s",td->filenamePath,lserver->getDefaultFilenamePath());

		if(sendHTTPFILE(td,s,defaultFileName,OnlyHeader,firstByte,lastByte))
			return 1;

		if(sendHTTPDIRECTORY(td,s,td->filenamePath))
			return 1;	

		return raiseHTTPError(td,s,e_404);
	}
	/*
	*getMIME return true if the ext is registered by a CGI.
	*/
	int mimeCMD=getMIME(td->response.CONTENTS_TYPE,td->filenamePath,ext,data);

	if((mimeCMD==CGI_CMD_RUNCGI)||(mimeCMD==CGI_CMD_EXECUTE))
	{
		if(ms_FileExists(td->filenamePath))
			if(sendCGI(td,s,td->filenamePath,ext,data,mimeCMD))
				return 1;
		return raiseHTTPError(td,s,e_404);
	}else if(mimeCMD==CGI_CMD_RUNISAPI)
	{
		if(ms_FileExists(td->filenamePath))
		{
#ifdef WIN32
			return sendISAPI(td,s,td->filenamePath,ext,data);
#else
			return raiseHTTPError(td,s,e_501);
#endif
		}
		else
			return raiseHTTPError(td,s,e_404);
	}
	else if(mimeCMD==CGI_CMD_RUNMSCGI)
	{
		char *target;
		if(td->request.URIOPTSPTR)
			target=td->request.URIOPTSPTR;
		else
			target=(char*)&td->request.URIOPTS;
		if(sendMSCGI(td,s,td->filenamePath,target))
			return 1;
		return raiseHTTPError(td,s,e_404);
	}
	else if(mimeCMD==CGI_CMD_SENDLINK)
	{
		MYSERVER_FILE_HANDLE h=ms_OpenFile(td->filenamePath,MYSERVER_FILE_OPEN_IFEXISTS|MYSERVER_FILE_OPEN_READ);
		u_long nbr;
		char linkpath[MAX_PATH];
		char pathInfo[MAX_PATH];
		ms_ReadFromFile(h,linkpath,MAX_PATH,&nbr);
		ms_CloseFile(h);
		linkpath[nbr]='\0';
		lstrcpy(pathInfo,td->pathInfo);
		translateEscapeString(pathInfo);
		lstrcat(linkpath,pathInfo);

		if(nbr)
			return sendHTTPRESOURCE(td,s,linkpath,systemrequest,OnlyHeader,firstByte,lastByte,1);
		else
			return raiseHTTPError(td,s,e_404);
	}
	
	if(sendHTTPFILE(td,s,td->filenamePath,OnlyHeader,firstByte,lastByte))
		return 1;

	return raiseHTTPError(td,s,e_404);
}

/*
*This is the HTTP protocol main procedure.
*/
int controlHTTPConnection(LPCONNECTION a,char *b1,char *b2,int bs1,int bs2,u_long nbtr,LOGGEDUSERID *imp,u_long id)
{
	/*
	*Bit mask.
	*|...|31|32|
	*Bit 32		->	Return value;
	*Bit 31		->	Return from the function;
	*Bits 1-30	->	Don't used.
	*/
	int retvalue=0;

	httpThreadContext td;
	td.buffer=b1;
	td.buffer2=b2;
	td.buffersize=bs1;
	td.buffersize2=bs2;
	td.nBytesToRead=nbtr;
	td.hImpersonation=*imp;
	td.identity[0]='\0';
	td.connection=a;
	td.id=id;
	td.inputData =(MYSERVER_FILE_HANDLE)0;


	/*
	*Reset the request structure.
	*/
	resetHTTPRequest(&td.request);

	u_long validRequest=buildHTTPRequestHeaderStruct(&td);


	/*
	*If the server verbosity is > 4 then save the HTTP request header.
	*/
	if(lserver->getVerbosity()>4)
	{
		td.buffer[td.nBytesToRead]='\n';
		td.buffer[td.nBytesToRead+1]='\0';
		ms_warningsLogWrite(td.buffer);
	}
	/*
	*If the header is an invalid request send the correct error message to the client and return immediately.
	*/
	if(validRequest==0)
	{
		raiseHTTPError(&td,a,e_400);
		/*
		*Returning Zero we remove the connection from the connections list.
		*/
		return 0;
	}/*If the URI is too long*/
	else if(validRequest==414)
	{
		return raiseHTTPError(&td,a,e_414);
	}
	

	/*
	*For methods that accept data after the HTTP header set the correct pointer and create a file
	*containing the informations after the header.
	*/
	if(!lstrcmpi(td.request.CMD,"POST"))
	{
		td.request.URIOPTSPTR=&td.buffer[td.nHeaderChars];
		td.buffer[min(td.nBytesToRead,td.buffersize)]='\0';
		/*
		*Create the file that contains the data posted.
		*This data is the stdin file in the CGI.
		*/
		char stdInFilePath[MAX_PATH];
		ms_getdefaultwd(stdInFilePath,MAX_PATH);
		sprintf(&stdInFilePath[lstrlen(stdInFilePath)],"/stdInFile__%u",td.id);
		td.inputData=ms_CreateTemporaryFile(stdInFilePath);
		u_long nbw;
		ms_WriteToFile(td.inputData,td.request.URIOPTSPTR,min(td.nBytesToRead,td.buffersize)-td.nHeaderChars,&nbw);
		/*
		*If there are others bytes to read from the socket.
		*/
		if(ms_bytesToRead(td.connection->socket))
		{
			int err;
			do
			{
				err=ms_recv(td.connection->socket,td.buffer2,td.buffersize2, 0);
				if(err==-1)
				{
					/*
					If we get an error remove the file and the connection.
					*/
					ms_CloseFile(td.inputData);
					td.inputData=0;
					retvalue|=1;/*set return value to 1.*/
					retvalue|=2;
				}
				ms_WriteToFile(td.inputData,td.buffer2,err,&nbw);
			}
			while(err);
		}
		ms_setFilePointer(td.inputData,0);
		td.buffer2[0]='\0';
	}
	else
	{
		td.request.URIOPTSPTR=0;
	}
	
	if(!(retvalue&2))/*If return value is not setted.*/
	{
		/*
		*Record the request in the log file.
		*/
		ms_accessesLogWrite(a->ipAddr);
		ms_accessesLogWrite(":");
		ms_accessesLogWrite(td.request.CMD);
		ms_accessesLogWrite(" ");
		ms_accessesLogWrite(td.request.URI);
		if(td.request.URIOPTS[0])
		{
			ms_accessesLogWrite("?");
			ms_accessesLogWrite(td.request.URIOPTS);
		}
		ms_accessesLogWrite("\r\n");
		/*
		*End record the request in the structure.
		*/

		/*
		*How is expressly said in the rfc2616 a client that sends an 
		*HTTP/1.1 request MUST sends a Host header.
		*Servers MUST reports a 400 (Bad request) error if an HTTP/1.1
		*request does not include a Host request-header.
		*/
		if(td.request.HOST[0]==0)
		{
			raiseHTTPError(&td,a,e_400);
			if(td.inputData)
			{
				ms_CloseFile(td.inputData);
				td.inputData=0;
			}
			retvalue=0xFFFFFFFE & (~1);/*Set first bit to 0*/
			return 0;
		}

		
		/*
		*Here we control all the HTTP commands.
		*/
		if(!lstrcmpi(td.request.CMD,"GET"))/*GET REQUEST*/
		{
			if(!lstrcmpi(td.request.RANGETYPE,"bytes"))

				sendHTTPRESOURCE(&td,a,td.request.URI,false,false,atoi(td.request.RANGEBYTEBEGIN),atoi(td.request.RANGEBYTEEND));
			else
				sendHTTPRESOURCE(&td,a,td.request.URI);
		}
		else if(!lstrcmpi(td.request.CMD,"POST"))/*POST REQUEST*/
		{
			if(!lstrcmpi(td.request.RANGETYPE,"bytes"))
				sendHTTPRESOURCE(&td,a,td.request.URI,false,false,atoi(td.request.RANGEBYTEBEGIN),atoi(td.request.RANGEBYTEEND));
			else
				sendHTTPRESOURCE(&td,a,td.request.URI);
		}
		else if(!lstrcmpi(td.request.CMD,"HEAD"))/*HEAD REQUEST*/
		{
			if(!lstrcmpi(td.request.RANGETYPE,"bytes"))
				sendHTTPRESOURCE(&td,a,td.request.URI,false,true,atoi(td.request.RANGEBYTEBEGIN),atoi(td.request.RANGEBYTEEND));
			else
				sendHTTPRESOURCE(&td,a,td.request.URI,false,true);
		}
		else
		{
			raiseHTTPError(&td,a,e_501);
			retvalue=0xFFFFFFFE & (~1);/*Set first bit to 0*/
		}
	}
	/*
	*If the connection is not Keep-Alive remove it from the connections list returning 0.
	*/
	if(!lstrcmpi(td.request.CONNECTION,"Keep-Alive"))
	{
		retvalue|=1;/*Set first bit to 1*/
		retvalue|=2;/*Set second bit to 1*/
	}
	else
	{
		retvalue=0xFFFFFFFE & (~1);/*Set first bit to 0*/
		retvalue|=2;/*Set second bit to 1*/
	}
	/*
	*If the inputData file was not closed close it.
	*/
	if(td.inputData)
	{
		ms_CloseFile(td.inputData);
		td.inputData=0;
	}
	return (retvalue&1)?1:0;
}
/*
*Reset all the HTTP_REQUEST_HEADER structure members.
*/
void resetHTTPRequest(HTTP_REQUEST_HEADER *request)
{
	request->CMD[0]='\0';		
	request->VER[0]='\0';		
	request->ACCEPT[0]='\0';
	request->AUTH[0]='\0';
	request->ACCEPTENC[0]='\0';	
	request->ACCEPTLAN[0]='\0';	
	request->ACCEPTCHARSET[0]='\0';
	request->CONNECTION[0]='\0';
	request->USER_AGENT[0]='\0';
	request->COOKIE[0]='\0';
	request->CONTENTS_TYPE[0]='\0';
	request->CONTENTS_DIM[0]='\0';
	request->DATE[0]='\0';
	request->FROM[0]='\0';
	request->DATEEXP[0]='\0';	
	request->MODIFIED_SINCE[0]='\0';
	request->LAST_MODIFIED[0]='\0';	
	request->URI[0]='\0';			
	request->URIOPTS[0]='\0';		
	request->URIOPTSPTR=NULL;		
	request->REFERER[0]='\0';	
	request->HOST[0]='\0';			
	request->OTHER[0]='\0';
	request->PRAGMA[0]='\0';
	request->RANGETYPE[0]='\0';		
	request->RANGEBYTEBEGIN[0]='\0';
	request->RANGEBYTEEND[0]='\0';
	request->uriEndsWithSlash=false;
}
/*
*Reset all the HTTP_RESPONSE_HEADER structure members.
*/
void resetHTTPResponse(HTTP_RESPONSE_HEADER *response)
{
	response->httpStatus=200;
	response->VER[0]='\0';	
	response->SERVER_NAME[0]='\0';
	response->CONTENTS_TYPE[0]='\0';
	response->CONNECTION[0]='\0';
	response->MIMEVER[0]='\0';
	response->P3P[0]='\0';
	response->COOKIE[0]='\0';
	response->CONTENTS_DIM[0]='\0';
	response->ERROR_TYPE[0]='\0';
	response->LOCATION[0]='\0';
	response->DATE[0]='\0';		
	response->DATEEXP[0]='\0';	
	response->isError=0;		
	response->OTHER[0]='\0';	
}


/*
*Builds an HTTP header string starting from an HTTP_RESPONSE_HEADER structure.
*/
void buildHTTPResponseHeader(char *str,HTTP_RESPONSE_HEADER* response)
{
	/*
	*Here is builded the HEADER of a HTTP response.
	*Passing a HTTP_RESPONSE_HEADER struct this builds an header string.
	*Every directive ends with a \r\n sequence.
    */
	if(response->isError)
		sprintf(str,"HTTP/%s %s\r\nStatus: \r\n",response->VER,response->ERROR_TYPE,response->ERROR_TYPE);
	else
		sprintf(str,"HTTP/%s 200 OK\r\n",response->VER);

	if(response->CONTENTS_DIM[0])
	{
		lstrcat(str,"Content-Length:");
		lstrcat(str,response->CONTENTS_DIM);
		lstrcat(str,"\r\n");
	}
	if(response->SERVER_NAME[0])
	{
		lstrcat(str,"Server:");
		lstrcat(str,response->SERVER_NAME);
		lstrcat(str,"\r\n");
	}
	if(response->CONNECTION[0])
	{
		lstrcat(str,"Connection:");
		lstrcat(str,response->CONNECTION);
		lstrcat(str,"\r\n");
	}
	if(response->COOKIE[0])
	{
		lstrcat(str,"Set-Cookie:");
		lstrcat(str,response->COOKIE);
		lstrcat(str,"\r\n");
	}
	if(response->P3P[0])
	{
		lstrcat(str,"P3P:");
		lstrcat(str,response->P3P);
		lstrcat(str,"\r\n");
	}
	if(response->SERVER_NAME[0])
	{
		lstrcat(str,"Server:");
		lstrcat(str,response->SERVER_NAME);
		lstrcat(str,"\r\n");
	}
	if(response->MIMEVER[0])
	{
		lstrcat(str,"MIME-Version:");
		lstrcat(str,response->MIMEVER);
		lstrcat(str,"\r\n");
	}
	if(response->CONTENTS_TYPE[0])
	{
		lstrcat(str,"Content-Type:");
		lstrcat(str,response->CONTENTS_TYPE);
		lstrcat(str,"\r\n");
	}
	if(response->DATE[0])
	{
		lstrcat(str,"Date:");
		lstrcat(str,response->DATE);
		lstrcat(str,"\r\n");
	}
	if(response->DATEEXP[0])
	{
		lstrcat(str,"Expires:");
		lstrcat(str,response->DATEEXP);
		lstrcat(str,"\r\n");
	}
	if(response->LOCATION[0])
	{
		lstrcat(str,"Location:");
		lstrcat(str,response->LOCATION);
		lstrcat(str,"\r\n");
	}
	if(response->OTHER[0])
	{
		lstrcat(str,response->OTHER);
		lstrcat(str,"\r\n");
	}

	/*
	*myServer supports the bytes range.
	*/
	lstrcat(str,"Accept-Ranges: bytes\r\n");
	/*
	*The HTTP header ends with a \r\n sequence.
	*/
	lstrcat(str,"\r\n");

}
/*
*Set the defaults value for a HTTP_RESPONSE_HEADER structure.
*/
void buildDefaultHTTPResponseHeader(HTTP_RESPONSE_HEADER* response)
{
	resetHTTPResponse(response);
	/*
	*By default use:
	*1) the MIME type of the page equal to text/html.
	*2) the version of the HTTP protocol to 1.1.
	*3) the date of the page and the expiration date to the current time.
	*4) then set the name of the server.
	*5) set the page that it is not an error page.
	*/
	lstrcpy(response->CONTENTS_TYPE,"text/html");
	lstrcpy(response->VER,"1.1");
	response->isError=false;
	lstrcpy(response->DATE,getRFC822GMTTime());
	lstrcpy(response->DATEEXP,getRFC822GMTTime());
	sprintf(response->SERVER_NAME,"MyServer %s",versionOfSoftware);
}
/*
*Sends an error page to the client described by the connection.
*/
int raiseHTTPError(httpThreadContext* td,LPCONNECTION a,int ID)
{
	if(ID==e_401AUTH)
	{
		sprintf(td->buffer2,"HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic\r\nServer: %s\r\nDate: %s\r\nContent-type: text/html\r\nContent-length: 0\r\n\r\n",lserver->getServerName(),getRFC822GMTTime());
		ms_send(a->socket,td->buffer2,lstrlen(td->buffer2),0);
		return 1;
	}
	if(lserver->mustUseMessagesFiles())
	{
		 return sendHTTPRESOURCE(td,a,HTTP_ERROR_HTMLS[ID],true);
		
	}
	buildDefaultHTTPResponseHeader(&(td->response));
	/*
	*Set the isError member to true to build an error page.
	*/
	td->response.isError=true;
	lstrcpy(td->response.ERROR_TYPE,HTTP_ERROR_MSGS[ID]);
	sprintf(td->response.CONTENTS_DIM,"%i",lstrlen(HTTP_ERROR_MSGS[ID]));
	buildHTTPResponseHeader(td->buffer,&td->response);
	lstrcat(td->buffer,HTTP_ERROR_MSGS[ID]);
	ms_send(a->socket,td->buffer,lstrlen(td->buffer), 0);
	return 1;
}

/*
*Returns the MIME type passing its extension.
*/
int getMIME(char *MIME,char *filename,char *dest,char *dest2)
{
	getFileExt(dest,filename);
	/*
	*Returns true if file is registered by a CGI.
	*/
	return lserver->mimeManager.getMIME(dest,MIME,dest2);
}
/*
*Map an URL to the machine file system.
*/
void getPath(char *filenamePath,const char *filename,int systemrequest)
{
	/*
	*If it is a system request, search the file in the system folder.
	*/
	if(systemrequest)
	{
		sprintf(filenamePath,"%s/%s",lserver->getSystemPath(),filename);
	}
	/*
	*Else the file is in the web folder.
	*/
	else
	{
		sprintf(filenamePath,"%s/%s",lserver->getPath(),filename);
	}
}

/*
*Controls if the req string is a valid HTTP request header.
*Returns 0 if req is an invalid header, a non-zero value if is a valid header.
*nLinesptr is a value of the lines number in the HEADER.
*ncharsptr is a value of the characters number in the HEADER.
*/
u_long validHTTPRequest(httpThreadContext* td,u_long* nLinesptr,u_long* ncharsptr)
{
	u_long i;
	char *req=td->buffer;
	u_long buffersize=td->buffersize;
	u_long nLinechars;
	int isValidCommand=false;
	nLinechars=0;
	u_long nLines=0;
	u_long maxTotchars=0;
	if(req==0)
		return 0;
	/*
	*Count the number of lines in the header.
	*/
	for(nLines=i=0;;i++)
	{
		if(req[i]=='\n')
		{
			if(req[i+2]=='\n')
			{
				maxTotchars=i+3;
				if(maxTotchars>buffersize)
				{
					isValidCommand=false;
					break;				
				}
				isValidCommand=true;
				break;
			}
			nLines++;
		}
		else
			nLinechars++;
		/*
		*We set a maximal theorical number of characters in a line to 1024.
		*If a line contains more than 1024 lines we consider the header invalid.
		*/
		if(nLinechars>1024)
			break;
	}

	/*
	*Set the output variables.
	*/
	*nLinesptr=nLines;
	*ncharsptr=maxTotchars;
	
	/*
	*Return if is a valid request header.
	*/
    return((isValidCommand)?1:0);
}


/*
*Controls if the req string is a valid HTTP response header.
*Returns 0 if req is an invalid header, a non-zero value if is a valid header.
*nLinesptr is a value of the lines number in the HEADER.
*ncharsptr is a value of the characters number in the HEADER.
*/
u_long validHTTPResponse(httpThreadContext* td,u_long* nLinesptr,u_long* ncharsptr)
{
	u_long i;
	char *req=td->buffer;
	u_long buffersize=td->buffersize;
	u_long nLinechars;
	int isValidCommand=false;
	nLinechars=0;
	u_long nLines=0;
	u_long maxTotchars=0;
	if(req==0)
		return 0;
	/*
	*Count the number of lines in the header.
	*/
	for(nLines=i=0;;i++)
	{
		if(req[i]=='\n')
		{
			if((req[i+2]=='\n')|(req[i+1]=='\0'))
			{
				maxTotchars=i+3;
				if(maxTotchars>buffersize)
				{
					isValidCommand=false;
					break;				
				}
				isValidCommand=true;
				break;
			}
			nLines++;
		}
		else
			nLinechars++;
		/*
		*We set a maximal theorical number of characters in a line to 1024.
		*If a line contains more than 1024 lines we consider the header invalid.
		*/
		if(nLinechars>1024)
			break;
	}

	/*
	*Set the output variables.
	*/
	*nLinesptr=nLines;
	*ncharsptr=maxTotchars;
	
	/*
	*Return if is a valid request header.
	*/
    return((isValidCommand)?1:0);
}

/*
*Send a redirect message to the client.
*/
int sendHTTPRedirect(httpThreadContext* td,LPCONNECTION a,char *newURL)
{
	sprintf(td->buffer2,"HTTP/1.1 301 Moved\r\nWWW-Authenticate: Basic\r\nServer: %s\r\nDate: %s\r\nContent-type: text/html\r\nLocation: %s\r\nContent-length: 0\r\n\r\n",lserver->getServerName(),getRFC822GMTTime(),newURL);
	ms_send(a->socket,td->buffer2,lstrlen(td->buffer2),0);
	return 1;
}
/*
*Send a non-modified message to the client.
*/
int sendHTTPNonModified(httpThreadContext* td,LPCONNECTION a)
{
	sprintf(td->buffer2,"HTTP/1.1 304 Not Modified\r\nWWW-Authenticate: Basic\r\nServer: %s\r\nDate: %s\r\nContent-type: text/html\r\nContent-length: 0\r\n\r\n",lserver->getServerName(),getRFC822GMTTime());
	ms_send(a->socket,td->buffer2,lstrlen(td->buffer2),0);
	return 1;
}
/*
*Build the HTTP REQUEST HEADER string.
*If no input is setted the input is the main buffer of the httpThreadContext structure.
*/
int buildHTTPRequestHeaderStruct(httpThreadContext* td,char *input)
{
	/*
	*In this function there is the HTTP protocol parse.
	*The request is mapped into a HTTP_REQUEST_HEADER structure
	*And at the end of this every command is treated
	*differently. We use this mode for parse the HTTP
	*because especially in the CGI is requested a continous
	*HTTP header access.
	*Before mapping the header in the structure 
	*control if this is a regular request.
	*The HTTP header ends with a \r\n\r\n sequence.
	*/

	/*
	*Control if the HTTP header is a valid header.
	*/
	u_long i=0,j=0,max=0;
	u_long nLines,maxTotchars;
	u_long validRequest=validHTTPRequest(td,&nLines,&maxTotchars);

	const int max_URI=MAX_PATH+200;
	const char seps[]   = " ,\t\n\r";
	const char cmdseps[]   = ": ,\t\n\r";

	static char *token=0;
	static char command[96];

	static int nLineControlled;
	nLineControlled=0;
	static int lineControlled;
	if(input)
		token=input;
	else
		token=td->buffer;

	token = strtok( token, cmdseps );
	do
	{
		/*
		*Reset the flag lineControlled.
		*/
		lineControlled=false;

		/*
		*Copy the HTTP command.
		*/
		lstrcpy(command,token);
		
		
		nLineControlled++;
		if(nLineControlled==1)
		{
			/*
			*The first line has the form:
			*GET /index.html HTTP/1.1
			*/
			lineControlled=true;
			/*
			*Copy the method type.
			*/
			lstrcpy(td->request.CMD,command);
		
			token = strtok( NULL, " ,\t\n\r" );
			max=lstrlen(token);
			int containOpts=false;
			for(i=0;i<max;i++)
			{
				if(token[i]=='?')
				{
					containOpts=true;
					break;
				}
				td->request.URI[i]=token[i];
			}
			td->request.URI[i]='\0';

			if(containOpts)
			{
				for(j=0;i<max;j++)
				{
					td->request.URIOPTS[j]=token[++i];
				}
			}
			token = strtok( NULL, seps );
			lstrcpy(td->request.VER,token);
			/*
			*Version of the protocol in the HTTP_REQUEST_HEADER
			*struct is leaved as a number.
			*For example HTTP/1.1 in the struct is 1.1
			*/
			StrTrim(td->request.VER,"HTTP /");
			if(td->request.URI[lstrlen(td->request.URI)-1]=='/')
				td->request.uriEndsWithSlash=true;
			else
				td->request.uriEndsWithSlash=false;
			StrTrim(td->request.URI," /");
			StrTrim(td->request.URIOPTS," /");
			max=lstrlen(td->request.URI);
			if(max>max_URI)
			{
				return 414;
			}
		}else
		/*User-Agent*/
		if(!lstrcmpi(command,"User-Agent"))
		{
			token = strtok( NULL, "\r\n" );
			lineControlled=true;
			lstrcpy(td->request.USER_AGENT,token);
			StrTrim(td->request.USER_AGENT," ");
		}else
		/*Authorization*/
		if(!lstrcmpi(command,"Authorization"))
		{
			token = strtok( NULL, "\r\n" );
			lineControlled=true;		
			td->buffer2[0]='\0';
			/*
			*Basic authorization in base64 is login:password.
			*Assume that it is Basic anyway.
			*/
			lstrcpy(td->request.AUTH,"Basic");
			int len=lstrlen(token);
			char *base64=base64Utils.Decode(&token[lstrlen("Basic:")],&len);
			char* lbuffer2=base64;
			while(*lbuffer2!=':')
			{
				td->connection->login[lstrlen(td->connection->login)]=*lbuffer2++;
			}
			lbuffer2++;
			while(*lbuffer2)
			{
				td->connection->password[lstrlen(td->connection->password)]=*lbuffer2++;
			}
			free(base64);
		}else
		/*Host*/
		if(!lstrcmpi(command,"Host"))
		{
			token = strtok( NULL, seps );
			lineControlled=true;
			lstrcpy(td->request.HOST,token);
			StrTrim(td->request.HOST," ");
		}else
		/*Accept*/
		if(!lstrcmpi(command,"Accept"))
		{
			token = strtok( NULL, "\n\r" );
			lineControlled=true;
			lstrcat(td->request.ACCEPT,token);
			StrTrim(td->request.ACCEPT," ");
		}else
		/*Accept-Language*/
		if(!lstrcmpi(command,"Accept-Language"))
		{
			token = strtok( NULL, "\n\r" );
			lineControlled=true;
			lstrcpy(td->request.ACCEPTLAN,token);
			StrTrim(td->request.ACCEPTLAN," ");
		}else
		/*Accept-charset*/
		if(!lstrcmpi(command,"Accept-charset"))
		{
			token = strtok( NULL, "\n\r" );
			lineControlled=true;
			lstrcpy(td->request.ACCEPTCHARSET,token);
			StrTrim(td->request.ACCEPTCHARSET," ");
		}else
		/*Accept-Encoding*/
		if(!lstrcmpi(command,"Accept-Encoding"))
		{
			token = strtok( NULL, "\n\r" );
			lineControlled=true;
			lstrcpy(td->request.ACCEPTENC,token);
			StrTrim(td->request.ACCEPTENC," ");
		}else
		/*Connection*/
		if(!lstrcmpi(command,"Connection"))
		{
			token = strtok( NULL, "\n\r" );
			lineControlled=true;
			lstrcpy(td->request.CONNECTION,token);
			StrTrim(td->request.CONNECTION," ");
		}else
		/*Cookie*/
		if(!lstrcmpi(command,"Cookie"))
		{
			token = strtok( NULL, "\n\r" );
			lineControlled=true;
			lstrcpy(td->request.COOKIE,token);
			StrTrim(td->request.COOKIE," ");
		}else
		/*From*/
		if(!lstrcmpi(command,"From"))
		{
			token = strtok( NULL, "\r\n" );
			lineControlled=true;
			lstrcpy(td->request.FROM,token);
			StrTrim(td->request.FROM," ");
		}else
		/*Connection*/
		if(!lstrcmpi(command,"Content-Length"))
		{
			token = strtok( NULL, "\n\r" );
			lineControlled=true;
			lstrcpy(td->request.CONTENTS_DIM,token);
		}else
		/*Range*/
		if(!lstrcmpi(command,"Range"))
		{
			td->request.RANGETYPE[0]='\0';
			td->request.RANGEBYTEBEGIN[0]='\0';
			td->request.RANGEBYTEEND[0]='\0';
			lineControlled=true;
			token = strtok( NULL, "\r\n\t" );
			do
			{
				td->request.RANGETYPE[lstrlen(td->request.RANGETYPE)]=*token;
			}
			while(*token++ != '=');
			do
			{
				td->request.RANGEBYTEBEGIN[lstrlen(td->request.RANGEBYTEBEGIN)]=*token;
			}
			while(*token++ != '-');
			do
			{
				td->request.RANGEBYTEEND[lstrlen(td->request.RANGEBYTEEND)]=*token;
			}
			while(*token++);
			StrTrim(td->request.RANGETYPE,"= ");
			StrTrim(td->request.RANGEBYTEBEGIN,"- ");
			StrTrim(td->request.RANGEBYTEEND,"- ");
			
			if(td->request.RANGEBYTEBEGIN[0]==0)
				lstrcpy(td->request.RANGEBYTEBEGIN,"0");
			if(td->request.RANGEBYTEEND[0]==0)
				lstrcpy(td->request.RANGEBYTEEND,"-1");

		}else
		/*Referer*/
		if(!lstrcmpi(command,"Referer"))
		{
			token = strtok( NULL, seps );
			lineControlled=true;
			lstrcpy(td->request.REFERER,token);
			StrTrim(td->request.REFERER," ");
		}else
		/*Pragma*/
		if(!lstrcmpi(command,"Pragma"))
		{
			token = strtok( NULL, seps );
			lineControlled=true;
			lstrcpy(td->request.PRAGMA,token);
			StrTrim(td->request.PRAGMA," ");
		}
		
		/*
		*If the line is not controlled arrive with the token
		*at the end of the line.
		*/
		if(!lineControlled)
		{
			token = strtok( NULL, "\n" );
		}
		token = strtok( NULL, cmdseps );
	}while((u_long)(token-td->buffer)<maxTotchars);
	/*
	*END REQUEST STRUCTURE BUILD.
	*/
	td->nHeaderChars=maxTotchars;
	return validRequest;
}


/*
*Build the HTTP RESPONSE HEADER string.
*If no input is setted the input is the main buffer of the httpThreadContext structure.
*/
int buildHTTPResponseHeaderStruct(httpThreadContext* td,char *input)
{
	/*
	*In this function there is the HTTP protocol parse.
	*The request is mapped into a HTTP_REQUEST_HEADER structure
	*And at the end of this every command is treated
	*differently. We use this mode for parse the HTTP
	*because especially in the CGI is requested a continous
	*HTTP header access.
	*Before mapping the header in the structure 
	*control if this is a regular request.
	*The HTTP header ends with a \r\n\r\n sequence.
	*/

	/*
	*Control if the HTTP header is a valid header.
	*/
	u_long nLines,maxTotchars;
	u_long validRequest=validHTTPResponse(td,&nLines,&maxTotchars);

	const char seps[]   = " ,\t\n\r";
	const char cmdseps[]   = ": ,\t\n\r";

	static char *token=0;
	static char command[96];

	static int nLineControlled;
	nLineControlled=0;
	static int lineControlled;
	if(input)
		token=input;
	else
		token=td->buffer;

	token = strtok( token, cmdseps );
	do
	{
		/*
		*Reset the flag lineControlled.
		*/
		lineControlled=false;

		/*
		*Copy the HTTP command.
		*/
		lstrcpy(command,token);
		
		
		nLineControlled++;
		if((nLineControlled==1)&&(token[0]=='H')&&(token[1]=='T')&&(token[2]=='T')&&(token[3]=='P'))
		{
			/*
			*The first line has the form:
			*GET /index.html HTTP/1.1
			*/
			lineControlled=true;
			/*
			*Copy the HTTP version, do not include "HTTP/".
			*/
			lstrcpy(td->response.VER,&command[5]);
		
			token = strtok( NULL, " ,\t\n\r" );
			td->response.httpStatus=atoi(token);
			
			token = strtok( NULL, seps );
			strcpy(td->response.ERROR_TYPE,token);

		}else
		/*Server*/
		if(!lstrcmpi(command,"Server"))
		{
			token = strtok( NULL, "\r\n" );
			lineControlled=true;
			lstrcpy(td->response.SERVER_NAME,token);
			StrTrim(td->response.SERVER_NAME," ");
		}else
		/*Location*/
		if(!lstrcmpi(command,"Location"))
		{
			token = strtok( NULL, "\r\n" );
			lineControlled=true;
			lstrcpy(td->response.LOCATION,token);
			StrTrim(td->response.LOCATION," ");
		}else
		/*Date*/
		if(!lstrcmpi(command,"Date"))
		{
			token = strtok( NULL, "\r\n" );
			lineControlled=true;
			lstrcpy(td->response.DATE,token);
			StrTrim(td->response.DATE," ");
		}else
		/*Content-Type*/
		if(!lstrcmpi(command,"Content-Type"))
		{
			token = strtok( NULL, "\r\n" );
			lineControlled=true;
			lstrcpy(td->response.CONTENTS_TYPE,token);
			StrTrim(td->response.CONTENTS_TYPE," ");
		}else
		/*MIME-Version*/
		if(!lstrcmpi(command,"MIME-Version"))
		{
			token = strtok( NULL, "\r\n" );
			lineControlled=true;
			lstrcpy(td->response.MIMEVER,token);
			StrTrim(td->response.MIMEVER," ");
		}else
		/*Set-Cookie*/
		if(!lstrcmpi(command,"Set-Cookie"))
		{
			token = strtok( NULL, "\r\n" );
			lineControlled=true;
			/*Concatenate string at the end cause can be more that one cookie line.*/
			lstrcat(td->response.COOKIE,token);
			StrTrim(td->response.COOKIE," ");
		}else
		/*Content-Length*/
		if(!lstrcmpi(command,"Content-Length"))
		{
			token = strtok( NULL, "\r\n" );
			lineControlled=true;
			lstrcpy(td->response.CONTENTS_DIM,token);
			StrTrim(td->response.CONTENTS_DIM," ");
		}else
		/*P3P*/
		if(!lstrcmpi(command,"P3P"))
		{
			token = strtok( NULL, "\r\n" );
			lineControlled=true;
			lstrcpy(td->response.P3P,token);
			StrTrim(td->response.P3P," ");
		}else
		/*Connection*/
		if(!lstrcmpi(command,"Connection"))
		{
			token = strtok( NULL, "\r\n" );
			lineControlled=true;
			lstrcpy(td->response.CONNECTION,token);
			StrTrim(td->response.CONNECTION," ");
		}else
		/*Expires*/
		if(!lstrcmpi(command,"Expires"))
		{
			token = strtok( NULL, "\r\n" );
			lineControlled=true;
			lstrcpy(td->response.DATEEXP,token);
			StrTrim(td->response.DATEEXP," ");
		}
		/*
		*If the line is not controlled arrive with the token
		*at the end of the line.
		*/
		if(!lineControlled)
		{
			token = strtok( NULL, "\n" );
		}
		token = strtok( NULL, cmdseps );
	}while((u_long)(token-td->buffer)<maxTotchars);
	/*
	*END REQUEST STRUCTURE BUILD.
	*/
	td->nBytesToRead=maxTotchars;
	return validRequest;
}
