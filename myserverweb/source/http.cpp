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


#include "../include/http.h"
#include "../include/cserver.h"
#include "../include/security.h"
#include "../include/AMMimeUtils.h"
#include "../include/filemanager.h"
#include "../include/sockets.h"
#include "../include/winCGI.h"
#include "../include/fastCGI.h"
#include "../include/utility.h"
#include "../include/isapi.h"
#include "../include/stringutils.h"
extern "C" {
#ifdef WIN32
#include <direct.h>
#include <errno.h>
#endif
#ifdef __linux__
#include <string.h>
#include <errno.h>
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
/*
*Browse a folder over the HTTP.
*/
int sendHTTPDIRECTORY(httpThreadContext* td,LPCONNECTION s,char* folder)
{
	/*
	*Send the folder content.
	*/
	u_long nbw;
	MYSERVER_FILE outFile;
	char outFilePath[MAX_PATH];
	getdefaultwd(outFilePath,MAX_PATH);
	sprintf(&outFilePath[strlen(outFilePath)],"/stdInFile_%u",td->id);
	outFile.openFile(outFilePath,MYSERVER_FILE_CREATE_ALWAYS|MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_WRITE);

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
	sprintf(td->buffer2,"<HTML>\r\n<HEAD>\r\n<TITLE>%s</TITLE>\r\n</HEAD>\r\n",td->request.URI);
	outFile.writeToFile(td->buffer2,(u_long)strlen(td->buffer2),&nbw);
	/*
	*If it is defined a CSS file for the graphic layout of the browse folder insert it in the page.
	*/
	if(lserver->getBrowseDirCSS()[0])
	{
		MYSERVER_FILE cssHandle;
		int ret;
		ret=cssHandle.openFile(lserver->getBrowseDirCSS(),MYSERVER_FILE_OPEN_IFEXISTS|MYSERVER_FILE_OPEN_READ);
		if(ret>0)
		{
			u_long nbr;
			cssHandle.readFromFile(td->buffer,td->buffersize,&nbr);
			strcpy(td->buffer2,"<STYLE>\r\n<!--\r\n");
			outFile.writeToFile(td->buffer2,(u_long)strlen(td->buffer2),&nbw);
			outFile.writeToFile(td->buffer,(u_long)nbr,&nbw);
			strcpy(td->buffer2,"-->\r\n</STYLE>\r\n");
			outFile.writeToFile(td->buffer2,(u_long)strlen(td->buffer2),&nbw);
			cssHandle.closeFile();

		}
	}

#ifdef WIN32
	sprintf(filename,"%s/*.*",folder);
#endif
#ifdef __linux__
	sprintf(filename,"%s/",folder);
#endif
	strcpy(td->buffer2,"\r\n<BODY><H1>\r\n");
	strcat(td->buffer2,"Contents of folder");
	strcat(td->buffer2," ");
	strcat(td->buffer2,&td->request.URI[startchar]);
	strcat(td->buffer2,"</H1>\r\n<P>\r\n<HR>\r\n");
	outFile.writeToFile(td->buffer2,strlen(td->buffer2),&nbw);
	intptr_t ff;
	ff=_findfirst(filename,&fd);

#ifdef WIN32
	if(ff==-1)
#endif
#ifdef __linux__
	if((int)ff==-1)
#endif
	{
		if(errno==EACCES)
		{
			/*
			*If client have tried to post a login
			*and a password more times send error 401.
			*/
			return sendAuth(td,s);
		}
		else
		{
			return raiseHTTPError(td,s,e_404);
		}
	}
	/*
	*With the current code we build the HTML TABLE that describes the files in the folder.
	*/
	sprintf(td->buffer2,"<TABLE width=\"100%%\">\r\n<TR>\r\n<TD>%s</TD>\r\n<TD>%s</TD>\r\n<TD>%s</TD>\r\n</TR>\r\n","File","Last modify","Size");
	outFile.writeToFile(td->buffer2,strlen(td->buffer2),&nbw);
	static char fileSize[10];
	static char fileTime[20];
	do
	{	
		if(fd.name[0]=='.')
			continue;
		/*
		*Do not show the security file
		*/
		if(!strcmp(fd.name,"security"))
			continue;
		strcpy(td->buffer2,"<TR>\r\n<TD><A HREF=\"");
		if(!td->request.uriEndsWithSlash)
		{
			strcat(td->buffer2,&td->request.URI[startchar]);
			strcat(td->buffer2,"/");
		}
		strcat(td->buffer2,fd.name);
		strcat(td->buffer2,"\">");
		strcat(td->buffer2,fd.name);
		strcat(td->buffer2,"</TD>\r\n<TD>");
			
		tm *st=gmtime(&fd.time_write);

		sprintf(fileTime,"%u\\%u\\%u-%u:%u:%u System time",st->tm_wday,st->tm_mon,st->tm_year,st->tm_hour,st->tm_min,st->tm_sec);
		strcat(td->buffer2,fileTime);

		strcat(td->buffer2,"</TD>\r\n<TD>");
		if(fd.attrib & FILE_ATTRIBUTE_DIRECTORY)
		{
			strcat(td->buffer2,"[dir]");
		}
		else
		{
			sprintf(fileSize,"%i bytes",fd.size);
			strcat(td->buffer2,fileSize);
		}
		strcat(td->buffer2,"</TD>\r\n</TR>\r\n");
		outFile.writeToFile(td->buffer2,strlen(td->buffer2),&nbw);
	}while(!_findnext(ff,&fd));
	strcpy(td->buffer2,"</TABLE>\r\n<HR>\r\n<ADDRESS>\r\n");
	strcat(td->buffer2,"Running on");
	strcat(td->buffer2," MyServer ");
	strcat(td->buffer2,versionOfSoftware);
	strcat(td->buffer2,"</ADDRESS>\r\n</BODY>\r\n</HTML>\r\n");
	outFile.writeToFile(td->buffer2,strlen(td->buffer2),&nbw);
	_findclose(ff);
	/*
	*Changes the \ character in the / character 
	*/
	char *buffer2Loop=td->buffer2;
	while(*buffer2Loop++)
		if(*buffer2Loop=='\\')
			*buffer2Loop='/';
	buildDefaultHTTPResponseHeader(&(td->response));
	sprintf(td->response.CONTENT_LENGTH,"%u",outFile.getFileSize());
	outFile.setFilePointer(0);
	buildHTTPResponseHeader(td->buffer,&(td->response));
	s->socket.send(td->buffer,strlen(td->buffer), 0);
	u_long nbr,nbs;
	do
	{
		outFile.readFromFile(td->buffer,td->buffersize,&nbr);
		nbs=s->socket.send(td->buffer,nbr,0);
	}while(nbr && nbs);
	outFile.closeFile();
	MYSERVER_FILE::deleteFile(outFilePath);
	return 1;

}
int sendHTTPFILE(httpThreadContext* td,LPCONNECTION s,char *filenamePath,int OnlyHeader,int firstByte,int lastByte)
{
	/*
	*With this routine we send a file through the HTTP protocol.
	*Open the file and save its handle.
	*/
	int ret;
	MYSERVER_FILE h;
	ret=h.openFile(filenamePath,MYSERVER_FILE_OPEN_IFEXISTS|MYSERVER_FILE_OPEN_READ);
	if(ret==0)
	{	
		return 0;
	}
	else if(ret==-1)
	{
		return sendAuth(td,s);
	}
	/*
	*If the file is a valid handle.
	*/
	u_long bytesToSend=h.getFileSize();
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
	if(h.setFilePointer(firstByte))
	{
		return raiseHTTPError(td,s,e_500);
	}

	td->buffer[0]='\0';
	/*If a Range was requested send 206 and not 200 for success*/
	if((lastByte == -1)|(firstByte))
		td->response.httpStatus = 206;
	sprintf(td->response.CONTENT_LENGTH,"%u",bytesToSend);
	time_t lastmodTime=h.getLastModTime();
	getRFC822LocalTime(lastmodTime,td->response.LAST_MODIFIED,HTTP_RESPONSE_LAST_MODIFIED_DIM);
	buildHTTPResponseHeader(td->buffer,&td->response);
	s->socket.send(td->buffer,strlen(td->buffer), 0);

	/*
	*If is requested only the header exit from the function; used by the HEAD request.
	*/
	if(OnlyHeader)
		return 1;

	if(lserver->getVerbosity()>2)
	{
		char msg[500];
		sprintf(msg,"%s %s\n","Sending",filenamePath);
		((vhost*)td->connection->host)->warningsLogWrite(msg);
	}
	for(;;)
	{
		u_long nbr;
		/*
		*Read from the file the bytes to send.
		*/
		h.readFromFile(td->buffer,min(bytesToSend,td->buffersize),&nbr);
		/*
		*When the bytes number read from the file is zero, stop to send the file.
		*/
		if(nbr==0)
			break;
		/*
		*If there are bytes to send, send them.
		*/
		if(s->socket.send(td->buffer,nbr, 0) == SOCKET_ERROR)
			break;
	}
	h.closeFile();
	return 1;

}
/*
*Main function to handle the HTTP PUT command.
*/
int putHTTPRESOURCE(httpThreadContext* td,LPCONNECTION s,char *filename,int systemrequest,int,int firstByte,int lastByte,int yetmapped)
{
	int httpStatus=td->response.httpStatus;
	buildDefaultHTTPResponseHeader(&td->response);
	td->response.httpStatus=httpStatus;
	/*
	*td->filenamePath is the file system mapped path while filename is the URI requested.
	*systemrequest is 1 if the file is in the system folder.
	*If filename is already mapped on the file system don't map it again.
	*/
	if(yetmapped)
	{
		strcpy(td->filenamePath,filename);
	}
	else
	{
		/*
		*If the client try to access files that aren't in the web folder send a 401 error.
		*/
		translateEscapeString(filename );
		if((filename[0] != '\0')&&(MYSERVER_FILE::getPathRecursionLevel(filename)<1))
		{
			return raiseHTTPError(td,s,e_401);
		}
		getPath(td,td->filenamePath,filename,0);
	}
	int permissions=-1;
	char folder[MAX_PATH];
	if(MYSERVER_FILE::isFolder(td->filenamePath))
		strcpy(folder,td->filenamePath);
	else
		MYSERVER_FILE::splitPath(td->filenamePath,folder,filename);
	
	if(td->connection->login[0])
		permissions=getPermissionMask(td->connection->login,td->connection->password,folder,filename,((vhost*)(td->connection->host))->systemRoot);
	else/*The default user is Guest with a null password*/
		permissions=getPermissionMask("Guest","",folder,filename,((vhost*)(td->connection->host))->systemRoot);
	if(!(permissions & MYSERVER_PERMISSION_WRITE))
	{
		return sendAuth(td,s);
	}
	if(MYSERVER_FILE::fileExists(td->filenamePath))
	{
		/*
		*If the file exists update it.
		*/
		MYSERVER_FILE file;
		file.openFile(td->filenamePath,MYSERVER_FILE_OPEN_IFEXISTS|MYSERVER_FILE_OPEN_WRITE);
		file.setFilePointer(firstByte);
		for(;;)
		{
			u_long nbr=0,nbw=0;
			td->inputData.readFromFile(td->buffer,td->buffersize,&nbr);
			if(nbr)
				file.writeToFile(td->buffer,nbr,&nbw);
			else
				break;
			if(nbw!=nbr)
			{
				file.closeFile();
				return raiseHTTPError(td,s,e_500);/*Internal server error*/
			}
		}
		file.closeFile();
		raiseHTTPError(td,s,e_200);/*Successful updated*/
		return 1;
	}
	else
	{
		/*
		*If the file doesn't exist create it.
		*/
		MYSERVER_FILE file;
		file.openFile(td->filenamePath,MYSERVER_FILE_CREATE_ALWAYS|MYSERVER_FILE_OPEN_WRITE);
		for(;;)
		{
			u_long nbr=0,nbw=0;
			td->inputData.readFromFile(td->buffer,td->buffersize,&nbr);
			if(nbr)
				file.writeToFile(td->buffer,nbr,&nbw);
			else
				break;
			if(nbw!=nbr)
			{
				file.closeFile();
				return raiseHTTPError(td,s,e_500);/*Internal server error*/
			}
		}
		file.closeFile();
		raiseHTTPError(td,s,e_201);/*Successful created*/
		return 1;
	}
	
}
/*
*Delete the resource identified by filename
*/
int deleteHTTPRESOURCE(httpThreadContext* td,LPCONNECTION s,char *filename,int yetmapped)
{

	int httpStatus=td->response.httpStatus;
	buildDefaultHTTPResponseHeader(&td->response);
	td->response.httpStatus=httpStatus;
	/*
	*td->filenamePath is the file system mapped path while filename is the URI requested.
	*systemrequest is 1 if the file is in the system folder.
	*If filename is already mapped on the file system don't map it again.
	*/
	if(yetmapped)
	{
		strcpy(td->filenamePath,filename);
	}
	else
	{
		/*
		*If the client try to access files that aren't in the web folder send a 401 error.
		*/
		translateEscapeString(filename );
		if((filename[0] != '\0')&&(MYSERVER_FILE::getPathRecursionLevel(filename)<1))
		{
			return raiseHTTPError(td,s,e_401);
		}
		getPath(td,td->filenamePath,filename,0);
	}
	int permissions=-1;
	char folder[MAX_PATH];
	if(MYSERVER_FILE::isFolder(td->filenamePath))
		strcpy(folder,td->filenamePath);
	else
		MYSERVER_FILE::splitPath(td->filenamePath,folder,filename);
	
	if(td->connection->login[0])
		permissions=getPermissionMask(td->connection->login,td->connection->password,folder,filename,((vhost*)(td->connection->host))->systemRoot);
	else/*The default user is Guest with a null password*/
		permissions=getPermissionMask("Guest","",folder,filename,((vhost*)(td->connection->host))->systemRoot);
	if(!(permissions & MYSERVER_PERMISSION_DELETE))
	{
		return sendAuth(td,s);
	}
	if(MYSERVER_FILE::fileExists(td->filenamePath))
	{
		MYSERVER_FILE::deleteFile(td->filenamePath);
		return raiseHTTPError(td,s,e_202);/*Successful deleted*/
	}
	else
	{
		return raiseHTTPError(td,s,e_204);/*No content*/
	}
}
/*
*Main function to send a resource to a client.
*/
int sendHTTPRESOURCE(httpThreadContext* td,LPCONNECTION s,char *URI,int systemrequest,int OnlyHeader,int firstByte,int lastByte,int yetmapped)
{
	/*
	*With this code we manage a request of a file or a folder or anything that we must send
	*over the HTTP.
	*/
	char filename[MAX_PATH];
	strncpy(filename,URI,MAX_PATH);
	td->buffer[0]='\0';
	if(!systemrequest)
	{
		buildDefaultHTTPResponseHeader(&td->response);
	}
	else
	{
		int httpStatus=td->response.httpStatus;
		buildDefaultHTTPResponseHeader(&td->response);
		td->response.httpStatus=httpStatus;
		/*
		*The response will be recorded by the error handler function.
		*/
	}

	static char ext[10];
	static char data[MAX_PATH];
	/*
	*td->filenamePath is the file system mapped path while filename is the URI requested.
	*systemrequest is 1 if the file is in the system folder.
	*If filename is already mapped on the file system don't map it again.
	*/
	if(yetmapped)
	{
		strcpy(td->filenamePath,filename);
	}
	else
	{
		/*
		*If the client try to access files that aren't in the web folder send a 401 error.
		*/
		translateEscapeString(filename );
		if((filename[0] != '\0')&&(MYSERVER_FILE::getPathRecursionLevel(filename)<1))
		{
			return raiseHTTPError(td,s,e_401);
		}
		getPath(td,td->filenamePath,filename,systemrequest);
	}

	int permissions=-1;/*By default everything is permitted*/
	if(!systemrequest)
	{
		char folder[MAX_PATH];
		if(MYSERVER_FILE::isFolder(td->filenamePath))
			strcpy(folder,td->filenamePath);
		else
			MYSERVER_FILE::splitPath(td->filenamePath,folder,filename);
		if(td->connection->login[0])
			permissions=getPermissionMask(td->connection->login,td->connection->password,folder,filename,((vhost*)(td->connection->host))->systemRoot);
		else/*The default user is Guest with a null password*/
			permissions=getPermissionMask("Guest","",folder,filename,((vhost*)(td->connection->host))->systemRoot);
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
	int filenamePathLen=(int)strlen(td->filenamePath);
	for(int i=0,len=0;i<filenamePathLen;i++)
	{
		/*
		*http://host/pathtofile/filetosend.php/PATH_INFO_VALUE?QUERY_INFO_VALUE
		*When a request has this form send the file filetosend.php with the
		*environment string PATH_INFO equals to PATH_INFO_VALUE and QUERY_INFO
		*to QUERY_INFO_VALUE.
		*/
		if(i && (td->filenamePath[i]=='/'))/*There is the '/' character check if dirscan is a file*/
		{
			if(!MYSERVER_FILE::isFolder(dirscan))
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
	if(td->pathInfo[0])
	{
        td->pathTranslated[0]='\0';
		/*
		*Start from the second character because the first is a slash character.
		*/
		getPath(td,(td->pathTranslated),&((td->pathInfo)[1]),0);
		MYSERVER_FILE::completePath(td->pathTranslated);
	}
	else
	{
        td->pathTranslated[0]='\0';
	}
	MYSERVER_FILE::completePath(td->filenamePath);

	/*
	*If there are not any extension then we do one of this in order:
	1)We send the default files in the folder in order.
	2)We send the folder content.
	3)We send an error.
	*/
	if(MYSERVER_FILE::isFolder((char *)(td->filenamePath)))
	{
		if(!(permissions & MYSERVER_PERMISSION_BROWSE))
		{
			return sendAuth(td,s);
		}
		int i;
		for(i=0;;i++)
		{
			static char defaultFileName[MAX_PATH];
			char *defaultFileNamePath=lserver->getDefaultFilenamePath(i);
			if(defaultFileNamePath)
				sprintf(defaultFileName,"%s/%s",td->filenamePath,defaultFileNamePath);
			else
				break;
			if(MYSERVER_FILE::fileExists(defaultFileName))
			{
				/*
				*Change the URI to reflect the default file name.
				*/
				char nURL[MAX_PATH+HTTP_REQUEST_URI_DIM+12];
				if(((vhost*)td->connection->host)->protocol==PROTOCOL_HTTP)
					strcpy(nURL,"http://");
				strcat(nURL,td->request.HOST);
				int isPortSpecified=0;
				for(int i=0;td->request.HOST[i];i++)
				{
					if(td->request.HOST[i]==':')
					{
						isPortSpecified	= 1;
						break;
					}
				}
				if(!isPortSpecified)
					sprintf(&nURL[strlen(nURL)],":%u",((vhost*)td->connection->host)->port);
				if(nURL[strlen(nURL)-1]!='/')
					strcat(nURL,"/");
				strcat(nURL,td->request.URI);
				if(nURL[strlen(nURL)-1]!='/')
					strcat(nURL,"/");
				strcat(nURL,defaultFileNamePath);
				if(td->request.URIOPTS[0])
				{
					strcat(nURL,"?");
					strcat(nURL,td->request.URIOPTS);
				}

				if(sendHTTPRedirect(td,s,nURL))
					return 1;
				else
					return 0;
			}
		}

		if(sendHTTPDIRECTORY(td,s,td->filenamePath))
			return 1;	

		return raiseHTTPError(td,s,e_404);
	}

	if(!MYSERVER_FILE::fileExists(td->filenamePath))
		return raiseHTTPError(td,s,e_404);

	/*
	*getMIME returns the type of command registered by the extension.
	*/
	int mimeCMD=getMIME(td->response.CONTENT_TYPE,td->filenamePath,ext,data);
	if((mimeCMD==CGI_CMD_RUNCGI)||(mimeCMD==CGI_CMD_EXECUTE))
	{
		if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
		{
			return sendAuth(td,s);
		}
		if(sendCGI(td,s,td->filenamePath,ext,data,mimeCMD))
			return 1;
	}else if(mimeCMD==CGI_CMD_RUNISAPI)
	{
#ifdef WIN32
		if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
		{
			return sendAuth(td,s);
		}
		return sendISAPI(td,s,td->filenamePath,ext,data,0);
#endif
#ifdef __linux__
		return raiseHTTPError(td,s,e_501);
#endif
	}else if(mimeCMD==CGI_CMD_EXECUTEISAPI)
	{
#ifdef WIN32
		if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
		{
			return sendAuth(td,s);
		}
		return sendISAPI(td,s,td->filenamePath,ext,data,1);
#endif
#ifdef __linux__
		return raiseHTTPError(td,s,e_501);
#endif
	}
	else if(mimeCMD==CGI_CMD_RUNMSCGI)
	{
		if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
		{
			return sendAuth(td,s);
		}
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
	}else if(mimeCMD==CGI_CMD_EXECUTEWINCGI)
	{
		if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
		{
			return sendAuth(td,s);
		}
		char cgipath[MAX_PATH*2];
		if(data[0])
			sprintf(cgipath,"%s \"%s\"",data,td->filenamePath);
		else
			sprintf(cgipath,"%s",td->filenamePath);
	
		int ret=sendWINCGI(td,s,cgipath);
		if(td->outputData.getHandle())
		{
			td->outputData.closeFile();
			MYSERVER_FILE::deleteFile(td->outputDataPath);
		}
		if(td->inputData.getHandle())
		{
			td->inputData.closeFile();
			MYSERVER_FILE::deleteFile(td->inputDataPath);
		}
		return ret;

	}
	else if(mimeCMD==CGI_CMD_RUNFASTCGI)
	{
		if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
		{
			return sendAuth(td,s);
		}	
		int ret = sendFASTCGI(td,s,td->filenamePath,ext,data,0);
		if(td->outputData.getHandle())
		{
			td->outputData.closeFile();
			MYSERVER_FILE::deleteFile(td->outputDataPath);
		}
		if(td->inputData.getHandle())
		{
			td->inputData.closeFile();
			MYSERVER_FILE::deleteFile(td->inputDataPath);
		}
		return ret;
	}
	else if(mimeCMD==CGI_CMD_EXECUTEFASTCGI)
	{
		if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
		{
			return sendAuth(td,s);
		}
		int ret = sendFASTCGI(td,s,td->filenamePath,ext,data,1);
		if(td->outputData.getHandle())
		{
			td->outputData.closeFile();
			MYSERVER_FILE::deleteFile(td->outputDataPath);
		}
		if(td->inputData.getHandle())
		{
			td->inputData.closeFile();
			MYSERVER_FILE::deleteFile(td->inputDataPath);
		}
		return ret;
	}
	else if(mimeCMD==CGI_CMD_SENDLINK)
	{
		if(!(permissions & MYSERVER_PERMISSION_READ))
		{
			return sendAuth(td,s);
		}
		MYSERVER_FILE h;
		h.openFile(td->filenamePath,MYSERVER_FILE_OPEN_IFEXISTS|MYSERVER_FILE_OPEN_READ);
		u_long nbr;
		char linkpath[MAX_PATH];
		char pathInfo[MAX_PATH];
		h.readFromFile(linkpath,MAX_PATH,&nbr);
		h.closeFile();
		linkpath[nbr]='\0';
		strcpy(pathInfo,td->pathInfo);
		translateEscapeString(pathInfo);
		strcat(linkpath,pathInfo);

		if(nbr)
			return sendHTTPRESOURCE(td,s,linkpath,systemrequest,OnlyHeader,firstByte,lastByte,1);
		else
			return raiseHTTPError(td,s,e_404);
	}
	if(!(permissions & MYSERVER_PERMISSION_READ))
	{
		return sendAuth(td,s);
	}

	time_t lastMT=MYSERVER_FILE::getLastModTime(td->filenamePath);
	if(lastMT==-1)
		return raiseHTTPError(td,s,e_500);
	getRFC822LocalTime(lastMT,td->response.LAST_MODIFIED,HTTP_RESPONSE_LAST_MODIFIED_DIM);
	if(td->request.IF_MODIFIED_SINCE[0])
	{
		time_t timeMS=getTime(td->request.IF_MODIFIED_SINCE);
		if(timeMS==lastMT)
			return sendHTTPNonModified(td,s);
	}
	if(sendHTTPFILE(td,s,td->filenamePath,OnlyHeader,firstByte,lastByte))
		return 1;
	return sendHTTPhardError500(td,s);
	
}

/*
*This is the HTTP protocol main procedure to parse a request made over the HTTP.
*/
int controlHTTPConnection(LPCONNECTION a,char *b1,char *b2,int bs1,int bs2,u_long nbtr,u_long id)
{
	/*
	*Bit mask.
	*|...|31|32|
	*Bit 32		->	Return value;
	*Bit 31		->	Return from the function;
	*Bits 1-30	->	Not used.
	*/
	int retvalue=0;
	httpThreadContext td;
	td.buffer=b1;
	td.buffer2=b2;
	td.buffersize=bs1;
	td.buffersize2=bs2;
	td.nBytesToRead=nbtr;
	td.identity[0]='\0';
	td.connection=a;
	td.id=id;
	td.inputData.setHandle(0);
	td.outputData.setHandle(0);
	td.outputDataPath[0]='\0';
	td.inputDataPath[0]='\0';
	/*
	*Reset the request structure.
	*/
	resetHTTPRequest(&td.request);

	u_long validRequest=buildHTTPRequestHeaderStruct(&td.request,&td);
	if(validRequest==-1)/*If the header is incomplete returns 2*/
	{
		return 2;
	}
	td.nBytesToRead+=a->dataRead;
	/*
	*If the header is an invalid request send the correct error message to the client and return immediately.
	*/
	if(validRequest==0)
	{
		sprintf(td.buffer,"Bad request from: %s\r\n",td.connection->ipAddr);
		((vhost*)td.connection->host)->warningsLogWrite(td.buffer);
		return raiseHTTPError(&td,a,e_400);
	}/*If the URI is too long*/
	else if(validRequest==414)
	{
		sprintf(td.buffer,"URI too long from: %s\r\n",td.connection->ipAddr);
		((vhost*)td.connection->host)->warningsLogWrite(td.buffer);
		return raiseHTTPError(&td,a,e_414);
	}
	

	/*
	*For methods that accept data after the HTTP header set the correct pointer and create a file
	*containing the informations after the header.
	*/
	getdefaultwd(td.inputDataPath,MAX_PATH);
	sprintf(&td.inputDataPath[strlen(td.inputDataPath)],"/stdInFile_%u",td.id);
	getdefaultwd(td.outputDataPath,MAX_PATH);
	sprintf(&td.outputDataPath[strlen(td.outputDataPath)],"/stdOutFile_%u",td.id);
	if((!lstrcmpi(td.request.CMD,"POST"))||(!lstrcmpi(td.request.CMD,"PUT")))
	{
		if(td.request.CONTENT_TYPE[0]=='\0')
			strcpy(td.request.CONTENT_TYPE,"application/x-www-form-urlencoded");

		/*
		*Read POST data
		*/
		{
			td.request.URIOPTSPTR=&td.buffer[td.nHeaderChars];
			td.buffer[min(td.nBytesToRead,td.buffersize)]='\0';
			/*
			*Create the file that contains the data posted.
			*This data is the stdin file in the CGI.
			*/
			td.inputData.openFile(td.inputDataPath,MYSERVER_FILE_CREATE_ALWAYS|MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_WRITE);
			u_long nbw;
			u_long total_nbr=min(td.nBytesToRead,td.buffersize)-td.nHeaderChars;
			
			td.inputData.writeToFile(td.request.URIOPTSPTR,total_nbr,&nbw);
			
			u_long content_len=atoi(td.request.CONTENT_LENGTH);
			/*
			*If the connection is Keep-Alive be sure that the client specify a the
			*HTTP CONTENT-LENGTH field.
			*If a CONTENT-ENCODING is specified the CONTENT-LENGTH is not always needed.
			*/
			if(!lstrcmpi(td.request.CONNECTION,"Keep-Alive"))
			{
				if((td.request.CONTENT_ENCODING[0]=='\0') && (td.request.CONTENT_LENGTH[0]=='\0'))
				{
					td.inputData.closeFile();
					td.inputData.deleteFile(td.inputDataPath);
					return raiseHTTPError(&td,a,e_400);
				}
			}
			/*
			*If there are others bytes to read from the socket.
			*/
			u_long timeout=clock();
			if((content_len)&&(content_len!=nbw))
			{
				int err;
				u_long fs;
				do
				{
					err=0;
					fs=td.inputData.getFileSize();
					while(clock()-timeout<SEC(5))
					{
						if(content_len==total_nbr)
						{
							/*
							*Consider only CONTENT-LENGTH bytes of data.
							*/
							while(td.connection->socket.bytesToRead())
							{
								/*
								*Read the unwanted bytes but do not save them.
								*/
								err=td.connection->socket.recv(td.buffer2,td.buffersize2, 0);
							}
							break;
						}
						if((content_len>fs)&&(td.connection->socket.bytesToRead()))
						{				
							u_long tr=min(content_len-total_nbr,td.buffersize2);
							err=td.connection->socket.recv(td.buffer2,tr, 0);
							td.inputData.writeToFile(td.buffer2,min((u_long)err, (content_len-fs)),&nbw);	
							total_nbr+=nbw;
							timeout=clock();
							break;
						}
					}
					if(clock()-timeout>=SEC(5))
						break;
				}
				while(content_len!=total_nbr);

				fs=td.inputData.getFileSize();
				if(content_len!=fs)
				{
					/*
					If we get an error remove the file and the connection.
					*/
					td.inputData.closeFile();
					td.inputData.deleteFile(td.inputDataPath);
					return raiseHTTPError(&td,a,e_500);
				}
			}
			else if(content_len==0)/*If CONTENT-LENGTH is not specified read all the data*/
			{
				int err;
				do
				{
					err=0;
					while(clock()-timeout<SEC(3))
					{
						if(td.connection->socket.bytesToRead())
						{				
							err=td.connection->socket.recv(td.buffer2,td.buffersize2, 0);
							td.inputData.writeToFile(td.buffer2,err,&nbw);	
							total_nbr+=nbw;
							timeout=clock();
							break;
						}
					}
					if(clock()-timeout>=SEC(3))
						break;
				}
				while(content_len!=total_nbr);
				sprintf(td.response.CONTENT_LENGTH,"%u",td.inputData.getFileSize());
		
			}
			td.inputData.setFilePointer(0);
			td.buffer2[0]='\0';

		}
	}
	else
	{
		td.request.URIOPTSPTR=0;
	}
	if(!strcmpi(td.request.CONTENT_ENCODING,"chunked"))
	{
		MYSERVER_FILE newStdIn;
		sprintf(td.inputDataPath,"%s_encoded",td.inputData.getFilename());
		newStdIn.openFile(td.inputDataPath,MYSERVER_FILE_CREATE_ALWAYS|MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_WRITE);		
		char buffer[20];
		char c;
		u_long nbr;
		u_long bufferlen;
		for(;;)
		{
			bufferlen=0;
			buffer[0]='\0';
			for(;;)
			{
				td.inputData.readFromFile(&c,1,&nbr);
				if(nbr=!1)
					break;
				if((c!='\r') && (bufferlen<19))
				{
					buffer[bufferlen++]=c;
					buffer[bufferlen]='\0';
				}
				else
					break;
			}
			td.inputData.readFromFile(&c,1,&nbr);/*Read the \n char too*/
			u_long dataToRead=(u_long)hexToInt(buffer);
			if(dataToRead==0)/*The last chunk length is 0*/
				break;

			u_long dataRead=0;
			while(dataRead<dataToRead)
			{
				td.inputData.readFromFile(td.buffer,min(dataToRead-dataRead,td.buffersize),&nbr);
				if(nbr==0)
					break;
				dataRead+=nbr;
				u_long nbw;
				newStdIn.writeToFile(td.buffer,nbr,&nbw);
				if(nbw!=nbr)
					break;
			}

		}
		/*
		*Now replace the file with the encoded one.
		*/
		td.inputData.closeFile();
		td.inputData.deleteFile(td.inputData.getFilename());
		td.inputData=newStdIn;
	}


	if(!(retvalue&2))/*If return value is not setted.*/
	{
		/*
		*Record the request in the log file.
		*/
		((vhost*)(td.connection->host))->accessesLogWrite(a->ipAddr);
		((vhost*)(td.connection->host))->accessesLogWrite(":");
		((vhost*)(td.connection->host))->accessesLogWrite(td.request.CMD);
		((vhost*)(td.connection->host))->accessesLogWrite(" ");
		((vhost*)(td.connection->host))->accessesLogWrite(td.request.URI);
		if(td.request.URIOPTS[0])
		{
			((vhost*)(td.connection->host))->accessesLogWrite("?");
			((vhost*)(td.connection->host))->accessesLogWrite(td.request.URIOPTS);
		}
		((vhost*)(td.connection->host))->accessesLogWrite("\r\n");
		/*
		*End record the request in the structure.
		*/

		/*
		*How is expressly said in the rfc2616 a client that sends an 
		*HTTP/1.1 request MUST sends a Host header.
		*Servers MUST reports a 400 (Bad request) error if an HTTP/1.1
		*request does not include a Host request-header.
		*/
		if((!strcmp(td.request.VER,"HTTP/1.1")) && td.request.HOST[0]==0)
		{
			raiseHTTPError(&td,a,e_400);
			if(td.inputData.getHandle())
			{
				td.inputData.closeFile();
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
				if(td.inputData.getHandle())
				{
					td.inputData.closeFile();
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
				sendHTTPRESOURCE(&td,a,td.request.URI,0,0,atoi(td.request.RANGEBYTEBEGIN),atoi(td.request.RANGEBYTEEND));
			else
				sendHTTPRESOURCE(&td,a,td.request.URI);
		}
		else if(!lstrcmpi(td.request.CMD,"POST"))/*POST REQUEST*/
		{
			if(!lstrcmpi(td.request.RANGETYPE,"bytes"))
				sendHTTPRESOURCE(&td,a,td.request.URI,0,0,atoi(td.request.RANGEBYTEBEGIN),atoi(td.request.RANGEBYTEEND));
			else
				sendHTTPRESOURCE(&td,a,td.request.URI);
		}
		else if(!lstrcmpi(td.request.CMD,"HEAD"))/*HEAD REQUEST*/
		{
			if(!lstrcmpi(td.request.RANGETYPE,"bytes"))
				sendHTTPRESOURCE(&td,a,td.request.URI,0,1,atoi(td.request.RANGEBYTEBEGIN),atoi(td.request.RANGEBYTEEND));
			else
				sendHTTPRESOURCE(&td,a,td.request.URI,0,1);
		}
		else if(!lstrcmpi(td.request.CMD,"DELETE"))/*DELETE REQUEST*/
		{
			deleteHTTPRESOURCE(&td,a,td.request.URI,0);
		}
		else if(!lstrcmpi(td.request.CMD,"PUT"))/*PUT REQUEST*/
		{
			if(!lstrcmpi(td.request.RANGETYPE,"bytes"))
				putHTTPRESOURCE(&td,a,td.request.URI,0,1,atoi(td.request.RANGEBYTEBEGIN),atoi(td.request.RANGEBYTEEND));
			else
				putHTTPRESOURCE(&td,a,td.request.URI,0,1);
		}
		else
		{
			raiseHTTPError(&td,a,e_501);
			retvalue=(~1);/*Set first bit to 0*/
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
		retvalue=(~1);/*Set first bit to 0*/
		retvalue|=2;/*Set second bit to 1*/
	}
	/*
	*If the inputData file was not closed close it.
	*/
	if(td.inputData.getHandle())
	{
		td.inputData.closeFile();
		MYSERVER_FILE::deleteFile(td.inputDataPath);
	}
	/*
	*If the outputData file was not closed close it.
	*/
	if(td.outputData.getHandle())
	{
		td.outputData.closeFile();
		MYSERVER_FILE::deleteFile(td.outputDataPath);
	}	
	return (retvalue&1)?1:0;
}
/*
*Reset all the HTTP_REQUEST_HEADER structure members.
*/
void resetHTTPRequest(HTTP_REQUEST_HEADER *request)
{
	request->CONTENT_ENCODING[0]='\0';	
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
	request->CONTENT_TYPE[0]='\0';
	request->CONTENT_LENGTH[0]='\0';
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
	request->CACHE_CONTROL[0]='\0';
	request->IF_MODIFIED_SINCE[0]='\0';
	request->OTHER[0]='\0';
	request->PRAGMA[0]='\0';
	request->RANGETYPE[0]='\0';		
	request->RANGEBYTEBEGIN[0]='\0';
	request->RANGEBYTEEND[0]='\0';
	request->uriEndsWithSlash=0;
}
/*
*Reset all the HTTP_RESPONSE_HEADER structure members.
*/
void resetHTTPResponse(HTTP_RESPONSE_HEADER *response)
{
	response->httpStatus=200;
	response->VER[0]='\0';	
	response->SERVER_NAME[0]='\0';
	response->CONTENT_TYPE[0]='\0';
	response->CONNECTION[0]='\0';
	response->MIMEVER[0]='\0';
	response->P3P[0]='\0';
	response->COOKIE[0]='\0';
	response->CONTENT_LENGTH[0]='\0';
	response->ERROR_TYPE[0]='\0';
	response->CONTENT_ENCODING[0]='\0';
	response->LOCATION[0]='\0';
	response->DATE[0]='\0';		
	response->AUTH[0]='\0';
	response->DATEEXP[0]='\0';	
	response->OTHER[0]='\0';
	response->LAST_MODIFIED[0]='\0';
	response->CACHE_CONTROL[0]='\0';
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
	{
		if(response->ERROR_TYPE[0]=='\0')
		{
			int errID=getErrorIDfromHTTPStatusCode(response->httpStatus);
			if(errID!=-1)
				strcpy(response->ERROR_TYPE,HTTP_ERROR_MSGS[errID]);
		}
		sprintf(str,"%s %i %s\r\nStatus: %s\r\n",response->VER,response->httpStatus,response->ERROR_TYPE,response->ERROR_TYPE);
	}
	else
		sprintf(str,"%s 200 OK\r\n",response->VER);

	if(response->CONTENT_LENGTH[0])
	{
		strcat(str,"Content-Length:");
		strcat(str,response->CONTENT_LENGTH);
		strcat(str,"\r\n");
	}
	if(response->SERVER_NAME[0])
	{
		strcat(str,"Server:");
		strcat(str,response->SERVER_NAME);
		strcat(str,"\r\n");
	}
	if(response->CACHE_CONTROL[0])
	{
		strcat(str,"Cache-Control:");
		strcat(str,response->CACHE_CONTROL);
		strcat(str,"\r\n");
	}
	if(response->LAST_MODIFIED[0])
	{
		strcat(str,"Last-Modified:");
		strcat(str,response->LAST_MODIFIED);
		strcat(str,"\r\n");
	}
	if(response->CONNECTION[0])
	{
		strcat(str,"Connection:");
		strcat(str,response->CONNECTION);
		strcat(str,"\r\n");
	}
	if(response->CONTENT_ENCODING[0])
	{
		strcat(str,"Content-Encoding:");
		strcat(str,response->CONTENT_ENCODING);
		strcat(str,"\r\n");
	}
	if(response->COOKIE[0])
	{
		char *token=strtok(response->COOKIE,"\n");
		do
		{
			strcat(str,"Set-Cookie:");
			strcat(str,token);
			strcat(str,"\r\n");		
			token=strtok(NULL,"\n");
		}while(token);
	}
	if(response->P3P[0])
	{
		strcat(str,"P3P:");
		strcat(str,response->P3P);
		strcat(str,"\r\n");
	}
	if(response->MIMEVER[0])
	{
		strcat(str,"MIME-Version:");
		strcat(str,response->MIMEVER);
		strcat(str,"\r\n");
	}
	if(response->CONTENT_TYPE[0])
	{
		strcat(str,"Content-Type:");
		strcat(str,response->CONTENT_TYPE);
		strcat(str,"\r\n");
	}
	if(response->DATE[0])
	{
		strcat(str,"Date:");
		strcat(str,response->DATE);
		strcat(str,"\r\n");
	}
	if(response->DATEEXP[0])
	{
		strcat(str,"Expires:");
		strcat(str,response->DATEEXP);
		strcat(str,"\r\n");
	}
	if(response->AUTH[0])
	{
		strcat(str,"WWW-Authenticate:");
		strcat(str,response->AUTH);
		strcat(str,"\r\n");
	}
	
	if(response->LOCATION[0])
	{
		strcat(str,"Location:");
		strcat(str,response->LOCATION);
		strcat(str,"\r\n");
	}
	if(response->OTHER[0])
	{
		strcat(str,response->OTHER);
	}

	/*
	*MyServer supports the bytes range.
	*/
	strcat(str,"Accept-Ranges: bytes\r\n");
	/*
	*The HTTP header ends with a \r\n sequence.
	*/
	strcat(str,"\r\n");

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
	strcpy(response->CONTENT_TYPE,"text/html");
	strcpy(response->VER,"HTTP/1.1");
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
		sprintf(td->buffer2,"HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic\r\nAccept-Ranges: bytes\r\nServer: MyServer %s\r\nContent-type: text/html\r\nConnection:%s\r\nContent-length: 0\r\n",versionOfSoftware,td->request.CONNECTION);
		strcat(td->buffer2,"Date: ");
		getRFC822GMTTime(&td->buffer2[strlen(td->buffer2)],HTTP_RESPONSE_DATE_DIM);
		strcat(td->buffer2,"\r\n\r\n");
		a->socket.send(td->buffer2,(int)strlen(td->buffer2),0);
		return 1;
	}
	else
	{
		char defFile[MAX_PATH*2];
		if(getErrorFileName(((vhost*)td->connection->host)->documentRoot,getHTTPStatusCodeFromErrorID(ID),defFile))
		{
			/*
			*Change the URI to reflect the default file name.
			*/
			char nURL[MAX_PATH+HTTP_REQUEST_URI_DIM+12];
			if(((vhost*)td->connection->host)->protocol==PROTOCOL_HTTP)
				strcpy(nURL,"http://");
			strcat(nURL,td->request.HOST);
			sprintf(&nURL[strlen(nURL)],":%u",((vhost*)td->connection->host)->port);
			if(nURL[strlen(nURL)-1]!='/')
				strcat(nURL,"/");
			strcat(nURL,defFile);
			return sendHTTPRedirect(td,a,nURL);
		}
		if(lserver->getVerbosity()>1)
		{
			/*
			*Record the error in the log file.
			*/
			sprintf(td->buffer,"%s error to: %s\r\n",HTTP_ERROR_MSGS[ID],td->connection->ipAddr);
			((vhost*)td->connection->host)->warningsLogWrite(td->buffer);
		}
	}
	getRFC822GMTTime(td->response.DATEEXP,HTTP_RESPONSE_DATEEXP_DIM);
	td->response.httpStatus=getHTTPStatusCodeFromErrorID(ID);
	strcpy(td->response.ERROR_TYPE,HTTP_ERROR_MSGS[ID]);
	char errorFile[MAX_PATH];
	sprintf(errorFile,"%s/%s",((vhost*)(td->connection->host))->systemRoot,HTTP_ERROR_HTMLS[ID]);
	if(lserver->mustUseMessagesFiles() && MYSERVER_FILE::fileExists(errorFile))
	{
		 return sendHTTPRESOURCE(td,a,HTTP_ERROR_HTMLS[ID],1);
	}
	sprintf(td->response.CONTENT_LENGTH,"%i",strlen(HTTP_ERROR_MSGS[ID]));

	buildHTTPResponseHeader(td->buffer,&td->response);
	a->socket.send(td->buffer,(int)strlen(td->buffer), 0);
	a->socket.send(HTTP_ERROR_MSGS[ID],strlen(HTTP_ERROR_MSGS[ID]), 0);
	return 1;
}
/*
*Send a hard wired 500 error when we have a system error
*/
int sendHTTPhardError500(httpThreadContext* td,LPCONNECTION a)
{
	sprintf(td->buffer,"%s from: %s\r\n",HTTP_ERROR_MSGS[e_500],td->connection->ipAddr);
	((vhost*)td->connection->host)->warningsLogWrite(td->buffer);
	const char hardHTML[] = "<!-- Hard Coded 500 Responce --><body bgcolor=\"#000000\"><p align=\"center\">\
		<font size=\"5\" color=\"#00C800\">Error 500</font></p><p align=\"center\"><font size=\"5\" color=\"#00C800\">\
		Internal Server error</font></p>\r\n";

	sprintf(td->buffer2,"HTTP/1.1 500 System Error\r\nServer: MyServer %s\r\nContent-type: text/html\r\nContent-length: %d\r\n",versionOfSoftware,strlen(hardHTML));
	strcat(td->buffer2,"Date: ");
	getRFC822GMTTime(&td->buffer2[strlen(td->buffer2)],HTTP_RESPONSE_DATE_DIM);
	strcat(td->buffer2,"\r\n\r\n");
	a->socket.send(td->buffer2,(int)strlen(td->buffer2),0);

	a->socket.send(hardHTML,(int)strlen(hardHTML), 0);
	return 1;
}
/*
*Returns the MIME type passing its extension.
*/
int getMIME(char *MIME,char *filename,char *dest,char *dest2)
{
	MYSERVER_FILE::getFileExt(dest,filename);
	/*
	*Returns 1 if file is registered by a CGI.
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
*Returns 0 if req is an invalid header, 
*Returns -1 if the header is incomplete,
*Returns another non-zero value if is a valid header.
*nLinesptr is a value of the lines number in the HEADER.
*ncharsptr is a value of the characters number in the HEADER.
*/
u_long validHTTPRequest(char *req,httpThreadContext* td,u_long* nLinesptr,u_long* ncharsptr)
{
	u_long i;
	u_long buffersize=td->buffersize;
	u_long nLinechars;
	u_long isValidCommand=0;
	nLinechars=0;
	u_long nLines=0;
	u_long maxTotchars=0;
	if(req==0)
		return 0;
	/*
	*Count the number of lines in the header.
	*/
	for(nLines=i=0;(i<KB(8));i++)
	{
		if(req[i]=='\n')
		{
			if(req[i+2]=='\n')
			{
				maxTotchars=i+3;
				if(maxTotchars>buffersize)
				{
					isValidCommand=0;
					break;				
				}
				isValidCommand=1;
				break;
			}
			nLines++;
			/*
			*If the lines number is greater than 25 we consider the header invalid.
			*/
			if(nLines>25)
				return 0;
		}
		else if(req[i]==0)
			return ((u_long)-1);
		else
		{
			nLinechars++;
		}
		/*
		*We set a maximal theorical number of characters in a line to 1024.
		*If a line contains more than 1024 lines we consider the header invalid.
		*/
		if(nLinechars>1024)
		{
			isValidCommand=0;
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
    return isValidCommand;
}


/*
*Controls if the req string is a valid HTTP response header.
*Returns 0 if req is an invalid header, a non-zero value if is a valid header.
*nLinesptr is a value of the lines number in the HEADER.
*ncharsptr is a value of the characters number in the HEADER.
*/
u_long validHTTPResponse(char *req,httpThreadContext* td,u_long* nLinesptr,u_long* ncharsptr)
{
	u_long i;
	u_long buffersize=td->buffersize;
	u_long nLinechars;
	int isValidCommand=0;
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
					isValidCommand=0;
					break;				
				}
				isValidCommand=1;
				break;
			}
			nLines++;
		}
		else
			nLinechars++;
		/*
		*We set a maximal theorical number of characters in a line.
		*If a line contains more than 4110 lines we consider the header invalid.
		*/
		if(nLinechars>4110)
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
	if(lserver->getVerbosity()>1)
	{
		/*
		*Record the error in the log file.
		*/
		sprintf(td->buffer,"%s redirected to %s\r\n",td->connection->ipAddr,newURL);
		((vhost*)td->connection->host)->warningsLogWrite(td->buffer);
	}
	sprintf(td->buffer2,"HTTP/1.1 302 Moved\r\nWWW-Authenticate: Basic\r\nAccept-Ranges: bytes\r\nServer: MyServer %s\r\nContent-type: text/html\r\nLocation: %s\r\nContent-length: 0\r\n",versionOfSoftware,newURL);
	strcat(td->buffer2,"Date: ");
	getRFC822GMTTime(&td->buffer2[strlen(td->buffer2)],HTTP_RESPONSE_DATE_DIM);
	strcat(td->buffer2,"\r\n\r\n");

	a->socket.send(td->buffer2,(int)strlen(td->buffer2),0);
	return 1;
}
/*
*Send a non-modified message to the client.
*/
int sendHTTPNonModified(httpThreadContext* td,LPCONNECTION a)
{
	if(lserver->getVerbosity()>1)
	{
		/*
		*Record the error in the log file.
		*/
		sprintf(td->buffer,"Not modified to %s\r\n",td->connection->ipAddr);
		((vhost*)td->connection->host)->warningsLogWrite(td->buffer);
	}
	sprintf(td->buffer2,"HTTP/1.1 304 Not Modified\r\nWWW-Authenticate: Basic\r\nAccept-Ranges: bytes\r\nServer: MyServer %s\r\nContent-type: text/html\r\nContent-length: 0\r\n",versionOfSoftware);
	strcat(td->buffer2,"Date: ");
	getRFC822GMTTime(&td->buffer2[strlen(td->buffer2)],HTTP_RESPONSE_DATE_DIM);
	strcat(td->buffer2,"\r\n\r\n");

	a->socket.send(td->buffer2,(int)strlen(td->buffer2),0);
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
	if(input==0)
		input=td->buffer;
	u_long validRequest=validHTTPRequest(input,td,&nLines,&maxTotchars);
	if(validRequest==0)/*Invalid header*/
		return 0;
	else if(validRequest==-1)/*Incomplete header*/
		return -1;

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
	if(!token)return 0;
	do
	{
		/*
		*Reset the flag lineControlled.
		*/
		lineControlled=0;

		/*
		*Copy the HTTP command.
		*/
		strcpy(command,token);
		
		nLineControlled++;
		if(nLineControlled==1)
		{
			/*
			*The first line has the form:
			*GET /index.html HTTP/1.1
			*/
			lineControlled=1;
			/*
			*Copy the method type.
			*/
			strncpy(request->CMD,command,HTTP_REQUEST_CMD_DIM);
			token = strtok( NULL, "\t\n\r" );
			if(!token)return 0;
			u_long len_token=(u_long)strlen(token);
			max=len_token;
			while((token[max]!=' ')&(len_token-max<HTTP_REQUEST_VER_DIM))
				max--;
			int containOpts=0;
			for(i=0;(i<max)&&(i<HTTP_REQUEST_URI_DIM);i++)
			{
				if(token[i]=='?')
				{
					containOpts=1;
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
			strncpy(request->VER,&token[max],HTTP_REQUEST_VER_DIM);

			if(request->URI[strlen(request->URI)-1]=='/')
				request->uriEndsWithSlash=1;
			else
				request->uriEndsWithSlash=0;
			StrTrim(request->URI," /");
			StrTrim(request->URIOPTS," /");
			max=(u_long)strlen(request->URI);
			if(max>max_URI)
			{
				return 414;
			}
		}else
		/*User-Agent*/
		if(!lstrcmpi(command,"User-Agent"))
		{
			token = strtok( NULL, "\r\n" );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->USER_AGENT,token,HTTP_REQUEST_USER_AGENT_DIM);
			StrTrim(request->USER_AGENT," ");
		}else
		/*Authorization*/
		if(!lstrcmpi(command,"Authorization"))
		{
			token = strtok( NULL, "\r\n" );
			if(!token)return 0;
			lineControlled=1;		
			td->buffer2[0]='\0';
			/*
			*Basic authorization in base64 is login:password.
			*Assume that it is Basic anyway.
			*/
			strcpy(request->AUTH,"Basic");
			char *base64=&token[strlen("Basic ")];
			int len=(int)strlen(base64);
			char* lbuffer2=base64Utils.Decode(base64,&len);
			char* keep_lbuffer2=lbuffer2;
			int i;
			for(i=0;(*lbuffer2!=':') && (i<19);i++)
			{
				td->connection->login[i]=*lbuffer2++;
				td->connection->login[i+1]='\0';
			}
			strcpy(td->identity,td->connection->login);
			lbuffer2++;
			for(i=0;(*lbuffer2)&&(i<31);i++)
			{
				td->connection->password[i]=*lbuffer2++;
				td->connection->password[i+1]='\0';
			}
			free(keep_lbuffer2);
		}else
		/*Host*/
		if(!lstrcmpi(command,"Host"))
		{
			token = strtok( NULL, seps );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->HOST,token,HTTP_REQUEST_HOST_DIM);
			StrTrim(request->HOST," ");
		}else
		/*Content-Encoding*/
		if(!lstrcmpi(command,"Content-Encoding"))
		{
			token = strtok( NULL, seps );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->CONTENT_ENCODING,token,HTTP_REQUEST_CONTENT_ENCODING_DIM);
			StrTrim(request->CONTENT_ENCODING," ");
		}else
		/*Content-Type*/
		if(!lstrcmpi(command,"Content-Type"))
		{
			token = strtok( NULL, seps );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->CONTENT_TYPE,token,HTTP_REQUEST_CONTENT_TYPE_DIM);
			StrTrim(request->CONTENT_TYPE," ");
		}else
		/*If-Modified-Since*/
		if(!lstrcmpi(command,"If-Modified-Since"))
		{
			token = strtok( NULL, seps );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->IF_MODIFIED_SINCE,token,HTTP_REQUEST_IF_MODIFIED_SINCE_DIM);
			StrTrim(request->IF_MODIFIED_SINCE," ");
		}else
		/*Accept*/
		if(!lstrcmpi(command,"Accept"))
		{
			token = strtok( NULL, "\n\r" );
			if(!token)return 0;
			lineControlled=1;
			strncat(request->ACCEPT,token,HTTP_REQUEST_ACCEPT_DIM-strlen(request->ACCEPT));
			StrTrim(request->ACCEPT," ");
		}else
		/*Accept-Language*/
		if(!lstrcmpi(command,"Accept-Language"))
		{
			token = strtok( NULL, "\n\r" );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->ACCEPTLAN,token,HTTP_REQUEST_ACCEPTLAN_DIM);
			StrTrim(request->ACCEPTLAN," ");
		}else
		/*Accept-charset*/
		if(!lstrcmpi(command,"Accept-charset"))
		{
			token = strtok( NULL, "\n\r" );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->ACCEPTCHARSET,token,HTTP_REQUEST_ACCEPTCHARSET_DIM);
			StrTrim(request->ACCEPTCHARSET," ");
		}else
		/*Accept-Encoding*/
		if(!lstrcmpi(command,"Accept-Encoding"))
		{
			token = strtok( NULL, "\n\r" );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->ACCEPTENC,token,HTTP_REQUEST_ACCEPTENC_DIM);
			StrTrim(request->ACCEPTENC," ");
		}else
		/*Connection*/
		if(!lstrcmpi(command,"Connection"))
		{
			token = strtok( NULL, "\n\r" );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->CONNECTION,token,HTTP_REQUEST_CONNECTION_DIM);
			StrTrim(request->CONNECTION," ");
		}else
		/*Cookie*/
		if(!lstrcmpi(command,"Cookie"))
		{
			token = strtok( NULL, "\n\r" );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->COOKIE,token,HTTP_REQUEST_COOKIE_DIM);
		}else
		/*From*/
		if(!lstrcmpi(command,"From"))
		{
			token = strtok( NULL, "\r\n" );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->FROM,token,HTTP_REQUEST_FROM_DIM);
			StrTrim(request->FROM," ");
		}else
		/*Content-Length*/
		if(!lstrcmpi(command,"Content-Length"))
		{
			token = strtok( NULL, "\n\r" );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->CONTENT_LENGTH,token,HTTP_REQUEST_CONTENT_LENGTH_DIM);
		}else
		/*Cache-Control*/
		if(!lstrcmpi(command,"Cache-Control"))
		{
			token = strtok( NULL, "\n\r" );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->CACHE_CONTROL,token,HTTP_REQUEST_CACHE_CONTROL_DIM);
		}else
		/*Range*/
		if(!lstrcmpi(command,"Range"))
		{
			request->RANGETYPE[0]='\0';
			request->RANGEBYTEBEGIN[0]='\0';
			request->RANGEBYTEEND[0]='\0';
			lineControlled=1;
			token = strtok( NULL, "\r\n\t" );
			if(!token)return 0;
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
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->REFERER,token,HTTP_REQUEST_REFERER_DIM);
			StrTrim(request->REFERER," ");
		}else
		/*Pragma*/
		if(!lstrcmpi(command,"Pragma"))
		{
			token = strtok( NULL, seps );
			if(!token)return 0;
			lineControlled=1;
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
			if(!token)return 0;
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
*Send a 401 error
*/
int sendAuth(httpThreadContext* td,LPCONNECTION s)
{
	if(s->nTries > 2)
	{
		if(lserver->getVerbosity()>1)
		{
			/*
			*Record the error in the log file.
			*/
			sprintf(td->buffer,"Request %s authorization\r\n",td->connection->ipAddr);
			((vhost*)td->connection->host)->warningsLogWrite(td->buffer);
		}
		return raiseHTTPError(td,s,e_401);
	}
	else
	{	
		if(lserver->getVerbosity()>1)
		{
			/*
			*Record the error in the log file.
			*/
			sprintf(td->buffer,"%s unauthorized\r\n",td->connection->ipAddr);
			((vhost*)td->connection->host)->warningsLogWrite(td->buffer);
		}
		s->nTries++;
		return raiseHTTPError(td,s,e_401AUTH);
	}
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


	if(input==0)
		input=td->buffer;
	/*
	*Control if the HTTP header is a valid header.
	*/
	if(input[0]==0)
		return 0;
	u_long nLines,maxTotchars;
	u_long validRequest=validHTTPResponse(input,td,&nLines,&maxTotchars);

	const char cmdseps[]   = ": ,\t\n\r\0";

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
		lineControlled=0;

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
			lineControlled=1;
			/*
			*Copy the HTTP version.
			*/
			strncpy(response->VER,command,HTTP_RESPONSE_VER_DIM);
		
			token = strtok( NULL, " ,\t\n\r" );
			response->httpStatus=atoi(token);
			
			token = strtok( NULL, "\r\n\0" );
			strncpy(response->ERROR_TYPE,token,HTTP_RESPONSE_ERROR_TYPE_DIM);

		}else
		/*Server*/
		if(!lstrcmpi(command,"Server"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			strncpy(response->SERVER_NAME,token,HTTP_RESPONSE_SERVER_NAME_DIM);
			StrTrim(response->SERVER_NAME," ");
		}else
		/*Location*/
		if(!lstrcmpi(command,"Location"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			strncpy(response->LOCATION,token,HTTP_RESPONSE_LOCATION_DIM);
			StrTrim(response->LOCATION," ");
		}else
		/*Last-Modified*/
		if(!lstrcmpi(command,"Last-Modified"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			strncpy(response->LAST_MODIFIED,token,HTTP_RESPONSE_LAST_MODIFIED_DIM);
			StrTrim(response->LAST_MODIFIED," ");
		}else
		/*Status*/
		if(!lstrcmpi(command,"Status"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			response->httpStatus=atoi(token);
		}else
		/*Content-Encoding*/
		if(!lstrcmpi(command,"Content-Encoding"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			strncpy(response->CONTENT_ENCODING,token,HTTP_RESPONSE_CONTENT_ENCODING_DIM);
			StrTrim(response->CONTENT_ENCODING," ");
		}else
		/*Cache-Control*/
		if(!lstrcmpi(command,"Cache-Control"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			strncpy(response->CACHE_CONTROL,token,HTTP_RESPONSE_CACHE_CONTROL_DIM);
		}else
		/*Date*/
		if(!lstrcmpi(command,"Date"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			strncpy(response->DATE,token,HTTP_RESPONSE_DATE_DIM);
			StrTrim(response->DATE," ");
		}else
		/*Content-Type*/
		if(!lstrcmpi(command,"Content-Type"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			strncpy(response->CONTENT_TYPE,token,HTTP_RESPONSE_CONTENT_TYPE_DIM);
			StrTrim(response->CONTENT_TYPE," ");
		}else
		/*MIME-Version*/
		if(!lstrcmpi(command,"MIME-Version"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			strncpy(response->MIMEVER,token,HTTP_RESPONSE_MIMEVER_DIM);
			StrTrim(response->MIMEVER," ");
		}else
		/*Set-Cookie*/
		if(!lstrcmpi(command,"Set-Cookie"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			strncat(response->COOKIE,token,HTTP_RESPONSE_COOKIE_DIM-strlen(response->COOKIE));
			strcat(response->COOKIE,"\n");/*Divide multiple cookies*/
		}else
		/*Content-Length*/
		if(!lstrcmpi(command,"Content-Length"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			strncpy(response->CONTENT_LENGTH,token,HTTP_RESPONSE_CONTENT_LENGTH_DIM);
			StrTrim(response->CONTENT_LENGTH," ");
		}else
		/*P3P*/
		if(!lstrcmpi(command,"P3P"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			strncpy(response->P3P,token,HTTP_RESPONSE_P3P_DIM);
			StrTrim(response->P3P," ");
		}else
		/*Connection*/
		if(!lstrcmpi(command,"Connection"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			strncpy(response->CONNECTION,token,HTTP_RESPONSE_CONNECTION_DIM);
			StrTrim(response->CONNECTION," ");
		}else
		/*Expires*/
		if(!lstrcmpi(command,"Expires"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
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
				if(response->OTHER[0])
					strncat(response->OTHER,"\r\n",HTTP_RESPONSE_OTHER_DIM-strlen(response->OTHER));
				strncat(response->OTHER,command,HTTP_RESPONSE_OTHER_DIM-strlen(response->OTHER));
				strncat(response->OTHER,": ",HTTP_RESPONSE_OTHER_DIM-strlen(response->OTHER));
				strncat(response->OTHER,token,HTTP_RESPONSE_OTHER_DIM-strlen(response->OTHER));
			}
		}
		token = strtok( NULL, cmdseps );
	}while((u_long)(token-input)<maxTotchars);
	/*
	*END REQUEST STRUCTURE BUILD.
	*/
	td->nBytesToRead=maxTotchars;
	return validRequest;
}
