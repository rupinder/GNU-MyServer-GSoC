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
#include "../include/http_headers.h"
#include "../include/cserver.h"
#include "../include/security.h"
#include "../include/AMMimeUtils.h"
#include "../include/cgi.h"
#include "../include/filemanager.h"
#include "../include/sockets.h"
#include "../include/winCGI.h"
#include "../include/fastCGI.h"
#include "../include/utility.h"
#include "../include/gzip.h"
#include "../include/md5.h"
#include "../include/isapi.h"
#include "../include/stringutils.h"
#define min(a,b)((a<b)?a:b)

extern "C" 
{
#ifdef WIN32
#include <direct.h>
#include <errno.h>
#endif

#ifdef NOT_WIN
#include <string.h>
#include <errno.h>
#endif
}

#ifdef NOT_WIN
#include "../include/lfind.h"
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

// Bloodshed Dev-C++ Helper
#ifndef intptr_t
#define intptr_t int
#endif

int http::mscgiLoaded=0;/*Store if the MSCGI library was loaded.*/
char http::browseDirCSSpath[MAX_PATH]="";
u_long http::gzip_threshold=0;
int http::useMessagesFiles=0;	
char *http::defaultFilename=0;	
u_long http::nDefaultFilename=0;

/*!
*Browse a folder printing its contents over the HTTP.
*/
int http::sendHTTPDIRECTORY(httpThreadContext* td,LPCONNECTION s,char* folder)
{
	/*!
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
				/*!
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
	sprintf(td->buffer2,"<html>\r\n<head>\r\n<title>%s</title>\r\n</head>\r\n",td->request.URI);
	outFile.writeToFile(td->buffer2,(u_long)strlen(td->buffer2),&nbw);
	/*!
	*If it is defined a CSS file for the graphic layout of the browse folder insert it in the page.
	*/
	if(browseDirCSSpath[0])
	{
		MYSERVER_FILE cssHandle;
		int ret;
		ret=cssHandle.openFile(browseDirCSSpath,MYSERVER_FILE_OPEN_IFEXISTS|MYSERVER_FILE_OPEN_READ);
		if(ret>0)
		{
			u_long nbr;
			cssHandle.readFromFile(td->buffer,td->buffersize,&nbr);
			strcpy(td->buffer2,"<style>\r\n<!--\r\n");
			outFile.writeToFile(td->buffer2,(u_long)strlen(td->buffer2),&nbw);
			outFile.writeToFile(td->buffer,(u_long)nbr,&nbw);
			strcpy(td->buffer2,"-->\r\n</style>\r\n");
			outFile.writeToFile(td->buffer2,(u_long)strlen(td->buffer2),&nbw);
			cssHandle.closeFile();

		}
	}

#ifdef WIN32
	sprintf(filename,"%s/*",folder);
#endif
#ifdef NOT_WIN
	sprintf(filename,"%s/",folder);
#endif
	strcpy(td->buffer2,"\r\n<body>\r\n<h1>");
	strcat(td->buffer2,"Contents of folder");
	strcat(td->buffer2," ");
	strcat(td->buffer2,&td->request.URI[startchar]);
	strcat(td->buffer2,"</h1>\r\n<p>\r\n<hr />\r\n");
	outFile.writeToFile(td->buffer2,(u_long)strlen(td->buffer2),&nbw);
	intptr_t ff;
	ff=(intptr_t)_findfirst(filename,&fd);

#ifdef WIN32
	if(ff==-1)
#endif
#ifdef NOT_WIN
	if((int)ff==-1)
#endif
	{
		if(errno==EACCES)
		{
			/*!
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
	/*!
	*With the current code we build the HTML TABLE to indicize the files in the folder.
	*/
	sprintf(td->buffer2,"<table width=\"100%%\">\r\n<tr>\r\n<td><b>%s</b></td>\r\n<td><b>%s</b></td>\r\n<td><b>%s</b></td>\r\n</tr>\r\n","File","Last modify","Size");
	outFile.writeToFile(td->buffer2,(u_long)strlen(td->buffer2),&nbw);
	static char fileSize[10];
	static char fileTime[32];
	do
	{	
		if(fd.name[0]=='.')
			continue;
		/*!
		*Do not show the security file
		*/
		if(!strcmp(fd.name,"security"))
			continue;
		strcpy(td->buffer2,"<tr>\r\n<td><a href=\"");
		if(!td->request.uriEndsWithSlash)
		{
			strcat(td->buffer2,&td->request.URI[startchar]);
			strcat(td->buffer2,"/");
		}
		strcat(td->buffer2,fd.name);
		strcat(td->buffer2,"\">");
		strcat(td->buffer2,fd.name);
		strcat(td->buffer2,"</td>\r\n<td>");
			
		tm *st=gmtime(&fd.time_write);
		
		getRFC822GMTTime((time_t)fd.time_write,fileTime,32);
						
		strcat(td->buffer2,fileTime);

		strcat(td->buffer2,"</td>\r\n<td>");
		if(fd.attrib & FILE_ATTRIBUTE_DIRECTORY)
		{
			strcat(td->buffer2,"[dir]");
		}
		else
		{
			sprintf(fileSize,"%i bytes",fd.size);
			strcat(td->buffer2,fileSize);
		}
		strcat(td->buffer2,"</td>\r\n</tr>\r\n");
		outFile.writeToFile(td->buffer2,(u_long)strlen(td->buffer2),&nbw);
	}while(!_findnext(ff,&fd));
	strcpy(td->buffer2,"</table>\r\n<hr />\r\n<address>");
	strcat(td->buffer2,"Running on");
	strcat(td->buffer2," MyServer ");
	strcat(td->buffer2,versionOfSoftware);
	strcat(td->buffer2,"</address>\r\n</body>\r\n</html>\r\n");
	outFile.writeToFile(td->buffer2,(u_long)strlen(td->buffer2),&nbw);
	_findclose(ff);
	/*!
	*Changes the \ character in the / character 
	*/
	char *buffer2Loop=td->buffer2;
	while(*buffer2Loop++)
		if(*buffer2Loop=='\\')
			*buffer2Loop='/';
	http_headers::buildDefaultHTTPResponseHeader(&(td->response));
	if(!lstrcmpi(td->request.CONNECTION,"Keep-Alive"))
		strcpy(td->response.CONNECTION,"Keep-Alive");
	sprintf(td->response.CONTENT_LENGTH,"%u",outFile.getFileSize());
	outFile.setFilePointer(0);
	http_headers::buildHTTPResponseHeader(td->buffer,&(td->response));
	
	s->socket.send(td->buffer,(u_long)strlen(td->buffer), 0);
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
/*!
*Send a file to the client using the HTTP.
*/
int http::sendHTTPFILE(httpThreadContext* td,LPCONNECTION s,char *filenamePath,int OnlyHeader,int firstByte,int lastByte)
{
	/*!
	*With this routine we send a file through the HTTP protocol.
	*Open the file and save its handle.
	*/
	int ret;
	int useGZIP=0;/*Will we use GZIP compression to send data?*/
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
	/*!
	*If the file is a valid handle.
	*/
	u_long bytesToSend=h.getFileSize();
	if(lastByte == -1)
	{
		lastByte=bytesToSend;
		
		if(bytesToSend > gzip_threshold)/*Use GZIP compression to send files bigger than GZIP threshold*/
		{
			useGZIP=1;
		}
	}
	else/*!If the client use ranges set the right value for the last byte number*/
	{
		lastByte=min((u_long)lastByte,bytesToSend);
	}
	uint keepalive = lstrcmpi(td->request.CONNECTION,"Keep-Alive")==0;
	/*
	*Be sure that client accept GZIP compressed data.
	*/
#ifndef DO_NOT_USE_GZIP		
	if(useGZIP)
		useGZIP &= (strstr(td->request.ACCEPTENC,"gzip")!=0);
#else
	/*
	*If compiled without GZIP support force the server to don't use it.
	*/
	useGZIP=0;
#endif	

	/*!
	*bytesToSend is the interval between the first and the last byte.
	*/
	bytesToSend=lastByte-firstByte;

	/*!
	*If failed to set the file pointer returns an internal server error.
	*/
	if(h.setFilePointer(firstByte))
	{
		h.closeFile();
		return raiseHTTPError(td,s,e_500);
	}

	td->buffer[0]='\0';
	/*!If a Range was requested send 206 and not 200 for success*/
	if((lastByte == -1)|(firstByte))
		td->response.httpStatus = 206;
	if(keepalive)
		sprintf(td->response.CONTENT_LENGTH,"%u",bytesToSend);
	else
		strcpy(td->response.CONNECTION,"Close");
	
	if(useGZIP)
	{
		if(keepalive)/*Do not use chunked transfer with old 1.0 clients*/
			strcpy(td->response.TRANSFER_ENCODING,"chunked");
		strcpy(td->response.CONTENT_ENCODING,"gzip");
	}
	http_headers::buildHTTPResponseHeader(td->buffer,&td->response);
	/*!
	*Send the HTTP header
	*/
	if(s->socket.send(td->buffer,(u_long)strlen(td->buffer), 0)== SOCKET_ERROR)
	{
		h.closeFile();
		return 0;
	}
	
	/*!
	*If is requested only the header exit from the function; used by the HEAD request.
	*/
	if(OnlyHeader)
	{
		h.closeFile();
		return 1;
	}
	/*!
	*Is the GZIP header still added to the buffer?
	*/
	u_long gzipheaderadded=0;
	/*!
	*gzip compression manager
	*/
	gzip gzip;
	/*
	*Number of bytes created by the zip compressor by loop.
	*/
	u_long gzip_dataused=0;
	if(useGZIP)
		gzip.gzip_initialize(td->buffer2,td->buffersize2,td->buffer,td->buffersize);
	for(;;)
	{
		u_long nbr;

		if(useGZIP)
		{
			gzip_dataused=0;
			u_long datatoread=min(bytesToSend,td->buffersize2/2);
			/*!
			*Read from the file the bytes to send.
			*/
			h.readFromFile(td->buffer2,datatoread,&nbr);

			if(nbr)
			{
				if(gzipheaderadded==0)
				{
					gzip_dataused+=gzip.gzip_getHEADER(td->buffer,td->buffersize);
					gzipheaderadded=1;
				}
				gzip_dataused+=gzip.gzip_compress(td->buffer2,nbr,&td->buffer[gzip_dataused],td->buffersize-gzip_dataused);
			}
			else
			{
				gzip_dataused=gzip.gzip_flush(td->buffer,td->buffersize);
				gzip.gzip_free(td->buffer2,nbr,td->buffer,td->buffersize);
			}


		}
		else
		{
			/*!
			*Read from the file the bytes to send.
			*/
			h.readFromFile(td->buffer,min(bytesToSend,td->buffersize),&nbr);
		}
		if(useGZIP)
		{
			char chunksize[12];
			if(keepalive)
			{
				sprintf(chunksize,"%x\r\n",gzip_dataused);
				if(s->socket.send(chunksize,(int)strlen(chunksize), 0) == SOCKET_ERROR)
					break;
			}
			if(gzip_dataused)
				if(s->socket.send(td->buffer,gzip_dataused, 0) == SOCKET_ERROR)
					break;
			if(keepalive)
				s->socket.send("\r\n",2, 0);
		}
		else
		{
			/*!
			*If there are bytes to send, send them.
			*/
			if(nbr)
			{
				if(s->socket.send(td->buffer,nbr, 0) == SOCKET_ERROR)
					break;		
			}
		}
		/*!
		*When the bytes number read from the file is zero, stop to send the file.
		*/
		if((nbr==0) || (gzip_dataused==GZIP_FOOTER_LENGTH))
		{
			if(keepalive)
				s->socket.send("0\r\n\r\n",5, 0);
			break;
		}
	}

	h.closeFile();
	return keepalive;

}
/*!
*Main function to handle the HTTP PUT command.
*/
int http::putHTTPRESOURCE(httpThreadContext* td,LPCONNECTION s,char *filename,int /*!systemrequest*/,int,int firstByte,int /*!lastByte*/,int yetmapped)
{
	int httpStatus=td->response.httpStatus;
	http_headers::buildDefaultHTTPResponseHeader(&td->response);
	uint keepalive=0;
	if(!lstrcmpi(td->request.CONNECTION,"Keep-Alive"))
	{
		strcpy(td->response.CONNECTION,"Keep-Alive");
		keepalive=1;
	}
	td->response.httpStatus=httpStatus;
	/*!
	*td->filenamePath is the file system mapped path while filename is the URI requested.
	*systemrequest is 1 if the file is in the system folder.
	*If filename is already mapped on the file system don't map it again.
	*/
	if(yetmapped)
	{
		strncpy(td->filenamePath,filename,MAX_PATH);
	}
	else
	{
		/*!
		*If the client try to access files that aren't in the web folder send a 401 error.
		*/
		translateEscapeString(filename );
		if((filename[0] != '\0')&&(MYSERVER_FILE::getPathRecursionLevel(filename)<1))
		{
		
			return raiseHTTPError(td,s,e_401);
		}
		getPath(td,s,td->filenamePath,filename,0);
	}
	int permissions=-1;
	char folder[MAX_PATH];
	if(MYSERVER_FILE::isFolder(td->filenamePath))
		strncpy(folder,td->filenamePath,MAX_PATH);
	else
		MYSERVER_FILE::splitPath(td->filenamePath,folder,filename);
	if(s->protocolBuffer==0)
	{
		s->protocolBuffer=malloc(sizeof(http_user_data));
		if(!s->protocolBuffer)
			return 0;
		resetHTTPUserData((http_user_data*)(s->protocolBuffer));
	}
	int permissions2=0;
	char auth_type[16];	
	if(td->request.AUTH[0])
		permissions=getPermissionMask(s->login,s->password,folder,filename,((vhost*)(s->host))->systemRoot,((http_user_data*)s->protocolBuffer)->needed_password,auth_type,16,&permissions2);
	else/*!The default user is Guest with a null password*/
		permissions=getPermissionMask("Guest","",folder,filename,((vhost*)(s->host))->systemRoot,((http_user_data*)s->protocolBuffer)->needed_password,auth_type,16);

	if(!lstrcmpi(auth_type,"Digest"))/*Check if we have to use digest for the current folder*/
	{
		if(!lstrcmpi(td->request.AUTH,"Digest"))
		{
			if(!((http_user_data*)s->protocolBuffer)->digest_checked)
				((http_user_data*)s->protocolBuffer)->digest = checkDigest(td,s);
			((http_user_data*)s->protocolBuffer)->digest_checked=1;
								
			if(((http_user_data*)s->protocolBuffer)->digest==1)
			{
				strcpy(s->password,((http_user_data*)s->protocolBuffer)->needed_password);
				permissions=permissions2;
			}
		}
		td->auth_scheme=HTTP_AUTH_SCHEME_DIGEST;
	}
	else/*By default use the Basic authentication scheme*/
	{
		td->auth_scheme=HTTP_AUTH_SCHEME_BASIC;
	}	
	/*If there are no permissions, use the Guest permissions*/
	if(td->request.AUTH[0] && (permissions==0))
		permissions=getPermissionMask("Guest","",folder,filename,((vhost*)(s->host))->systemRoot,((http_user_data*)s->protocolBuffer)->needed_password,auth_type,16);		

	if(!(permissions & MYSERVER_PERMISSION_WRITE))
	{
		return sendAuth(td,s);
	}
	if(MYSERVER_FILE::fileExists(td->filenamePath))
	{
		/*!
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
				return raiseHTTPError(td,s,e_500);/*!Internal server error*/
			}
		}
		file.closeFile();
		raiseHTTPError(td,s,e_200);/*!Successful updated*/
		return keepalive;
	}
	else
	{
		/*!
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
				return raiseHTTPError(td,s,e_500);/*!Internal server error*/
			}
		}
		file.closeFile();
		raiseHTTPError(td,s,e_201);/*!Successful created*/
		return 1;
	}
	
}
/*!
*Delete the resource identified by filename
*/
int http::deleteHTTPRESOURCE(httpThreadContext* td,LPCONNECTION s,char *filename,int yetmapped)
{

	int httpStatus=td->response.httpStatus;
	http_headers::buildDefaultHTTPResponseHeader(&td->response);
	if(!lstrcmpi(td->request.CONNECTION,"Keep-Alive"))
	{
		strcpy(td->response.CONNECTION,"Keep-Alive");
	}
	td->response.httpStatus=httpStatus;
	/*!
	*td->filenamePath is the file system mapped path while filename is the URI requested.
	*systemrequest is 1 if the file is in the system folder.
	*If filename is already mapped on the file system don't map it again.
	*/
	if(yetmapped)
	{
		strncpy(td->filenamePath,filename,MAX_PATH);
	}
	else
	{
		/*!
		*If the client try to access files that aren't in the web folder send a 401 error.
		*/
		translateEscapeString(filename );
		if((filename[0] != '\0')&&(MYSERVER_FILE::getPathRecursionLevel(filename)<1))
		{
			return raiseHTTPError(td,s,e_401);
		}
		getPath(td,s,td->filenamePath,filename,0);
	}
	int permissions=-1;
	char folder[MAX_PATH];
	if(MYSERVER_FILE::isFolder(td->filenamePath))
		strncpy(folder,td->filenamePath,MAX_PATH);
	else
		MYSERVER_FILE::splitPath(td->filenamePath,folder,filename);

	if(s->protocolBuffer==0)
	{
		s->protocolBuffer=malloc(sizeof(http_user_data));
		if(!s->protocolBuffer)
			return 0;
		resetHTTPUserData((http_user_data*)(s->protocolBuffer));
	}
	int permissions2=0;
	char auth_type[16];
	
	if(td->request.AUTH[0])
		permissions=getPermissionMask(s->login,s->password,folder,filename,((vhost*)(s->host))->systemRoot,((http_user_data*)s->protocolBuffer)->needed_password,auth_type,16,&permissions2);
	else/*!The default user is Guest with a null password*/
		permissions=getPermissionMask("Guest","",folder,filename,((vhost*)(s->host))->systemRoot,((http_user_data*)s->protocolBuffer)->needed_password,auth_type,16);
		

	if(!lstrcmpi(auth_type,"Digest"))/*Check if we have to use digest for the current folder*/
	{
		if(!lstrcmpi(td->request.AUTH,"Digest"))
		{
			if(!((http_user_data*)s->protocolBuffer)->digest_checked)
				((http_user_data*)s->protocolBuffer)->digest = checkDigest(td,s);
			((http_user_data*)s->protocolBuffer)->digest_checked=1;
			if(((http_user_data*)s->protocolBuffer)->digest==1)
			{
				strcpy(s->password,((http_user_data*)s->protocolBuffer)->needed_password);
				permissions=permissions2;
			}
		}
		td->auth_scheme=HTTP_AUTH_SCHEME_DIGEST;
	}
	else/*By default use the Basic authentication scheme*/
	{
		td->auth_scheme=HTTP_AUTH_SCHEME_BASIC;
	}	
	/*If there are no permissions, use the Guest permissions*/
	if(td->request.AUTH[0] && (permissions==0))
		permissions=getPermissionMask("Guest","",folder,filename,((vhost*)(s->host))->systemRoot,((http_user_data*)s->protocolBuffer)->needed_password,auth_type,16);		

	if(!(permissions & MYSERVER_PERMISSION_DELETE))
	{
		return sendAuth(td,s);
	}
	if(MYSERVER_FILE::fileExists(td->filenamePath))
	{
		MYSERVER_FILE::deleteFile(td->filenamePath);
		return raiseHTTPError(td,s,e_202);/*!Successful deleted*/
	}
	else
	{
		return raiseHTTPError(td,s,e_204);/*!No content*/
	}
}
/*!
*Check the Digest authorization
*/
u_long http::checkDigest(httpThreadContext* td,LPCONNECTION s)
{
	if(lstrcmp(td->request.digest_opaque,((http_user_data*)s->protocolBuffer)->opaque))/*If is not equal return 0*/
		return 0;
	
	if(lstrcmp(td->request.digest_realm,((http_user_data*)s->protocolBuffer)->realm))/*If is not equal return 0*/
		return 0;
	
	u_long digest_count = hexToInt(td->request.digest_nc);
	
	if(digest_count != ((http_user_data*)s->protocolBuffer)->nc+1)
		return 0;
	else
		((http_user_data*)s->protocolBuffer)->nc++;

	char A1[48];
	char A2[48];
	char response[48];
	   
   	MYSERVER_MD5Context md5;
	MYSERVER_MD5Init(&md5);
	sprintf(td->buffer2,"%s:%s:%s",td->request.digest_username,td->request.digest_realm,((http_user_data*)s->protocolBuffer)->needed_password);
	MYSERVER_MD5Update(&md5,(const unsigned char*)td->buffer2,(u_int)strlen(td->buffer2));
	MYSERVER_MD5End(&md5,A1);
	
	MYSERVER_MD5Init(&md5);
	char *method=td->request.CMD;
	char *uri=td->request.URIOPTS;
	if(td->request.digest_uri[0])
		uri=td->request.digest_uri;
	sprintf(td->buffer2,"%s:%s",method,uri);
	MYSERVER_MD5Update(&md5,(const unsigned char*)td->buffer2,(u_int)strlen(td->buffer2));
	MYSERVER_MD5End(&md5,A2);
	
	MYSERVER_MD5Init(&md5);
	sprintf(td->buffer2,"%s:%s:%s:%s:%s:%s",A1,((http_user_data*)s->protocolBuffer)->nonce,td->request.digest_nc,td->request.digest_cnonce,td->request.digest_qop,A2);
	MYSERVER_MD5Update(&md5,(const unsigned char*)td->buffer2,(u_int)strlen(td->buffer2));
	MYSERVER_MD5End(&md5,response);	

	if(!lstrcmp(response,td->request.digest_response))
		return 1;
}
/*!
*Reset an http_user_data structure.
*/
void http::resetHTTPUserData(http_user_data* ud)
{
	ud->realm[0]='\0';
	ud->opaque[0]='\0';
	ud->nonce[0]='\0';
	ud->cnonce[0]='\0';
	ud->needed_password[0]='\0';
	ud->nc=0;
	ud->digest=0;
}
/*!
*Main function to send a resource to a client.
*/
int http::sendHTTPRESOURCE(httpThreadContext* td,LPCONNECTION s,char *URI,int systemrequest,int OnlyHeader,int firstByte,int lastByte,int yetmapped)
{
	/*!
	*With this code we manage a request of a file or a folder or anything that we must send
	*over the HTTP.
	*/
	char filename[MAX_PATH];
	strncpy(filename,URI,MAX_PATH);
	td->buffer[0]='\0';
	if(!systemrequest)
	{
		http_headers::buildDefaultHTTPResponseHeader(&td->response);
	}
	else
	{
		int httpStatus=td->response.httpStatus;
		http_headers::buildDefaultHTTPResponseHeader(&td->response);
		td->response.httpStatus=httpStatus;
	}
	uint keepalive=0;
	if(!lstrcmpi(td->request.CONNECTION,"Keep-Alive"))
	{
		strcpy(td->response.CONNECTION,"Keep-Alive");
		keepalive=1;
	}
	static char ext[10];
	static char data[MAX_PATH];
	/*!
	*td->filenamePath is the file system mapped path while filename is the URI requested.
	*systemrequest is 1 if the file is in the system folder.
	*If filename is already mapped on the file system don't map it again.
	*/
	if(yetmapped)
	{
		strncpy(td->filenamePath,filename,MAX_PATH);
	}
	else
	{
		/*!
		*If the client try to access files that aren't in the web folder send a 401 error.
		*/
		translateEscapeString(filename );
		if((filename[0] != '\0')&&(MYSERVER_FILE::getPathRecursionLevel(filename)<1))
		{
			return raiseHTTPError(td,s,e_401);
		}
		getPath(td,s,td->filenamePath,filename,systemrequest);
	}
	int permissions=-1;/*!By default everything is permitted*/
	if(!systemrequest)
	{
		char folder[MAX_PATH];
		char auth_type[16];
		if(MYSERVER_FILE::isFolder(td->filenamePath))
			strncpy(folder,td->filenamePath,MAX_PATH);
		else
			MYSERVER_FILE::splitPath(td->filenamePath,folder,filename);

		if(s->protocolBuffer==0)
		{
			s->protocolBuffer=malloc(sizeof(http_user_data));
			if(!s->protocolBuffer)
				return 0;
			resetHTTPUserData((http_user_data*)(s->protocolBuffer));
		}
		int permissions2=0;
		if(td->request.AUTH[0])
			permissions=getPermissionMask(s->login,s->password,folder,filename,((vhost*)(s->host))->systemRoot,((http_user_data*)s->protocolBuffer)->needed_password,auth_type,16,&permissions2);
		else/*!The default user is Guest with a null password*/
			permissions=getPermissionMask("Guest","",folder,filename,((vhost*)(s->host))->systemRoot,((http_user_data*)s->protocolBuffer)->needed_password,auth_type,16);
			

		if(!lstrcmpi(auth_type,"Digest"))/*Check if we have to use digest for the current folder*/
		{
			if(!lstrcmpi(td->request.AUTH,"Digest"))
			{
				if(!((http_user_data*)s->protocolBuffer)->digest_checked)
					((http_user_data*)s->protocolBuffer)->digest = checkDigest(td,s);
				((http_user_data*)s->protocolBuffer)->digest_checked=1;
				if(((http_user_data*)s->protocolBuffer)->digest==1)
				{
					strcpy(s->password,((http_user_data*)s->protocolBuffer)->needed_password);
					permissions=permissions2;
				}
			}
			td->auth_scheme=HTTP_AUTH_SCHEME_DIGEST;
		}
		else/*By default use the Basic authentication scheme*/
		{
			td->auth_scheme=HTTP_AUTH_SCHEME_BASIC;
		}	
		/*If there are no permissions, use the Guest permissions*/
		if(td->request.AUTH[0] && (permissions==0))
			permissions=getPermissionMask("Guest","",folder,filename,((vhost*)(s->host))->systemRoot,((http_user_data*)s->protocolBuffer)->needed_password,auth_type,16);		
	}
	
	/*!
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
		/*!
		*http://host/pathtofile/filetosend.php/PATH_INFO_VALUE?QUERY_INFO_VALUE
		*When a request has this form send the file filetosend.php with the
		*environment string PATH_INFO equals to PATH_INFO_VALUE and QUERY_INFO
		*to QUERY_INFO_VALUE.
		*/
		if(i && (td->filenamePath[i]=='/'))/*!There is the '/' character check if dirscan is a file*/
		{
			if(!MYSERVER_FILE::isFolder(dirscan))
			{
				/*!
				*If the token is a file.
				*/
				strncpy(td->pathInfo,&td->filenamePath[len],MAX_PATH);
				strncpy(td->filenamePath,dirscan,MAX_PATH);
				break;
			}
		}
		if(len+1<MAX_PATH)
		{
			dirscan[len++]=(td->filenamePath)[i];
			dirscan[len]='\0';
		}
	}
	
	/*!
	*If there is a PATH_INFO value the get the PATH_TRANSLATED too.
	*PATH_TRANSLATED is the mapped to the local filesystem version of PATH_INFO.
	*/
	if(td->pathInfo[0])
	{
        td->pathTranslated[0]='\0';
		/*!
		*Start from the second character because the first is a slash character.
		*/
		getPath(td,s,(td->pathTranslated),&((td->pathInfo)[1]),0);
		MYSERVER_FILE::completePath(td->pathTranslated);
	}
	else
	{
        td->pathTranslated[0]='\0';
	}
	MYSERVER_FILE::completePath(td->filenamePath);

	/*!
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
			char *defaultFileNamePath=getDefaultFilenamePath(i);
			if(defaultFileNamePath)
				sprintf(defaultFileName,"%s/%s",td->filenamePath,defaultFileNamePath);
			else
				break;
			if(MYSERVER_FILE::fileExists(defaultFileName))
			{
				/*!
				*Change the URI to reflect the default file name.
				*/
				char nURL[MAX_PATH+HTTP_REQUEST_URI_DIM+12];
				strcpy(nURL,protocolPrefix);
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
					sprintf(&nURL[strlen(nURL)],":%u",((vhost*)s->host)->port);
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
					return keepalive;
				else
					return 0;
			}
		}

		if(sendHTTPDIRECTORY(td,s,td->filenamePath))
			return keepalive;
		return raiseHTTPError(td,s,e_404);
	}

	if(!MYSERVER_FILE::fileExists(td->filenamePath))
		return raiseHTTPError(td,s,e_404);

	/*!
	*getMIME returns the type of command registered by the extension.
	*/
	int mimeCMD=getMIME(td->response.CONTENT_TYPE,td->filenamePath,ext,data);
	if((mimeCMD==CGI_CMD_RUNCGI)||(mimeCMD==CGI_CMD_EXECUTE))
	{
		if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
		{
			return sendAuth(td,s);
		}
		return lcgi.sendCGI(td,s,td->filenamePath,ext,data,mimeCMD);
	}else if(mimeCMD==CGI_CMD_RUNISAPI)
	{
#ifdef WIN32
		if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
		{
			return sendAuth(td,s);
		}
		return lisapi.sendISAPI(td,s,td->filenamePath,ext,data,0);
#endif
#ifdef NOT_WIN
		return raiseHTTPError(td,s,e_501);
#endif
	}else if(mimeCMD==CGI_CMD_EXECUTEISAPI)
	{
#ifdef WIN32
		if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
		{
			return sendAuth(td,s);
		}
		return lisapi.sendISAPI(td,s,td->filenamePath,ext,data,1);
#endif
#ifdef NOT_WIN
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
		if(mscgiLoaded)
		{
			if(lmscgi.sendMSCGI(td,s,td->filenamePath,target))
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
	
		int ret=lwincgi.sendWINCGI(td,s,cgipath);
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
		return (ret&keepalive);

	}
	else if(mimeCMD==CGI_CMD_RUNFASTCGI)
	{
		if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
		{
			return sendAuth(td,s);
		}	
		int ret = lfastcgi.sendFASTCGI(td,s,td->filenamePath,ext,data,0);
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
		return (ret&keepalive);
	}
	else if(mimeCMD==CGI_CMD_EXECUTEFASTCGI)
	{
		if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
		{
			return sendAuth(td,s);
		}
		int ret = lfastcgi.sendFASTCGI(td,s,td->filenamePath,ext,data,1);
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
		return (ret&keepalive);
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
		strncpy(pathInfo,td->pathInfo,MAX_PATH);
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
	getRFC822GMTTime(lastMT,td->response.LAST_MODIFIED,HTTP_RESPONSE_LAST_MODIFIED_DIM);
	if(td->request.IF_MODIFIED_SINCE[0])
	{
		if(!strcmp(td->request.IF_MODIFIED_SINCE,td->response.LAST_MODIFIED))
			return sendHTTPNonModified(td,s);
	}
	if(sendHTTPFILE(td,s,td->filenamePath,OnlyHeader,firstByte,lastByte))
		return keepalive;
	return sendHTTPhardError500(td,s);
	
}
/*!
*Log the access using the Common Log Format
*/
int http::logHTTPaccess(httpThreadContext* td,LPCONNECTION a)
{
	strncpy(td->buffer2,a->ipAddr,td->buffersize2);
	strcat(td->buffer2," ");
	
	if(td->identity[0])
		strcat(td->buffer2,td->identity);
	else
		strcat(td->buffer2,"-");

	strcat(td->buffer2," ");

	if(td->identity[0])
		strcat(td->buffer2,td->identity);
	else
		strcat(td->buffer2,"-");

	strcat(td->buffer2," [");

	getRFC822GMTTime(&td->buffer2[strlen(td->buffer2)],HTTP_RESPONSE_DATE_DIM);
	strcat(td->buffer2,"] \"");
	strcat(td->buffer2,td->request.CMD);
	strcat(td->buffer2," ");
	strcat(td->buffer2,td->request.URI);
	if(td->request.URIOPTS[0])
	{
		strcat(td->buffer2,"?");
		strcat(td->buffer2,td->request.URIOPTS);
	}
	strcat(td->buffer2,td->request.VER);
	strcat(td->buffer2,"\" ");

	sprintf(&td->buffer2[strlen(td->buffer2)],"%i",td->response.httpStatus);

	strcat(td->buffer2," ");
	
	if(td->response.CONTENT_LENGTH[0] && (strlen(td->response.CONTENT_LENGTH)<HTTP_RESPONSE_CONTENT_LENGTH_DIM))
		strcat(td->buffer2,td->response.CONTENT_LENGTH);
	else
		strcat(td->buffer2,"0");
	strcat(td->buffer2,"\r\n");
	/*
	*Request the access to the log file then write then append the message
	*/
	((vhost*)(a->host))->accesseslogRequestAccess(td->id);
	((vhost*)(a->host))->accessesLogWrite(td->buffer2);
	((vhost*)(a->host))->accesseslogTerminateAccess(td->id);
	td->buffer2[0]='\0';
	return 1;
}

/*!
*This is the HTTP protocol main procedure to parse a request made over the HTTP.
*/
int http::controlConnection(LPCONNECTION a,char *b1,char *b2,int bs1,int bs2,u_long nbtr,u_long id)
{
	int retvalue=-1;
	td.buffer=b1;
	td.buffer2=b2;
	td.buffersize=bs1;
	td.buffersize2=bs2;
	td.nBytesToRead=nbtr;
	td.identity[0]='\0';
	td.connection=a;
	td.id=id;
	td.lhttp=this;
	td.inputData.setHandle((MYSERVER_FILE_HANDLE)0);
	td.outputData.setHandle((MYSERVER_FILE_HANDLE)0);
	td.outputDataPath[0]='\0';
	td.inputDataPath[0]='\0';
	/*!
	*Reset the request structure.
	*/
	http_headers::resetHTTPRequest(&td.request);

	
	/*
	*If the connection must be removed, remove it.
	*/
	if(td.connection->toRemove)
	{
		switch(td.connection->toRemove)
		{
			case CONNECTION_REMOVE_OVERLOAD:
				retvalue = raiseHTTPError(&td,a,e_503);
				logHTTPaccess(&td,a);						
				return 0;/*Remove the connection from the list*/
		}
	}
	
	
	u_long validRequest=http_headers::buildHTTPRequestHeaderStruct(&td.request,&td);
	if(validRequest==-1)/*!If the header is incomplete returns 2*/
	{
		if(!strcmp(td.request.VER,"HTTP/1.1"))/*Be sure that the client can handle the 100 status code*/
		{
			char* msg="HTTP/1.1 100 Continue\r\n\r\n";
			a->socket.send(msg,(int)strlen(msg),0);
		}
		logHTTPaccess(&td,a);
		return 2;
	}
	if(a->protocolBuffer)
		((http_user_data*)a->protocolBuffer)->digest_checked=0;	
	td.nBytesToRead+=a->dataRead;/*Offset to the buffer after the HTTP header.*/
	/*!
	*If the header is an invalid request send the correct error message to the client and return immediately.
	*/
	if(validRequest==0)
	{
		retvalue = raiseHTTPError(&td,a,e_400);
		logHTTPaccess(&td,a);
		return retvalue;
	}/*!If the URI is too long*/
	else if(validRequest==414)
	{
		retvalue = raiseHTTPError(&td,a,e_414);
		logHTTPaccess(&td,a);
		return retvalue;
	}
	/*Do not use Keep-Alive over HTTP version older than 1.1*/
	if(!strcmp(td.request.VER,"HTTP/1.1"))
	{
		if(td.request.CONNECTION[0])
		strcpy(td.request.CONNECTION,"Close");
	}
		
	u_long content_len=0;/*!POST data size if any*/
	
	/*!
	*For methods that accept data after the HTTP header set the correct pointer and create a file
	*containing the informations after the header.
	*/
	getdefaultwd(td.inputDataPath,MAX_PATH);
	sprintf(&td.inputDataPath[(u_long)strlen(td.inputDataPath)],"/stdInFile_%u",(u_long)td.id);
	getdefaultwd(td.outputDataPath,MAX_PATH);
	sprintf(&td.outputDataPath[(u_long)strlen(td.outputDataPath)],"/stdOutFile_%u",(u_long)td.id);
	if((!lstrcmpi(td.request.CMD,"POST"))||(!lstrcmpi(td.request.CMD,"PUT")))
	{
		if(td.request.CONTENT_TYPE[0]=='\0')
			strcpy(td.request.CONTENT_TYPE,"application/x-www-form-urlencoded");

		/*!
		*Read POST data
		*/
		{
			td.request.URIOPTSPTR=&td.buffer[td.nHeaderChars];
			td.buffer[min(td.nBytesToRead,td.buffersize)]='\0';
			/*!
			*Create the file that contains the data posted.
			*This data is the stdin file in the CGI.
			*/
			td.inputData.openFile(td.inputDataPath,MYSERVER_FILE_CREATE_ALWAYS|MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_WRITE);
			u_long nbw;
			u_long total_nbr=min(td.nBytesToRead,td.buffersize)-td.nHeaderChars;
			
			td.inputData.writeToFile(td.request.URIOPTSPTR,total_nbr,&nbw);
			
			content_len=atoi(td.request.CONTENT_LENGTH);
			/*!
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
					retvalue = raiseHTTPError(&td,a,e_400);
					logHTTPaccess(&td,a);
					return retvalue;
				}
			}
			/*!
			*If there are others bytes to read from the socket.
			*/
			u_long timeout=get_ticks();
			if((content_len)&&(content_len!=nbw))
			{
				int err;
				u_long fs;
				do
				{
					err=0;
					fs=td.inputData.getFileSize();
					while(get_ticks()-timeout<SEC(5))
					{
						if(content_len==total_nbr)
						{
							/*!
							*Consider only CONTENT-LENGTH bytes of data.
							*/
							while(td.connection->socket.bytesToRead())
							{
								/*!
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
							timeout=get_ticks();
							break;
						}
					}
					if(get_ticks()-timeout>=SEC(5))
						break;
				}
				while(content_len!=total_nbr);

				fs=td.inputData.getFileSize();
				if(content_len!=fs)
				{
					/*!
					If we get an error remove the file and the connection.
					*/
					td.inputData.closeFile();
					td.inputData.deleteFile(td.inputDataPath);
					retvalue = raiseHTTPError(&td,a,e_500);
					logHTTPaccess(&td,a);
					return retvalue;

				}
			}
			else if(content_len==0)/*!If CONTENT-LENGTH is not specified read all the data*/
			{
				int err;
				do
				{
					err=0;
					while(get_ticks()-timeout<SEC(3))
					{
						if(td.connection->socket.bytesToRead())
						{				
							err=td.connection->socket.recv(td.buffer2,td.buffersize2, 0);
							td.inputData.writeToFile(td.buffer2,err,&nbw);	
							total_nbr+=nbw;
							timeout=get_ticks();
							break;
						}
					}
					if(get_ticks()-timeout>=SEC(3))
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
	/*
	*Manage chunked transfers
	*/
	if(!lstrcmpi(td.request.TRANSFER_ENCODING,"chunked"))
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
				if(nbr!=1)
					break;
				if((c!='\r') && (bufferlen<19))
				{
					buffer[bufferlen++]=c;
					buffer[bufferlen]='\0';
				}
				else
					break;
			}
			td.inputData.readFromFile(&c,1,&nbr);/*!Read the \n char too*/
			u_long dataToRead=(u_long)hexToInt(buffer);
			if(dataToRead==0)/*!The last chunk length is 0*/
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
		/*!
		*Now replace the file with the not-chunked one.
		*/
		td.inputData.closeFile();
		td.inputData.deleteFile(td.inputData.getFilename());
		td.inputData=newStdIn;
	}else	
	/*
	*If is specified another Transfer Encoding not supported by the server send a 501 error.
	*/
	if(strlen(td.request.TRANSFER_ENCODING))
	{
			raiseHTTPError(&td,a,e_501);
			if(td.inputData.getHandle())
			{
				td.inputData.closeFile();
			}
			logHTTPaccess(&td,a);
			return 0;		
	}

	if(retvalue==-1)/*!If return value is not configured propertly.*/
	{
		/*!
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
			logHTTPaccess(&td,a);
			return 0;
		}
		else
		{
			/*!
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
		
		if(!lstrcmpi(td.request.CONNECTION,"Keep-Alive")) 
		{
			/*!
			*Support for HTTP pipelining.
			*/
			if(content_len==0)
			{
				a->dataRead=min(KB(8),(u_int)strlen(&td.buffer[td.nHeaderChars]));
				if(a->dataRead)
				{
					memcpy(a->connectionBuffer,&td.buffer[td.nHeaderChars],a->dataRead+1);
					retvalue=3;
				}
				else
					retvalue=1;
				
			}	
			else
				retvalue=1;
		}
		else
			retvalue=0;

		/*!
		*Here we control all the HTTP commands.
		*/
		if(!lstrcmpi(td.request.CMD,"GET"))/*!GET REQUEST*/
		{
			if(!lstrcmpi(td.request.RANGETYPE,"bytes"))
				sendHTTPRESOURCE(&td,a,td.request.URI,0,0,atoi(td.request.RANGEBYTEBEGIN),atoi(td.request.RANGEBYTEEND));
			else
				sendHTTPRESOURCE(&td,a,td.request.URI);
		}
		else if(!lstrcmpi(td.request.CMD,"POST"))/*!POST REQUEST*/
		{
			if(!lstrcmpi(td.request.RANGETYPE,"bytes"))
				sendHTTPRESOURCE(&td,a,td.request.URI,0,0,atoi(td.request.RANGEBYTEBEGIN),atoi(td.request.RANGEBYTEEND));
			else
				sendHTTPRESOURCE(&td,a,td.request.URI);
		}
		else if(!lstrcmpi(td.request.CMD,"HEAD"))/*!HEAD REQUEST*/
		{
			if(!lstrcmpi(td.request.RANGETYPE,"bytes"))
				sendHTTPRESOURCE(&td,a,td.request.URI,0,1,atoi(td.request.RANGEBYTEBEGIN),atoi(td.request.RANGEBYTEEND));
			else
				sendHTTPRESOURCE(&td,a,td.request.URI,0,1);
		}
		else if(!lstrcmpi(td.request.CMD,"DELETE"))/*!DELETE REQUEST*/
		{
			deleteHTTPRESOURCE(&td,a,td.request.URI,0);
		}
		else if(!lstrcmpi(td.request.CMD,"PUT"))/*!PUT REQUEST*/
		{
			if(!lstrcmpi(td.request.RANGETYPE,"bytes"))
				putHTTPRESOURCE(&td,a,td.request.URI,0,1,atoi(td.request.RANGEBYTEBEGIN),atoi(td.request.RANGEBYTEEND));
			else
				putHTTPRESOURCE(&td,a,td.request.URI,0,1);
		}
		else	/*Return Method not implemented(501)*/
		{
			raiseHTTPError(&td,a,e_501);
			retvalue=0;
		}
		logHTTPaccess(&td,a);
	}

	/*!
	*If the inputData file was not closed close it.
	*/
	if(td.inputData.getHandle())
	{
		td.inputData.closeFile();
		MYSERVER_FILE::deleteFile(td.inputDataPath);
	}
	/*!
	*If the outputData file was not closed close it.
	*/
	if(td.outputData.getHandle())
	{
		td.outputData.closeFile();
		MYSERVER_FILE::deleteFile(td.outputDataPath);
	}	
	return retvalue;
}

/*!
*Compute the Digest to out using a buffer.
*/
void http::computeDigest(httpThreadContext* td,char* out ,char* buffer)
{
	if(!out)
		return;
	MYSERVER_MD5Context md5;
	sprintf(buffer,"%i-%i-%s",clock(),td->id,td->connection->ipAddr);
	MYSERVER_MD5Init(&md5);
	MYSERVER_MD5Update(&md5,(const unsigned char*)buffer ,strlen(buffer));
	MYSERVER_MD5End(&md5,out);
}

/*!
*Sends an error page to the client.
*/
int http::raiseHTTPError(httpThreadContext* td,LPCONNECTION a,int ID)
{
	http_headers::buildDefaultHTTPResponseHeader(&(td->response));
	uint keepalive=0;
	if(!lstrcmpi(td->request.CONNECTION,"Keep-Alive"))
	{
		strcpy(td->response.CONNECTION,"Keep-Alive");
		keepalive=1;
	}
	if(ID==e_401AUTH)
	{
		td->response.httpStatus = 401;
		sprintf(td->buffer2,"HTTP/1.1 401 Unauthorized\r\nAccept-Ranges: bytes\r\nServer: MyServer %s\r\nContent-type: text/html\r\nConnection:%s\r\nContent-length: 0\r\n",versionOfSoftware,td->request.CONNECTION);
		if(td->auth_scheme==HTTP_AUTH_SCHEME_BASIC)
		{
			strcat(td->buffer2,"WWW-Authenticate: Basic\r\n");
		}
		else if(td->auth_scheme==HTTP_AUTH_SCHEME_DIGEST)
		{
			if(a->protocolBuffer==0)
				a->protocolBuffer=malloc(sizeof(http_user_data));
			if(a->protocolBuffer==0)
				return 0;
			strncpy(((http_user_data*)a->protocolBuffer)->realm,td->request.HOST,64);
			
			char md5_str[256];/*Just a random string*/
			strncpy(&(md5_str[2]),td->request.URI,256);
			md5_str[0]=td->id;
			md5_str[1]=clock();
			MYSERVER_MD5Context md5;
			MYSERVER_MD5Init(&md5);
			MYSERVER_MD5Update(&md5,(const unsigned char*)md5_str ,strlen(md5_str));
			MYSERVER_MD5End(&md5,((http_user_data*)a->protocolBuffer)->opaque);
			
			if((!(((http_user_data*)a->protocolBuffer)->digest)) || (((http_user_data*)a->protocolBuffer)->nonce[0]=='\0'))
			{
				computeDigest(td,((http_user_data*)a->protocolBuffer)->nonce,md5_str);
				((http_user_data*)a->protocolBuffer)->nc=0;
			}
			strcat(td->buffer2,"WWW-Authenticate: Digest ");
			sprintf(&(td->buffer2[lstrlen(td->buffer2)])," qop=\"auth\", algorithm =\"MD5\", realm =\"%s\",  opaque =\"%s\",  nonce =\"%s\" ", ((http_user_data*)a->protocolBuffer)->realm, ((http_user_data*)a->protocolBuffer)->opaque, ((http_user_data*)a->protocolBuffer)->nonce);
			if(((http_user_data*)a->protocolBuffer)->cnonce[0])
				sprintf(&(td->buffer2[lstrlen(td->buffer2)]),", cnonce =\"%s\" ", ((http_user_data*)a->protocolBuffer)->cnonce);
			strcat(td->buffer2,"\r\n");
		}		
		else
		{
			/*!
			*Just send a non implemented error page.
			*/
			return raiseHTTPError(td,a,501);
		}				
		strcat(td->buffer2,"Date: ");
		
		getRFC822GMTTime(&td->buffer2[strlen(td->buffer2)],HTTP_RESPONSE_DATE_DIM);
		strcat(td->buffer2,"\r\n\r\n");
		a->socket.send(td->buffer2,(int)strlen(td->buffer2),0);
		return keepalive;
	}
	else
	{
		td->response.httpStatus=getHTTPStatusCodeFromErrorID(ID);
		char defFile[MAX_PATH*2];
		if(getErrorFileName(((vhost*)a->host)->documentRoot,getHTTPStatusCodeFromErrorID(ID),defFile))
		{
			/*!
			*Change the URI to reflect the default file name.
			*/
			char nURL[MAX_PATH+HTTP_REQUEST_URI_DIM+12];
			strcpy(nURL,protocolPrefix);
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
				sprintf(&nURL[strlen(nURL)],":%u",((vhost*)a->host)->port);
			if(nURL[strlen(nURL)-1]!='/')
				strcat(nURL,"/");
			strcat(nURL,defFile);
			return sendHTTPRedirect(td,a,nURL);
		}
	}
	getRFC822GMTTime(td->response.DATEEXP,HTTP_RESPONSE_DATEEXP_DIM);
	strncpy(td->response.ERROR_TYPE,HTTP_ERROR_MSGS[ID],HTTP_RESPONSE_ERROR_TYPE_DIM);
	u_long lenErrorFile=strlen(((vhost*)(a->host))->systemRoot)+strlen(HTTP_ERROR_HTMLS[ID])+2;
	char *errorFile=(char*)malloc(lenErrorFile);
	if(errorFile)
	{
		sprintf(errorFile,"%s/%s",((vhost*)(a->host))->systemRoot,HTTP_ERROR_HTMLS[ID]);
		if(useMessagesFiles && MYSERVER_FILE::fileExists(errorFile))
		{
			return sendHTTPRESOURCE(td,a,HTTP_ERROR_HTMLS[ID],1);
		}
		free(errorFile);
	}
	/*
	*Send the error over the HTTP.
	*/
	sprintf(td->response.CONTENT_LENGTH,"%i",0);

	http_headers::buildHTTPResponseHeader(td->buffer,&td->response);
	a->socket.send(td->buffer,(u_long)strlen(td->buffer), 0);

	return keepalive;
}
/*!
*Send a hard wired 500 error when we have a system error
*/
int http::sendHTTPhardError500(httpThreadContext* td,LPCONNECTION a)
{
	td->response.httpStatus=500;
	sprintf(td->buffer,"%s from: %s\r\n",HTTP_ERROR_MSGS[e_500],a->ipAddr);
	const char hardHTML[] = "<!-- Hard Coded 500 Response --><body bgcolor=\"#000000\"><p align=\"center\">\
		<font size=\"5\" color=\"#00C800\">Error 500</font></p><p align=\"center\"><font size=\"5\" color=\"#00C800\">\
		Internal Server error</font></p>\r\n";

	sprintf(td->buffer2,"HTTP/1.1 500 System Error\r\nServer: MyServer %s\r\nContent-type: text/html\r\nContent-length: %d\r\n",versionOfSoftware,strlen(hardHTML));
	strcat(td->buffer2,"Date: ");
	getRFC822GMTTime(&td->buffer2[strlen(td->buffer2)],HTTP_RESPONSE_DATE_DIM);
	strcat(td->buffer2,"\r\n\r\n");
	a->socket.send(td->buffer2,(u_long)strlen(td->buffer2),0);

	a->socket.send(hardHTML,(u_long)strlen(hardHTML), 0);
	
	return 0;
}
/*!
*Returns the MIME type passing its extension.
*/
int http::getMIME(char *MIME,char *filename,char *dest,char *dest2)
{
	MYSERVER_FILE::getFileExt(dest,filename);
	/*!
	*Returns 1 if file is registered by a CGI.
	*/
	return lserver->mimeManager.getMIME(dest,MIME,dest2);
}
/*!
*Map an URL to the machine file system.
*The output buffer must be capable of receive MAX_PATH characters.
*/
void http::getPath(httpThreadContext* td,LPCONNECTION s,char *filenamePath,const char *filename,int systemrequest)
{
	/*!
	*If it is a system request, search the file in the system folder.
	*/
	if(systemrequest)
	{
		sprintf(filenamePath,"%s/%s",((vhost*)(td->connection->host))->systemRoot,filename);
	}
	/*!
	*Else the file is in the web folder.
	*/
	else
	{	
		if(filename[0])
		{
			strncpy(filenamePath,((vhost*)(td->connection->host))->documentRoot,MAX_PATH);
			u_long len=(u_long)(strlen(filenamePath)+1);
			filenamePath[len-1]='/';
			strncpy(&filenamePath[len],filename,MAX_PATH-len);
			filenamePath[MAX_PATH-1]='\0';
		}
		else
			strncpy(filenamePath,((vhost*)(td->connection->host))->documentRoot,MAX_PATH);

	}
}


/*!
*Send a redirect message to the client.
*/
int http::sendHTTPRedirect(httpThreadContext* td,LPCONNECTION a,char *newURL)
{
	td->response.httpStatus=302;
	sprintf(td->buffer2,"HTTP/1.1 302 Moved\r\nAccept-Ranges: bytes\r\nServer: MyServer %s\r\nContent-type: text/html\r\nLocation: %s\r\nContent-length: 0\r\n",versionOfSoftware,newURL);
	int keepalive=0;
	if(!lstrcmpi(td->request.CONNECTION,"Keep-Alive"))
	{
		strcat(td->buffer2,"Connection: Keep-Alive\r\n");	
		keepalive = 1;
	}
	strcat(td->buffer2,"Date: ");
	getRFC822GMTTime(&td->buffer2[strlen(td->buffer2)],HTTP_RESPONSE_DATE_DIM);
	strcat(td->buffer2,"\r\n\r\n");

	a->socket.send(td->buffer2,(int)strlen(td->buffer2),0);

	return keepalive;
}
/*!
*Send a non-modified message to the client.
*/
int http::sendHTTPNonModified(httpThreadContext* td,LPCONNECTION a)
{
	td->response.httpStatus=304;
	sprintf(td->buffer2,"HTTP/1.1 304 Not Modified\r\nAccept-Ranges: bytes\r\nServer: MyServer %s\r\n",versionOfSoftware);
	int keepalive=0;
	if(!lstrcmpi(td->request.CONNECTION,"Keep-Alive"))
	{
		strcat(td->buffer2,"Connection: Keep-Alive\r\n");	
		keepalive = 1;
	}	
	strcat(td->buffer2,"Date: ");
	getRFC822GMTTime(&td->buffer2[strlen(td->buffer2)],HTTP_RESPONSE_DATE_DIM);
	strcat(td->buffer2,"\r\n\r\n");

	a->socket.send(td->buffer2,(int)strlen(td->buffer2),0);
	return keepalive;
}

/*!
*Send a 401 error
*/
int http::sendAuth(httpThreadContext* td,LPCONNECTION s)
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
/*!
*Load the HTTP protocol.
*/
int http::loadProtocol(cXMLParser* languageParser,char* confFile)
{
	/*!
	*Initialize ISAPI.
	*/
	isapi::initISAPI();
	/*!
	*Initialize FastCGI
	*/
	fastcgi::initializeFASTCGI();	

	/*!
	*Load the MSCGI library.
	*/
	mscgiLoaded=mscgi::loadMSCGILib();
	if(mscgiLoaded)
		printf("%s\n",languageParser->getValue("MSG_LOADMSCGI"));
	else
	{
		preparePrintError();
		printf("%s\n",languageParser->getValue("ERR_LOADMSCGI"));
		endPrintError();
	}
	/*!
	*Store defaults value.
	*/
	gzip_threshold=1<<20;
	useMessagesFiles=1;
	browseDirCSSpath[0]='\0';
	cXMLParser configurationFileManager;
	configurationFileManager.open("myserver.xml");
	char *data;	
	/*!
	*Determine the min file size that will use GZIP compression.
	*/
	data=configurationFileManager.getValue("GZIP_THRESHOLD");
	if(data)
	{
		gzip_threshold=atoi(data);
	}		
	data=configurationFileManager.getValue("USE_ERRORS_FILES");
	if(data)
	{
		if(!lstrcmpi(data,"YES"))
			useMessagesFiles=1;
		else
			useMessagesFiles=0;
	}	
	data=configurationFileManager.getValue("BROWSEFOLDER_CSS");
	if(data)
	{
		lstrcpy(browseDirCSSpath,data);
	}
	/*!
	*Determine the number of default filenames written in the configuration file.
	*/
	nDefaultFilename=0;
	for(;;)
	{
		char xmlMember[21];
		sprintf(xmlMember,"DEFAULT_FILENAME%i",nDefaultFilename);
		if(!strlen(configurationFileManager.getValue(xmlMember)))
			break;
		nDefaultFilename++;

	}
	/*!
	*Copy the right values in the buffer.
	*/
	if(nDefaultFilename==0)
	{
		defaultFilename =(char*)malloc(MAX_PATH);
		if(defaultFilename)
			strcpy(defaultFilename,"default.html");
	}
	else
	{
		u_long i;
		if(defaultFilename)
			free(defaultFilename);
		defaultFilename =(char*)malloc(MAX_PATH*nDefaultFilename);
		for(i=0;defaultFilename && (i<nDefaultFilename);i++)
		{
			char xmlMember[21];
			sprintf(xmlMember,"DEFAULT_FILENAME%i",i);
			data=configurationFileManager.getValue(xmlMember);
			if(data)
				strcpy(&defaultFilename[i*MAX_PATH],data);
		}
	}	
	configurationFileManager.close();
	return 1;	
}
/*!
*Unload the HTTP protocol.
*/
int http::unloadProtocol(cXMLParser* languageParser)
{
	/*!
	*Clean ISAPI.
	*/
	isapi::cleanupISAPI();
	/*!
	*Clean FastCGI.
	*/
	fastcgi::cleanFASTCGI();
	/*!
	*Clean MSCGI.
	*/
	mscgi::freeMSCGILib();
	if(defaultFilename)
	{
		free(defaultFilename);
		defaultFilename=0;
	}
	return 1;
}

/*!
*Returns the default filename.
*/
char *http::getDefaultFilenamePath(u_long ID)
{
	if(ID<nDefaultFilename)
		return defaultFilename+ID*MAX_PATH;
	else
		return 0;
}

/*!
*Returns the name of the protocol. If an out buffer is defined fullfill it with the name too.
*/
char* http::registerName(char* out,int len)
{
	if(out)
	{
		strncpy(out,"HTTP",len);
	}
	return "HTTP";
}
/*!
*Constructor for the class http
*/
http::http()
{
	strcpy(protocolPrefix,"http://");
	PROTOCOL_OPTIONS=0;
}
