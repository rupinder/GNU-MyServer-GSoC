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
#include "../include/winCGI.h"
#include "../include/utility.h"
#include "../include/isapi.h"
#include "../include/stringutils.h"
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
		((vhost*)td->connection->host)->ms_warningsLogWrite(msg);
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
	{
		/*
		*If the client try to access files that aren't in the web folder send a 401 error.
		*/
		if((filename[0] != '\0')&&(ms_getPathRecursionLevel(filename)<1))
		{
			return raiseHTTPError(td,s,e_401);
		}
		translateEscapeString(filename );
		getPath(td,td->filenamePath,filename,systemrequest);
	}
	/*
	*Get the PATH_INFO value.
	*Use dirscan as a buffer for put temporary directory scan.
	*When an '/' character is present check if the path up to '/' character
	*is a file. If it is a file send the rest of the URI as PATH_INFO.
	*/
	char dirscan[MAX_PATH];
	dirscan[0]='\0';
	td->pathInfo[0]='\0';
	td->pathTranslated[0]='\0';
	int filenamePathLen=strlen(td->filenamePath);
	for(int i=0,len=0;i<filenamePathLen;i++)
	{
		/*
		*http://127.0.0.1/uri/filetosend.php/PATH_INFO_VALUE?QUERY_INFO_VALUE
		*When a request has this form send the file filetosend.php with the
		*environment string PATH_INFO equals to PATH_INFO_VALUE and QUERY_INFO
		*to QUERY_INFO_VALUE.
		*/
		if(i&& (td->filenamePath[i]=='/'))/*There is the '/' character check if dirscan is a file*/
		{
			if(!ms_IsFolder(dirscan))
			{
				/*
				*If the token is a file.
				*/
				strcpy(td->pathInfo,&td->filenamePath[len]);
				strcpy(td->filenamePath,dirscan);
				break;
			}
		}
		dirscan[len++]=(td->filenamePath)[i];
		dirscan[len]='\0';
	}
	
	/*
	*If there is a PATH_INFO value the get the PATH_TRANSLATED too.
	*PATH_TRANSLATED is the mapped to the local filesystem version of PATH_INFO.
	*/
	if((td->pathInfo[0])&&(td->pathInfo[0]!='/'))
	{
        td->pathTranslated[0]='\0';
		/*
		*Start from the second character cause the first is a slash character.
		*/
		getPath(td,(td->pathTranslated),&((td->pathInfo)[1]),false);
	}
	else
	{
        td->pathTranslated[0]='\0';
	}
	/*
	*If there are not any extension then we do one of this in order:
	1)We send the default files in the folder in order.
	2)We send the folder content.
	3)We send an error.
	*/
	if(ms_IsFolder((char *)(td->filenamePath)))
	{
		int i;
		for(i=0;;i++)
		{
			static char defaultFileName[MAX_PATH];
			char *defaultFileNamePath=lserver->getDefaultFilenamePath(i);
			if(defaultFileNamePath)
				sprintf(defaultFileName,"%s/%s",td->filenamePath,defaultFileNamePath);
			else
				break;
			if(ms_FileExists(defaultFileName))
			{
				/*
				*Change the URI to reflect the default file name.
				*/
				strcat(td->request.URI,"/");
				strcat(td->request.URI,defaultFileNamePath);
				strcpy(td->filenamePath,defaultFileName);
				if(sendHTTPRESOURCE(td,s,defaultFileName,0,0,0,-1,1))
					return 1;
			}
		}

		if(sendHTTPDIRECTORY(td,s,td->filenamePath))
			return 1;	

		return raiseHTTPError(td,s,e_404);
	}

	if(!ms_FileExists(td->filenamePath))
		raiseHTTPError(td,s,e_404);

	/*
	*getMIME returns true if the ext is registered by a CGI.
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
		if(lserver->mscgiLoaded)
		{
			if(sendMSCGI(td,s,td->filenamePath,target))
				return 1;
			return raiseHTTPError(td,s,e_404);
		}
		return raiseHTTPError(td,s,e_500);
	}else if(mimeCMD==CGI_CMD_WINCGI)
	{
	
		return sendWINCGI(td,s,td->filenamePath);
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
	time_t lastMT=ms_GetLastModTime(td->filenamePath);
	getRFC822GMTTime(lastMT,td->response.LAST_MODIFIED,HTTP_RESPONSE_LAST_MODIFIED_DIM);
	if(td->request.IF_MODIFIED_SINCE[0])
	{
		time_t timeMS=getTime(td->request.IF_MODIFIED_SINCE);
		if(timeMS<lastMT)
			return sendHTTPNonModified(td,s);
	}

	if(sendHTTPFILE(td,s,td->filenamePath,OnlyHeader,firstByte,lastByte))
		return 1;
	
	return raiseHTTPError(td,s,e_500);
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
	td.outputData =(MYSERVER_FILE_HANDLE)0;

	/*
	*Reset the request structure.
	*/
	resetHTTPRequest(&td.request);

	u_long validRequest=buildHTTPRequestHeaderStruct(&td.request,&td);


	/*
	*If the server verbosity is > 4 then save the HTTP request header.
	*/
	if(lserver->getVerbosity()>4)
	{
		td.buffer[td.nBytesToRead]='\n';
		td.buffer[td.nBytesToRead+1]='\0';
		((vhost*)td.connection->host)->ms_warningsLogWrite(td.buffer);
	}
	/*
	*If the header is an invalid request send the correct error message to the client and return immediately.
	*/
	if(validRequest==0)
	{
		return raiseHTTPError(&td,a,e_400);
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
		ms_getdefaultwd(td.inputDataPath,MAX_PATH);
		sprintf(&td.inputDataPath[lstrlen(td.inputDataPath)],"/stdInFile_%u",td.id);
		td.inputData=ms_CreateTemporaryFile(td.inputDataPath);
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
		((vhost*)(td.connection->host))->ms_accessesLogWrite(a->ipAddr);
		((vhost*)(td.connection->host))->ms_accessesLogWrite(":");
		((vhost*)(td.connection->host))->ms_accessesLogWrite(td.request.CMD);
		((vhost*)(td.connection->host))->ms_accessesLogWrite(" ");
		((vhost*)(td.connection->host))->ms_accessesLogWrite(td.request.URI);
		if(td.request.URIOPTS[0])
		{
			((vhost*)(td.connection->host))->ms_accessesLogWrite("?");
			((vhost*)(td.connection->host))->ms_accessesLogWrite(td.request.URIOPTS);
		}
		((vhost*)(td.connection->host))->ms_accessesLogWrite("\r\n");
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
			return 0;
		}
		else
		{
			/*
			*Find the virtual host to check both host name and IP value.
			*/
			a->host=lserver->vhostList.getvHost(td.request.HOST,a->localIpAddr,a->localPort);
			if(a->host==0)
			{
				if(td.inputData)
				{
					ms_CloseFile(td.inputData);
					td.inputData=0;
				}
				return 0;
			}
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
	/*
	*If the outputData file was not closed close it.
	*/
	if(td.inputData)
	{
		ms_CloseFile(td.outputData);
		td.outputData=0;
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
	request->IF_MODIFIED_SINCE[0]='\0';
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
	response->AUTH[0]='\0';
	response->DATEEXP[0]='\0';	
	response->OTHER[0]='\0';
	response->LAST_MODIFIED[0]='\0';
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
	if(response->httpStatus!=200)
		sprintf(str,"HTTP/%s %i %s\r\nStatus: %s\r\n",response->VER,response->httpStatus,response->ERROR_TYPE,response->ERROR_TYPE);
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
	if(response->LAST_MODIFIED[0])
	{
		lstrcat(str,"Last-Modified:");
		lstrcat(str,response->LAST_MODIFIED);
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
	if(response->AUTH[0])
	{
		lstrcat(str,"WWW-Authenticate:");
		lstrcat(str,response->AUTH);
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
	*3) the date of the page and the expire date to the current time.
	*4) set the name of the server.
	*5) set the page that it is not an error page.
	*/
	lstrcpy(response->CONTENTS_TYPE,"text/html");
	lstrcpy(response->VER,"1.1");
	response->httpStatus=200;
	getRFC822GMTTime(response->DATE,HTTP_RESPONSE_DATE_DIM);
	strncpy(response->DATEEXP,response->DATE,HTTP_RESPONSE_DATEEXP_DIM);
	sprintf(response->SERVER_NAME,"MyServer %s",versionOfSoftware);
}
/*
*Sends an error page to the client.
*/
int raiseHTTPError(httpThreadContext* td,LPCONNECTION a,int ID)
{
	if(ID==e_401AUTH)
	{
		sprintf(td->buffer2,"HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic\r\nServer: %s\r\nContent-type: text/html\r\nContent-length: 0\r\n",lserver->getServerName());
		strcat(td->buffer2,"Date: ");
		getRFC822GMTTime(&td->buffer2[strlen(td->buffer2)],HTTP_RESPONSE_DATE_DIM);
		strcat(td->buffer2,"\r\n\r\n");
		ms_send(a->socket,td->buffer2,lstrlen(td->buffer2),0);
		return 1;
	}
	td->response.httpStatus=getHTTPStatusCodeFromErrorID(ID);
	lstrcpy(td->response.ERROR_TYPE,HTTP_ERROR_MSGS[ID]);
	if(lserver->mustUseMessagesFiles())
	{
		 return sendHTTPRESOURCE(td,a,HTTP_ERROR_HTMLS[ID],true);
	}
	sprintf(td->response.CONTENTS_DIM,"%i",lstrlen(HTTP_ERROR_MSGS[ID]));
	buildHTTPResponseHeader(td->buffer,&td->response);
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
void getPath(httpThreadContext* td,char *filenamePath,const char *filename,int systemrequest)
{
	/*
	*If it is a system request, search the file in the system folder.
	*/
	if(systemrequest)
	{
		sprintf(filenamePath,"%s/%s",((vhost*)(td->connection->host))->systemRoot,filename);
	}
	/*
	*Else the file is in the web folder.
	*/
	else
	{	
		if(filename[0])
			sprintf(filenamePath,"%s/%s",((vhost*)(td->connection->host))->documentRoot,filename);
		else
			sprintf(filenamePath,"%s",((vhost*)(td->connection->host))->documentRoot);

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
	for(nLines=i=0;i<KB(2) && req[i];i++)
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
		{
			isValidCommand=false;
			break;
		}
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
			if((req[i+2]=='\n')|(req[i+1]=='\0')|(req[i+1]=='\n'))
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
	sprintf(td->buffer2,"HTTP/1.1 302 Moved\r\nWWW-Authenticate: Basic\r\nServer: %s\r\nContent-type: text/html\r\nLocation: %s\r\nContent-length: 0\r\n",lserver->getServerName(),newURL);
	strcat(td->buffer2,"Date: ");
	getRFC822GMTTime(&td->buffer2[strlen(td->buffer2)],HTTP_RESPONSE_DATE_DIM);
	strcat(td->buffer2,"\r\n\r\n");

	ms_send(a->socket,td->buffer2,lstrlen(td->buffer2),0);
	return 1;
}
/*
*Send a non-modified message to the client.
*/
int sendHTTPNonModified(httpThreadContext* td,LPCONNECTION a)
{
	sprintf(td->buffer2,"HTTP/1.1 304 Not Modified\r\nWWW-Authenticate: Basic\r\nServer: %s\r\nContent-type: text/html\r\nContent-length: 0\r\n",lserver->getServerName());
	strcat(td->buffer2,"Date: ");
	getRFC822GMTTime(&td->buffer2[strlen(td->buffer2)],HTTP_RESPONSE_DATE_DIM);
	strcat(td->buffer2,"\r\n\r\n");

	ms_send(a->socket,td->buffer2,lstrlen(td->buffer2),0);
	return 1;
}
/*
*Build the HTTP REQUEST HEADER string.
*If no input is setted the input is the main buffer of the httpThreadContext structure.
*/
int buildHTTPRequestHeaderStruct(HTTP_REQUEST_HEADER *request,httpThreadContext *td,char *input)
{
	/*
	*In this function there is the HTTP protocol parse.
	*The request is mapped into a HTTP_REQUEST_HEADER structure
	*And at the end of this every command is treated
	*differently. We use this mode for parse the HTTP
	*cause especially in the CGI is requested a continous
	*HTTP header access.
	*Before mapping the header in the structure 
	*control if this is a regular request->
	*The HTTP header ends with a \r\n\r\n sequence.
	*/

	/*
	*Control if the HTTP header is a valid header.
	*/
	u_long i=0,j=0,max=0;
	u_long nLines,maxTotchars;
	u_long validRequest=validHTTPRequest(td,&nLines,&maxTotchars);
	if(!validRequest)
		return 0;

	const int max_URI=MAX_PATH+200;
	const char seps[]   = "\t\n\r";
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
			strncpy(request->CMD,command,HTTP_REQUEST_CMD_DIM);
		
			token = strtok( NULL, " ,\t\n\r" );
			max=lstrlen(token);
			int containOpts=false;
			for(i=0;(i<max)&&(i<HTTP_REQUEST_URI_DIM);i++)
			{
				if(token[i]=='?')
				{
					containOpts=true;
					break;
				}
				request->URI[i]=token[i];
			}
			request->URI[i]='\0';

			if(containOpts)
			{
				for(j=0;(i<max) && (j<HTTP_REQUEST_URIOPTS_DIM);j++)
				{
					request->URIOPTS[j]=token[++i];
				}
			}
			token = strtok( NULL, seps );
			strncpy(request->VER,token,HTTP_REQUEST_VER_DIM);
			/*
			*Version of the protocol in the HTTP_REQUEST_HEADER
			*struct is leaved as a number.
			*For example HTTP/1.1 in the struct is 1.1
			*/
			StrTrim(request->VER,"HTTP /");
			if(request->URI[strlen(request->URI)-1]=='/')
				request->uriEndsWithSlash=true;
			else
				request->uriEndsWithSlash=false;
			StrTrim(request->URI," /");
			StrTrim(request->URIOPTS," /");
			max=strlen(request->URI);
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
			strncpy(request->USER_AGENT,token,HTTP_REQUEST_USER_AGENT_DIM);
			StrTrim(request->USER_AGENT," ");
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
			lstrcpy(request->AUTH,"Basic");
			int len=lstrlen(token);
			char *base64=base64Utils.Decode(&token[lstrlen("Basic:")],&len);
			char* lbuffer2=base64;
			int i;
			for(i=0;(*lbuffer2!=':') && (i<19);i++)
			{
				td->connection->login[i]=*lbuffer2++;
				td->connection->login[i+1]='\0';
			}
			lbuffer2++;
			for(i=0;(*lbuffer2)&&(i<31);i++)
			{
				td->connection->password[i]=*lbuffer2++;
				td->connection->password[i+1]='\0';
			}
			free(base64);
		}else
		/*Host*/
		if(!lstrcmpi(command,"Host"))
		{
			token = strtok( NULL, seps );
			lineControlled=true;
			strncpy(request->HOST,token,HTTP_REQUEST_HOST_DIM);
			StrTrim(request->HOST," ");
		}else
		/*If-Modified-Since*/
		if(!lstrcmpi(command,"If-Modified-Since"))
		{
			token = strtok( NULL, seps );
			lineControlled=true;
			strncpy(request->IF_MODIFIED_SINCE,token,HTTP_REQUEST_IF_MODIFIED_SINCE_DIM);
			StrTrim(request->IF_MODIFIED_SINCE," ");
		}else
		/*Accept*/
		if(!lstrcmpi(command,"Accept"))
		{
			token = strtok( NULL, "\n\r" );
			lineControlled=true;
			strncat(request->ACCEPT,token,HTTP_REQUEST_ACCEPT_DIM-strlen(request->ACCEPT));
			StrTrim(request->ACCEPT," ");
		}else
		/*Accept-Language*/
		if(!lstrcmpi(command,"Accept-Language"))
		{
			token = strtok( NULL, "\n\r" );
			lineControlled=true;
			strncpy(request->ACCEPTLAN,token,HTTP_REQUEST_ACCEPTLAN_DIM);
			StrTrim(request->ACCEPTLAN," ");
		}else
		/*Accept-charset*/
		if(!lstrcmpi(command,"Accept-charset"))
		{
			token = strtok( NULL, "\n\r" );
			lineControlled=true;
			strncpy(request->ACCEPTCHARSET,token,HTTP_REQUEST_ACCEPTCHARSET_DIM);
			StrTrim(request->ACCEPTCHARSET," ");
		}else
		/*Accept-Encoding*/
		if(!lstrcmpi(command,"Accept-Encoding"))
		{
			token = strtok( NULL, "\n\r" );
			lineControlled=true;
			strncpy(request->ACCEPTENC,token,HTTP_REQUEST_ACCEPTENC_DIM);
			StrTrim(request->ACCEPTENC," ");
		}else
		/*Connection*/
		if(!lstrcmpi(command,"Connection"))
		{
			token = strtok( NULL, "\n\r" );
			lineControlled=true;
			strncpy(request->CONNECTION,token,HTTP_REQUEST_CONNECTION_DIM);
			StrTrim(request->CONNECTION," ");
		}else
		/*Cookie*/
		if(!lstrcmpi(command,"Cookie"))
		{
			token = strtok( NULL, "\n\r" );
			lineControlled=true;
			strncpy(request->COOKIE,token,HTTP_REQUEST_COOKIE_DIM);
			StrTrim(request->COOKIE," ");
		}else
		/*From*/
		if(!lstrcmpi(command,"From"))
		{
			token = strtok( NULL, "\r\n" );
			lineControlled=true;
			strncpy(request->FROM,token,HTTP_REQUEST_FROM_DIM);
			StrTrim(request->FROM," ");
		}else
		/*Connection*/
		if(!lstrcmpi(command,"Content-Length"))
		{
			token = strtok( NULL, "\n\r" );
			lineControlled=true;
			strncpy(request->CONTENTS_DIM,token,HTTP_REQUEST_CONTENTS_DIM_DIM);
		}else
		/*Range*/
		if(!lstrcmpi(command,"Range"))
		{
			request->RANGETYPE[0]='\0';
			request->RANGEBYTEBEGIN[0]='\0';
			request->RANGEBYTEEND[0]='\0';
			lineControlled=true;
			token = strtok( NULL, "\r\n\t" );
			int i=0;
			do
			{
				request->RANGETYPE[i++]=*token;
				request->RANGETYPE[i]='\0';
			}
			while((*token++ != '=')&&(i<HTTP_REQUEST_RANGETYPE_DIM));
			i=0;
			do
			{
				request->RANGEBYTEBEGIN[i++]=*token;
				request->RANGEBYTEBEGIN[i]='\0';
			}
			while((*token++ != '-')&&(i<HTTP_REQUEST_RANGEBYTEBEGIN_DIM));
			i=0;
			do
			{
				request->RANGEBYTEEND[i++]=*token;
				request->RANGEBYTEEND[i]='\0';
			}
			while((*token++)&&(i<HTTP_REQUEST_RANGEBYTEEND_DIM));
			StrTrim(request->RANGETYPE,"= ");
			StrTrim(request->RANGEBYTEBEGIN,"- ");
			StrTrim(request->RANGEBYTEEND,"- ");
			
			if(request->RANGEBYTEBEGIN[0]==0)
				strcpy(request->RANGEBYTEBEGIN,"0");
			if(request->RANGEBYTEEND[0]==0)
				strcpy(request->RANGEBYTEEND,"-1");

		}else
		/*Referer*/
		if(!lstrcmpi(command,"Referer"))
		{
			token = strtok( NULL, seps );
			lineControlled=true;
			strncpy(request->REFERER,token,HTTP_REQUEST_REFERER_DIM);
			StrTrim(request->REFERER," ");
		}else
		/*Pragma*/
		if(!lstrcmpi(command,"Pragma"))
		{
			token = strtok( NULL, seps );
			lineControlled=true;
			strncpy(request->PRAGMA,token,HTTP_REQUEST_PRAGMA_DIM);
			StrTrim(request->PRAGMA," ");
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
int buildHTTPResponseHeaderStruct(HTTP_RESPONSE_HEADER *response,httpThreadContext *td,char *input)
{
	/*
	*In this function there is the HTTP protocol parse.
	*The request is mapped into a HTTP_REQUEST_HEADER structure
	*And at the end of this every command is treated
	*differently. We use this mode for parse the HTTP
	*cause especially in the CGI is requested a continous
	*HTTP header access.
	*Before mapping the header in the structure 
	*control if this is a regular request.
	*The HTTP header ends with a \r\n\r\n sequence.
	*/

	/*
	*Control if the HTTP header is a valid header.
	*/
	if((input==0) || (strlen(input)==0))
		return 0;
	u_long nLines,maxTotchars;
	u_long validRequest=validHTTPResponse(td,&nLines,&maxTotchars);

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
		if(!token)
			break;
		/*
		*Reset the flag lineControlled.
		*/
		lineControlled=false;

		/*
		*Copy the HTTP command.
		*/
		strcpy(command,token);
		
		
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
			strncpy(response->VER,&command[5],HTTP_RESPONSE_VER_DIM);
		
			token = strtok( NULL, " ,\t\n\r" );
			response->httpStatus=atoi(token);
			
			token = strtok( NULL, "\r\n" );
			strncpy(response->ERROR_TYPE,token,HTTP_RESPONSE_ERROR_TYPE_DIM);

		}else
		/*Server*/
		if(!lstrcmpi(command,"Server"))
		{
			token = strtok( NULL, "\r\n" );
			lineControlled=true;
			strncpy(response->SERVER_NAME,token,HTTP_RESPONSE_SERVER_NAME_DIM);
			StrTrim(response->SERVER_NAME," ");
		}else
		/*Location*/
		if(!lstrcmpi(command,"Location"))
		{
			token = strtok( NULL, "\r\n" );
			lineControlled=true;
			strncpy(response->LOCATION,token,HTTP_RESPONSE_LOCATION_DIM);
			StrTrim(response->LOCATION," ");
		}else
		/*Last-Modified*/
		if(!lstrcmpi(command,"Last-Modified"))
		{
			token = strtok( NULL, "\r\n" );
			lineControlled=true;
			strncpy(response->LAST_MODIFIED,token,HTTP_RESPONSE_LAST_MODIFIED_DIM);
			StrTrim(response->LAST_MODIFIED," ");
		}else
		/*Status*/
		if(!lstrcmpi(command,"Status"))
		{
			token = strtok( NULL, "\r\n" );
			lineControlled=true;
			response->httpStatus=atoi(token);
		}else
		/*Date*/
		if(!lstrcmpi(command,"Date"))
		{
			token = strtok( NULL, "\r\n" );
			lineControlled=true;
			strncpy(response->DATE,token,HTTP_RESPONSE_DATE_DIM);
			StrTrim(response->DATE," ");
		}else
		/*Content-Type*/
		if(!lstrcmpi(command,"Content-Type"))
		{
			token = strtok( NULL, "\r\n" );
			lineControlled=true;
			strncpy(response->CONTENTS_TYPE,token,HTTP_RESPONSE_CONTENTS_TYPE_DIM);
			StrTrim(response->CONTENTS_TYPE," ");
		}else
		/*MIME-Version*/
		if(!lstrcmpi(command,"MIME-Version"))
		{
			token = strtok( NULL, "\r\n" );
			lineControlled=true;
			strncpy(response->MIMEVER,token,HTTP_RESPONSE_MIMEVER_DIM);
			StrTrim(response->MIMEVER," ");
		}else
		/*Set-Cookie*/
		if(!lstrcmpi(command,"Set-Cookie"))
		{
			token = strtok( NULL, "\r\n" );
			lineControlled=true;
			/*Concatenate string at the end cause can be more than one cookie line.*/
			strncat(response->COOKIE,token,HTTP_RESPONSE_COOKIE_DIM-strlen(response->COOKIE));
			StrTrim(response->COOKIE," ");
		}else
		/*Content-Length*/
		if(!lstrcmpi(command,"Content-Length"))
		{
			token = strtok( NULL, "\r\n" );
			lineControlled=true;
			strncpy(response->CONTENTS_DIM,token,HTTP_RESPONSE_CONTENTS_DIM_DIM);
			StrTrim(response->CONTENTS_DIM," ");
		}else
		/*P3P*/
		if(!lstrcmpi(command,"P3P"))
		{
			token = strtok( NULL, "\r\n" );
			lineControlled=true;
			strncpy(response->P3P,token,HTTP_RESPONSE_P3P_DIM);
			StrTrim(response->P3P," ");
		}else
		/*Connection*/
		if(!lstrcmpi(command,"Connection"))
		{
			token = strtok( NULL, "\r\n" );
			lineControlled=true;
			strncpy(response->CONNECTION,token,HTTP_RESPONSE_CONNECTION_DIM);
			StrTrim(response->CONNECTION," ");
		}else
		/*Expires*/
		if(!lstrcmpi(command,"Expires"))
		{
			token = strtok( NULL, "\r\n" );
			lineControlled=true;
			strncpy(response->DATEEXP,token,HTTP_RESPONSE_DATEEXP_DIM);
			StrTrim(response->DATEEXP," ");
		}
		/*
		*If the line is not controlled arrive with the token
		*at the end of the line.
		*/
		if(!lineControlled)
		{
			token = strtok( NULL, "\n" );
			if(token)
			{
				strncat(response->OTHER,token,HTTP_RESPONSE_OTHER_DIM-strlen(response->OTHER));
				strncat(response->OTHER,"\r\n",HTTP_RESPONSE_OTHER_DIM-strlen(response->OTHER));
			}
		}
		token = strtok( NULL, cmdseps );
	}while((u_long)(token-td->buffer)<maxTotchars);
	/*
	*END REQUEST STRUCTURE BUILD.
	*/
	td->nBytesToRead=maxTotchars;
	return validRequest;
}
