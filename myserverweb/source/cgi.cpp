/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
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

#include "../stdafx.h"
#include "../include/cgi.h"
#include "../include/http_headers.h"
#include "../include/http.h"
#include "../include/HTTPmsg.h"
#include "../include/cserver.h"
#include "../include/security.h"
#include "../include/AMMimeUtils.h"
#include "../include/filemanager.h"
#include "../include/sockets.h"
#include "../include/utility.h"

extern "C" {
#ifdef WIN32
#include <direct.h>
#endif
#include <string.h>
}

#ifndef lstrcpy
#define lstrcpy strcpy
#endif
#ifndef lstrlen
#define lstrlen strlen
#endif
#ifndef lstrcpyn
#define lstrcpyn strncpy
#endif
#ifndef strcat
#define strcat strcat
#endif
#ifndef strnicmp
#define strnicmp strncmp
#endif

/*!
*Run the standard CGI and send the result to the client.
*/
int cgi::sendCGI(httpThreadContext* td,LPCONNECTION s,char* scriptpath,char* /*!ext*/,char *cgipath,int cmd)
{
	/*! Use this variable to determine if the CGI executable is nph(Non Parsed Header).  */
	int nph;
	char cmdLine[MAX_PATH*3+1];
	char filename[MAX_PATH];
	lstrcpy(td->scriptPath,scriptpath);
	MYSERVER_FILE::splitPath(scriptpath,td->scriptDir,td->scriptFile);

	MYSERVER_FILE::splitPath(cgipath,td->cgiRoot,td->cgiFile);

	/*!
	*If the cmd is equal to CGI_CMD_EXECUTE then we must execute the
	*scriptpath file as an executable.
	*Then to determine if is a nph CGI we must use the scriptpath
	*string.
	*/
	if(cmd==CGI_CMD_EXECUTE)
	{
		MYSERVER_FILE::getFilename(scriptpath,filename);
#ifdef WIN32
		/*!
		*Under the windows platform to run a file like an executable
		*use the sintact "cmd /c filename".
		*/
		sprintf(cmdLine,"cmd /c %s %s",td->scriptFile,td->pathInfo[0]?&td->pathInfo[1]:td->pathInfo);
		nph=(strnicmp("nph-",td->scriptFile, 4)==0)?1:0;
#endif
	}
	else if(cmd==CGI_CMD_RUNCGI)
	{
		if(!MYSERVER_FILE::fileExists(cgipath))
		{
			((vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
			((vhost*)td->connection->host)->warningsLogWrite("Cannot find ");
			((vhost*)td->connection->host)->warningsLogWrite(cgipath);
			((vhost*)td->connection->host)->warningsLogWrite(" executable\r\n");
			((vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);		
			return ((http*)td->lhttp)->raiseHTTPError(td,s,e_500);
		}
		sprintf(cmdLine,"%s %s",cgipath,td->scriptFile);
		nph=(strnicmp("nph-",td->cgiFile, 4)==0)?1:0;
	}
	else
	{
		/*!
		*If the command was not recognized send an 501 page error.
		*/
		return ((http*)td->lhttp)->raiseHTTPError(td,s,e_501);
	}

	/*!
	*Use a temporary file to store CGI output.
	*Every thread has it own tmp file name(td->outputDataPath),
	*so use this name for the file that is going to be
	*created because more threads can access more CGI at the same time.
	*/
	char currentpath[MAX_PATH];
	char outputDataPath[MAX_PATH];
	getdefaultwd(currentpath,MAX_PATH);
	sprintf(outputDataPath,"%s/stdOutFileCGI_%u",currentpath,(unsigned int)td->id);
	
	/*!
	*Standard CGI uses STDOUT to output the result and the STDIN 
	*to get other params like in a POST request.
	*/
	MYSERVER_FILE stdOutFile;
	if(!stdOutFile.createTemporaryFile(outputDataPath))
	{
		((vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((vhost*)td->connection->host)->warningsLogWrite("Cannot create CGI stdout file\r\n" );
		((vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		return ((http*)td->lhttp)->raiseHTTPError(td,s,e_500);
	}
	MYSERVER_FILE stdInFile;
	td->inputData.closeFile();
	if(!stdInFile.openFile(td->inputDataPath,MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_ALWAYS))
	{
		((vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((vhost*)td->connection->host)->warningsLogWrite("Cannot open CGI stdin file\r\n" );
		((vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		stdOutFile.closeFile();
		return ((http*)td->lhttp)->raiseHTTPError(td,s,e_500);
	
	}

	/*!
	*Build the environment string used by the CGI started
	*by the execHiddenProcess(...) function.
	*Use the td->buffer2 to build the environment string.
	*/
	((char*)td->buffer2->GetBuffer())[0]='\0';
	buildCGIEnvironmentString(td,(char*)td->buffer2->GetBuffer());

	/*!
	*With this code we execute the CGI process.
	*Fill the START_PROC_INFO struct with the correct values and use it
	*to run the process
	*/
	START_PROC_INFO spi;
	spi.cmdLine = cmdLine;
	spi.cwd=td->scriptDir;
	// added for unix support
	spi.cmd = cgipath;
	spi.arg = td->scriptFile;
	
	spi.stdError = stdOutFile.getHandle();
	spi.stdIn = stdInFile.getHandle();
	spi.stdOut = stdOutFile.getHandle();
	spi.envString=(char*)td->buffer2->GetBuffer();
	execHiddenProcess(&spi);
	td->buffer2->SetLength(0);

	/*! Read the CGI output.  */
	u_long nBytesRead=0;
	if(!stdOutFile.setFilePointer(0))
		stdOutFile.readFromFile((char*)td->buffer2->GetBuffer(),td->buffer2->GetRealLength(),&nBytesRead);
		
	((char*)td->buffer2->GetBuffer())[nBytesRead]='\0';
		
	int yetoutputted=0;
	if(nBytesRead==0)
	{
		((vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((vhost*)td->connection->host)->warningsLogWrite("Error CGI zero bytes read\r\n" );
		((vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		((http*)td->lhttp)->raiseHTTPError(td,s,e_500);
		yetoutputted=1;
	}
	/*! Standard CGI can include an extra HTTP header.  */
	u_long headerSize=0;

	for(u_long i=0;i<nBytesRead;i++)
	{
		char *buff=(char*)td->buffer2->GetBuffer();
		if((buff[i]=='\r')&&(buff[i+1]=='\n')&&(buff[i+2]=='\r')&&(buff[i+3]=='\n'))
		{
			/*!
			*The HTTP header ends with a \r\n\r\n sequence so 
			*determinate where it ends and set the header size
			*to i + 4.
			*/
			headerSize=i+4;
			break;
		}
		/*! If it is present Location: xxx in the header send a redirect to xxx.  */
		else if(!strncmp(&((char*)td->buffer2->GetBuffer())[i],"Location",8))
		{
			char nURL[MAX_PATH];
			nURL[0]='\0';
			u_long j;
			j=0;

			int start=(int)strlen(nURL);
			while(((char*)td->buffer2->GetBuffer())[i+j+10]!='\r')
			{

				nURL[j+start]=((char*)td->buffer2->GetBuffer())[i+j+10];
				nURL[j+start+1]='\0';
				j++;
			}
			if(!yetoutputted)
			{
				((http*)td->lhttp)->sendHTTPRedirect(td,s,nURL);
			}
			yetoutputted=1;
		}
	}
	if(!yetoutputted)
	{
		if(!lstrcmpi(td->request.CONNECTION,"Keep-Alive"))
			strcpy(td->response.CONNECTION,"Keep-Alive");
		/*! Don't send any other HTTP header if the CGI executable has the nph-.... form name.  */
		if(nph)
		{
			/*! Resetting the structure we send only the information gived by the CGI. */
			http_headers::resetHTTPResponse(&(td->response));
		}
		u_long nbw=0;

		/*! Send the header.  */
		if(!td->appendOutputs)
		{
			if(headerSize)
				http_headers::buildHTTPResponseHeaderStruct(&td->response,td,(char*)td->buffer2->GetBuffer());
   			/*! Always specify the size of the HTTP contents.  */
			sprintf(td->response.CONTENT_LENGTH,"%u",(unsigned int)stdOutFile.getFileSize()-headerSize);
			http_headers::buildHTTPResponseHeader((char*)td->buffer->GetBuffer(),&td->response);
			td->buffer->SetLength((u_int)strlen((char*)td->buffer->GetBuffer()));
			s->socket.send((char*)td->buffer->GetBuffer(),(int)(td->buffer->GetLength()), 0);
			s->socket.send((char*)(((char*)td->buffer2->GetBuffer())+headerSize),nBytesRead-headerSize, 0);
		}
		else
			td->outputData.writeToFile((char*)(((char*)td->buffer2->GetBuffer())+headerSize),nBytesRead-headerSize,&nbw);
				
		while(stdOutFile.readFromFile((char*)td->buffer2->GetBuffer(),td->buffer2->GetLength(),&nBytesRead))
		{
			if(nBytesRead)
			{
				
				if(!td->appendOutputs)/*Send the header*/
					s->socket.send((char*)td->buffer2->GetBuffer(),nBytesRead, 0);
				else
					td->outputData.writeToFile((char*)td->buffer2->GetBuffer(),nBytesRead,&nbw);
			}
			else
				break;
		}
		
	}
	
	/*! Close and delete the stdin and stdout files used by the CGI.  */
	stdOutFile.closeFile();
	stdInFile.closeFile();
	MYSERVER_FILE::deleteFile(td->inputDataPath);

	/*! Don't close the socket for Keep-Alive connections.  */
	return (!lstrcmpi(td->response.CONNECTION,"Keep-Alive"));
}
/*!
*Write the string that contain the CGI environment to cgiEnvString.
*This function is used by other server side protocols too.
*/
void cgi::buildCGIEnvironmentString(httpThreadContext* td,char *cgi_env_string,int processEnv)
{
	/*!
	*The Environment string is a null-terminated block of null-terminated strings.
	*For no problems with the function strcat we use the character \r for the \0 character
	*and at the end we change every \r in \0.
	*/
	strncat(cgi_env_string,"SERVER_SOFTWARE=MyServer ",strlen(cgi_env_string)+26);
	strncat(cgi_env_string,versionOfSoftware,strlen(cgi_env_string)+strlen(versionOfSoftware)+1);

#ifdef WIN32
	strncat(cgi_env_string," (WIN32)",strlen(cgi_env_string)+9);
#else
#ifdef HOST_STR
	strncat(cgi_env_string, HOST_STR,strlen(cgi_env_string)+strlen(HOST_STR)+1);
#else
	strncat(cgi_env_string, " (Unknown)",strlen(cgi_env_string)+11);
#endif
#endif
	/*! *Must use REDIRECT_STATUS for php and others.  */
	strncat(cgi_env_string,"\rREDIRECT_STATUS=TRUE",strlen(cgi_env_string)+22);
	
	strncat(cgi_env_string,"\rSERVER_NAME=",strlen(cgi_env_string)+14);
 	strncat(cgi_env_string,lserver->getServerName(),strlen(cgi_env_string)+strlen(lserver->getServerName())+1);

	strncat(cgi_env_string,"\rSERVER_SIGNATURE=",strlen(cgi_env_string)+19);
	strncat(cgi_env_string,"<address>MyServer ",strlen(cgi_env_string)+19);
	strncat(cgi_env_string,versionOfSoftware,strlen(cgi_env_string)+strlen(versionOfSoftware)+1);
	strncat(cgi_env_string,"</address>",strlen(cgi_env_string)+11);

	strncat(cgi_env_string,"\rSERVER_PROTOCOL=",strlen(cgi_env_string)+18);
	strncat(cgi_env_string,td->request.VER,strlen(cgi_env_string)+strlen(td->request.VER)+1);	
	
	strncat(cgi_env_string,"\rSERVER_PORT=",strlen(cgi_env_string)+14);
	sprintf(&cgi_env_string[strlen(cgi_env_string)],"%u",td->connection->localPort);

	strncat(cgi_env_string,"\rSERVER_ADMIN=",strlen(cgi_env_string)+15);
	strncat(cgi_env_string,lserver->getServerAdmin(),strlen(cgi_env_string)+strlen(lserver->getServerAdmin())+1);

	strncat(cgi_env_string,"\rREQUEST_METHOD=",strlen(cgi_env_string)+17);
	strncat(cgi_env_string,td->request.CMD,strlen(cgi_env_string)+strlen(td->request.CMD)+1);

	strncat(cgi_env_string,"\rREQUEST_URI=/",strlen(cgi_env_string)+15);
	
 	lstrcpyn(&cgi_env_string[strlen(cgi_env_string)],td->request.URI,(int)(strlen(td->request.URI)-strlen(td->pathInfo)+1));

	strncat(cgi_env_string,"\rQUERY_STRING=",strlen(cgi_env_string)+15);
	strncat(cgi_env_string,td->request.URIOPTS,strlen(cgi_env_string)+strlen(td->request.URIOPTS)+1);

	strncat(cgi_env_string,"\rGATEWAY_INTERFACE=CGI/1.1",strlen(cgi_env_string)+27);

	if(td->request.CONTENT_TYPE[0])
	{
		strncat(cgi_env_string,"\rCONTENT_TYPE=",strlen(cgi_env_string)+15);
		strncat(cgi_env_string,td->request.CONTENT_TYPE,strlen(cgi_env_string)+strlen(td->request.CONTENT_TYPE)+1);
	}

	if(td->request.CONTENT_LENGTH[0])
	{
		strncat(cgi_env_string,"\rCONTENT_LENGTH=",strlen(cgi_env_string)+17);
		strncat(cgi_env_string,td->request.CONTENT_LENGTH,strlen(cgi_env_string)+strlen(td->request.CONTENT_LENGTH)+1);
	}
	else
	{
		u_long fs=0;
		if(td->inputData.getHandle())
			fs=td->inputData.getFileSize();
		sprintf(&cgi_env_string[strlen(cgi_env_string)],"\rCONTENT_LENGTH=%u",fs);
	}

	if(td->request.COOKIE[0])
	{
		strncat(cgi_env_string,"\rHTTP_COOKIE=",strlen(cgi_env_string)+14);
		strncat(cgi_env_string,td->request.COOKIE,strlen(cgi_env_string)+strlen(td->request.COOKIE)+1);
	}

	if(td->request.REFERER[0])
	{
		strncat(cgi_env_string,"\rHTTP_REFERER=",strlen(cgi_env_string)+15);
		strncat(cgi_env_string,td->request.REFERER,strlen(cgi_env_string)+strlen(td->request.REFERER)+1);
	}
	if(td->request.CACHE_CONTROL[0])
	{
		strncat(cgi_env_string,"\rHTTP_CACHE_CONTROL=",strlen(cgi_env_string)+21);
		strncat(cgi_env_string,td->request.CACHE_CONTROL,strlen(cgi_env_string)+strlen(td->request.CACHE_CONTROL)+1);
	}
	if(td->request.ACCEPT[0])
	{
		strncat(cgi_env_string,"\rHTTP_ACCEPT=",strlen(cgi_env_string)+14);
		strncat(cgi_env_string,td->request.ACCEPT,strlen(cgi_env_string)+strlen(td->request.ACCEPT)+1);
	}

	if(td->cgiRoot[0])
	{
		strncat(cgi_env_string,"\rCGI_ROOT=",strlen(cgi_env_string)+11);
		strncat(cgi_env_string,td->cgiRoot,strlen(cgi_env_string)+strlen(td->cgiRoot)+1);
	}
	if(td->request.HOST[0])
	{
		strncat(cgi_env_string,"\rHTTP_HOST=",strlen(cgi_env_string)+12);
		strncat(cgi_env_string,td->request.HOST,strlen(cgi_env_string)+strlen(td->request.HOST)+1);
	}
	if(td->connection->ipAddr[0])
	{
		strncat(cgi_env_string,"\rREMOTE_ADDR=",strlen(cgi_env_string)+14);
		strncat(cgi_env_string,td->connection->ipAddr,strlen(cgi_env_string)+strlen(td->connection->ipAddr)+1);
	}
	if(td->connection->port)
	{
	 	strncat(cgi_env_string,"\rREMOTE_PORT=",strlen(cgi_env_string)+14);
		sprintf(&cgi_env_string[strlen(cgi_env_string)],"%u",td->connection->port);
	}

	if(td->connection->login[0])
	{
	  	strncat(cgi_env_string,"\rREMOTE_USER=",strlen(cgi_env_string)+14);
		strncat(cgi_env_string,td->connection->login,strlen(cgi_env_string)+strlen(td->connection->login)+1);
	}
	
	if(((vhost*)(td->connection->host))->protocol==PROTOCOL_HTTPS)
		strncat(cgi_env_string,"\rSSL=ON",strlen(cgi_env_string)+8);
	else
		strncat(cgi_env_string,"\rSSL=OFF",strlen(cgi_env_string+9));

	if(td->request.CONNECTION[0])
	{
		strncat(cgi_env_string,"\rHTTP_CONNECTION=",strlen(cgi_env_string)+18);
		strncat(cgi_env_string,td->request.CONNECTION,strlen(cgi_env_string)+strlen(td->request.CONNECTION)+1);
	}
	if(td->request.AUTH[0])
	{
		strncat(cgi_env_string,"\rAUTH_TYPE=",strlen(cgi_env_string)+12);
		strncat(cgi_env_string,td->request.AUTH,strlen(cgi_env_string)+strlen(td->request.AUTH)+1);
	}
	if(td->request.USER_AGENT[0])
	{
		strncat(cgi_env_string,"\rHTTP_USER_AGENT=",strlen(cgi_env_string)+18);
		strncat(cgi_env_string,td->request.USER_AGENT,strlen(cgi_env_string)+strlen(td->request.USER_AGENT)+1);
	}
	if(td->request.ACCEPTENC[0])
	{
		strncat(cgi_env_string,"\rHTTP_ACCEPT_ENCODING=",strlen(cgi_env_string)+23);
		strncat(cgi_env_string,td->request.ACCEPTENC,strlen(cgi_env_string)+strlen(td->request.ACCEPTENC)+1);
	}
	if(td->request.ACCEPTLAN[0])
	{
		strncat(cgi_env_string,"\rHTTP_ACCEPT_LANGUAGE=",strlen(cgi_env_string)+23);
	  	strncat(cgi_env_string,td->request.ACCEPTLAN,strlen(cgi_env_string)+strlen(td->request.ACCEPTLAN)+1);
	}
	if(td->pathInfo[0])
	{
		strncat(cgi_env_string,"\rPATH_INFO=",strlen(cgi_env_string)+12);
	  	strncat(cgi_env_string,td->pathInfo,strlen(cgi_env_string)+strlen(td->pathInfo)+1);
	  	
	  	strncat(cgi_env_string,"\rPATH_TRANSLATED=",strlen(cgi_env_string)+18);
		strncat(cgi_env_string,td->pathTranslated,strlen(cgi_env_string)+strlen(td->pathTranslated)+1);
	}
	else
	{
  		strncat(cgi_env_string,"\rPATH_TRANSLATED=",strlen(cgi_env_string)+18);
		strncat(cgi_env_string,td->filenamePath,strlen(cgi_env_string)+strlen(td->filenamePath)+1);
	}

 	strncat(cgi_env_string,"\rSCRIPT_FILENAME=",strlen(cgi_env_string)+18);
	strncat(cgi_env_string,td->filenamePath,strlen(cgi_env_string)+strlen(td->filenamePath)+1);
	
	/*!
	*For the DOCUMENT_URI and SCRIPT_NAME copy the requested URI without the pathInfo.
	*/
	strncat(cgi_env_string,"\rSCRIPT_NAME=/",strlen(cgi_env_string)+15);
	lstrcpyn(&cgi_env_string[strlen(cgi_env_string)],td->request.URI,(int)(strlen(td->request.URI)-strlen(td->pathInfo)+1));

	strncat(cgi_env_string,"\rSCRIPT_URL=/",strlen(cgi_env_string)+14);
	lstrcpy(&cgi_env_string[strlen(cgi_env_string)],td->request.URI);

	strncat(cgi_env_string,"\rDATE_GMT=",strlen(cgi_env_string)+11);
	getRFC822GMTTime(&cgi_env_string[strlen(cgi_env_string)],HTTP_RESPONSE_DATE_DIM);

 	strncat(cgi_env_string,"\rDATE_LOCAL=",strlen(cgi_env_string)+13);
	getRFC822LocalTime(&cgi_env_string[strlen(cgi_env_string)],HTTP_RESPONSE_DATE_DIM);

	strncat(cgi_env_string,"\rDOCUMENT_ROOT=",strlen(cgi_env_string)+16);
	strncat(cgi_env_string,((vhost*)(td->connection->host))->documentRoot,strlen(cgi_env_string+1));

	strncat(cgi_env_string,"\rDOCUMENT_URI=/",strlen(cgi_env_string)+15);
	lstrcpyn(&cgi_env_string[strlen(cgi_env_string)],td->request.URI,(int)(strlen(td->request.URI)-strlen(td->pathInfo)+1));
	
	strncat(cgi_env_string,"\rDOCUMENT_NAME=",strlen(cgi_env_string)+16);
	strncat(cgi_env_string,td->filenamePath,strlen(cgi_env_string)+strlen(td->filenamePath)+1);

	if(td->identity[0])
	{
  		strncat(cgi_env_string,"\rREMOTE_IDENT=",strlen(cgi_env_string)+15);
    	strncat(cgi_env_string,td->identity,strlen(cgi_env_string)+strlen(td->identity)+1);
	}
#ifdef WIN32
	if(processEnv)
	{
		strncat(cgi_env_string,"\r",strlen(cgi_env_string)+2);
  		LPTSTR lpszVariable; 
		LPVOID lpvEnv; 
		lpvEnv = lserver->envString; 
		for (lpszVariable = (LPTSTR) lpvEnv; *lpszVariable; lpszVariable++) 
		{ 
			if(((char*)lpszVariable)[0] !='=')
			{
				strncat(cgi_env_string,(char*)lpszVariable,strlen(cgi_env_string)+strlen(lpszVariable)+1);
				strncat(cgi_env_string,"\r",strlen(cgi_env_string)+2);
			}
			while (*lpszVariable)*lpszVariable++;
		} 
	}
#endif

	strncat(cgi_env_string,"\r\0\0\0\0\0",strlen(cgi_env_string)+7);
 	size_t max=strlen(cgi_env_string);
	for(size_t i=0;i<max;i++)
		if(cgi_env_string[i]=='\r')
			cgi_env_string[i]='\0';
}
