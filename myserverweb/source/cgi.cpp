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
	/*!
	*Use this variable to determine if the CGI executable is nph(Non Parsed Header).
	*/
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
	stdOutFile.createTemporaryFile(outputDataPath);
	MYSERVER_FILE stdInFile;
	td->inputData.closeFile();	
	stdInFile.openFile(td->inputDataPath,MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_ALWAYS);

	/*!
	*Build the environment string used by the CGI started
	*by the execHiddenProcess(...) function.
	*Use the td->buffer2 to build the environment string.
	*/
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
	/*!
	*Read the CGI output.
	*/
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
	/*!
	*Standard CGI can include an extra HTTP header.
	*/
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
		/*!
		*If it is present Location: xxx in the header send a redirect to xxx
		*/
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
		/*!
		*Don't send any other HTTP header if the CGI executable has the nph-.... form name.
		*/
		if(nph)
		{
			/*!
			*Resetting the structure we send only the information gived by the CGI.
			*/
			http_headers::resetHTTPResponse(&(td->response));
		}
		u_long nbw=0;

		if(!td->appendOutputs)/*Send the header*/
		{
			if(headerSize)
				http_headers::buildHTTPResponseHeaderStruct(&td->response,td,(char*)td->buffer2->GetBuffer());
			/*!
			*Always specify the size of the HTTP contents.
			*/
			sprintf(td->response.CONTENT_LENGTH,"%u",(unsigned int)stdOutFile.getFileSize()-headerSize);
			http_headers::buildHTTPResponseHeader((char*)td->buffer->GetBuffer(),&td->response);
			
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
	
	/*!
	*Close and delete the stdin and stdout files used by the CGI.
	*/
	stdOutFile.closeFile();
	stdInFile.closeFile();
	MYSERVER_FILE::deleteFile(td->inputDataPath);

	/*!
	*Don't close the socket for Keep-Alive connections.
	*/
	return (!lstrcmpi(td->response.CONNECTION,"Keep-Alive"));
}
/*!
*Write the string that contain the CGI environment to cgiEnvString.
*This function is used by other server side protocols too.
*/
void cgi::buildCGIEnvironmentString(httpThreadContext* td,char *cgiEnvString,int processEnv)
{
	/*!
	*The Environment string is a null-terminated block of null-terminated strings.
	*For no problems with the function strcat we use the character \r for the \0 character
	*and at the end we change every \r in \0.
	*/
	strncat(cgiEnvString,"SERVER_SOFTWARE=MyServer ",strlen(cgiEnvString)+26);
	strncat(cgiEnvString,versionOfSoftware,strlen(cgiEnvString)+strlen(versionOfSoftware)+1);

#ifdef WIN32
	strncat(cgiEnvString," (WIN32)",strlen(cgiEnvString)+9);
#else
#ifdef HOST_STR
	strncat(cgiEnvString, HOST_STR,strlen(cgiEnvString)+strlen(HOST_STR)+1);
#else
	strncat(cgiEnvString, " (Unknown)",strlen(cgiEnvString)+11);
#endif
#endif
	/*!
	*Must use REDIRECT_STATUS for php and others
	*/
	strncat(cgiEnvString,"\rREDIRECT_STATUS=TRUE",strlen(cgiEnvString)+22);
	
	strncat(cgiEnvString,"\rSERVER_NAME=",strlen(cgiEnvString)+14);
 	strncat(cgiEnvString,lserver->getServerName(),strlen(cgiEnvString)+strlen(lserver->getServerName())+1);

	strncat(cgiEnvString,"\rSERVER_SIGNATURE=",strlen(cgiEnvString)+19);
	strncat(cgiEnvString,"<address>MyServer ",strlen(cgiEnvString)+19);
	strncat(cgiEnvString,versionOfSoftware,strlen(cgiEnvString)+strlen(versionOfSoftware)+1);
	strncat(cgiEnvString,"</address>",strlen(cgiEnvString)+11);

	strncat(cgiEnvString,"\rSERVER_PROTOCOL=",strlen(cgiEnvString)+18);
	strncat(cgiEnvString,td->request.VER,strlen(cgiEnvString)+strlen(td->request.VER)+1);	
	
	strncat(cgiEnvString,"\rSERVER_PORT=",strlen(cgiEnvString)+14);
	sprintf(&cgiEnvString[strlen(cgiEnvString)],"%u",td->connection->localPort);

	strncat(cgiEnvString,"\rSERVER_ADMIN=",strlen(cgiEnvString)+15);
	strncat(cgiEnvString,lserver->getServerAdmin(),strlen(cgiEnvString)+strlen(lserver->getServerAdmin())+1);

	strncat(cgiEnvString,"\rREQUEST_METHOD=",strlen(cgiEnvString)+17);
	strncat(cgiEnvString,td->request.CMD,strlen(cgiEnvString)+strlen(td->request.CMD)+1);

	strncat(cgiEnvString,"\rREQUEST_URI=/",strlen(cgiEnvString)+15);
	
 	lstrcpyn(&cgiEnvString[strlen(cgiEnvString)],td->request.URI,(int)(strlen(td->request.URI)-strlen(td->pathInfo)+1));

	strncat(cgiEnvString,"\rQUERY_STRING=",strlen(cgiEnvString)+15);
	strncat(cgiEnvString,td->request.URIOPTS,strlen(cgiEnvString)+strlen(td->request.URIOPTS)+1);

	strncat(cgiEnvString,"\rGATEWAY_INTERFACE=CGI/1.1",strlen(cgiEnvString)+27);

	if(td->request.CONTENT_TYPE[0])
	{
		strncat(cgiEnvString,"\rCONTENT_TYPE=",strlen(cgiEnvString)+15);
		strncat(cgiEnvString,td->request.CONTENT_TYPE,strlen(cgiEnvString)+strlen(td->request.CONTENT_TYPE)+1);
	}

	if(td->request.CONTENT_LENGTH[0])
	{
		strncat(cgiEnvString,"\rCONTENT_LENGTH=",strlen(cgiEnvString)+17);
		strncat(cgiEnvString,td->request.CONTENT_LENGTH,strlen(cgiEnvString)+strlen(td->request.CONTENT_LENGTH)+1);
	}
	else
	{
		u_long fs=0;
		if(td->inputData.getHandle())
			fs=td->inputData.getFileSize();
		sprintf(&cgiEnvString[strlen(cgiEnvString)],"\rCONTENT_LENGTH=%u",fs);
	}

	if(td->request.COOKIE[0])
	{
		strncat(cgiEnvString,"\rHTTP_COOKIE=",strlen(cgiEnvString)+14);
		strncat(cgiEnvString,td->request.COOKIE,strlen(cgiEnvString)+strlen(td->request.COOKIE)+1);
	}

	if(td->request.REFERER[0])
	{
		strncat(cgiEnvString,"\rHTTP_REFERER=",strlen(cgiEnvString)+15);
		strncat(cgiEnvString,td->request.REFERER,strlen(cgiEnvString)+strlen(td->request.REFERER)+1);
	}
	if(td->request.CACHE_CONTROL[0])
	{
		strncat(cgiEnvString,"\rHTTP_CACHE_CONTROL=",strlen(cgiEnvString)+21);
		strncat(cgiEnvString,td->request.CACHE_CONTROL,strlen(cgiEnvString)+strlen(td->request.CACHE_CONTROL)+1);
	}
	if(td->request.ACCEPT[0])
	{
		strncat(cgiEnvString,"\rHTTP_ACCEPT=",strlen(cgiEnvString)+14);
		strncat(cgiEnvString,td->request.ACCEPT,strlen(cgiEnvString)+strlen(td->request.ACCEPT)+1);
	}

	if(td->cgiRoot[0])
	{
		strncat(cgiEnvString,"\rCGI_ROOT=",strlen(cgiEnvString)+11);
		strncat(cgiEnvString,td->cgiRoot,strlen(cgiEnvString)+strlen(td->cgiRoot)+1);
	}
	if(td->request.HOST[0])
	{
		strncat(cgiEnvString,"\rHTTP_HOST=",strlen(cgiEnvString)+12);
		strncat(cgiEnvString,td->request.HOST,strlen(cgiEnvString)+strlen(td->request.HOST)+1);
	}
	if(td->connection->ipAddr[0])
	{
		strncat(cgiEnvString,"\rREMOTE_ADDR=",strlen(cgiEnvString)+14);
		strncat(cgiEnvString,td->connection->ipAddr,strlen(cgiEnvString)+strlen(td->connection->ipAddr)+1);
	}
	if(td->connection->port)
	{
	 	strncat(cgiEnvString,"\rREMOTE_PORT=",strlen(cgiEnvString)+14);
		sprintf(&cgiEnvString[strlen(cgiEnvString)],"%u",td->connection->port);
	}

	if(td->connection->login[0])
	{
	  	strncat(cgiEnvString,"\rREMOTE_USER=",strlen(cgiEnvString)+14);
		strncat(cgiEnvString,td->connection->login,strlen(cgiEnvString)+strlen(td->connection->login)+1);
	}
	
	if(((vhost*)(td->connection->host))->protocol==PROTOCOL_HTTPS)
		strncat(cgiEnvString,"\rSSL=ON",strlen(cgiEnvString)+8);
	else
		strncat(cgiEnvString,"\rSSL=OFF",strlen(cgiEnvString+9));

	if(td->request.CONNECTION[0])
	{
		strncat(cgiEnvString,"\rHTTP_CONNECTION=",strlen(cgiEnvString)+18);
		strncat(cgiEnvString,td->request.CONNECTION,strlen(cgiEnvString)+strlen(td->request.CONNECTION)+1);
	}
	if(td->request.AUTH[0])
	{
		strncat(cgiEnvString,"\rAUTH_TYPE=",strlen(cgiEnvString)+12);
		strncat(cgiEnvString,td->request.AUTH,strlen(cgiEnvString)+strlen(td->request.AUTH)+1);
	}
	if(td->request.USER_AGENT[0])
	{
		strncat(cgiEnvString,"\rHTTP_USER_AGENT=",strlen(cgiEnvString)+18);
		strncat(cgiEnvString,td->request.USER_AGENT,strlen(cgiEnvString)+strlen(td->request.USER_AGENT)+1);
	}
	if(td->request.ACCEPTENC[0])
	{
		strncat(cgiEnvString,"\rHTTP_ACCEPT_ENCODING=",strlen(cgiEnvString)+23);
		strncat(cgiEnvString,td->request.ACCEPTENC,strlen(cgiEnvString)+strlen(td->request.ACCEPTENC)+1);
	}
	if(td->request.ACCEPTLAN[0])
	{
		strncat(cgiEnvString,"\rHTTP_ACCEPT_LANGUAGE=",strlen(cgiEnvString)+23);
	  	strncat(cgiEnvString,td->request.ACCEPTLAN,strlen(cgiEnvString)+strlen(td->request.ACCEPTLAN)+1);
	}
	if(td->pathInfo[0])
	{
		strncat(cgiEnvString,"\rPATH_INFO=",strlen(cgiEnvString)+12);
	  	strncat(cgiEnvString,td->pathInfo,strlen(cgiEnvString)+strlen(td->pathInfo)+1);
	  	
	  	strncat(cgiEnvString,"\rPATH_TRANSLATED=",strlen(cgiEnvString)+18);
		strncat(cgiEnvString,td->pathTranslated,strlen(cgiEnvString)+strlen(td->pathTranslated)+1);
	}
	else
	{
  		strncat(cgiEnvString,"\rPATH_TRANSLATED=",strlen(cgiEnvString)+18);
		strncat(cgiEnvString,td->filenamePath,strlen(cgiEnvString)+strlen(td->filenamePath)+1);
	}

 	strncat(cgiEnvString,"\rSCRIPT_FILENAME=",strlen(cgiEnvString)+18);
	strncat(cgiEnvString,td->filenamePath,strlen(cgiEnvString)+strlen(td->filenamePath)+1);
	
	/*!
	*For the DOCUMENT_URI and SCRIPT_NAME copy the requested URI without the pathInfo.
	*/
	strncat(cgiEnvString,"\rSCRIPT_NAME=/",strlen(cgiEnvString)+15);
	lstrcpyn(&cgiEnvString[strlen(cgiEnvString)],td->request.URI,(int)(strlen(td->request.URI)-strlen(td->pathInfo)+1));

	strncat(cgiEnvString,"\rSCRIPT_URL=/",strlen(cgiEnvString)+14);
	lstrcpy(&cgiEnvString[strlen(cgiEnvString)],td->request.URI);

	strncat(cgiEnvString,"\rDATE_GMT=",strlen(cgiEnvString)+11);
	getRFC822GMTTime(&cgiEnvString[strlen(cgiEnvString)],HTTP_RESPONSE_DATE_DIM);

 	strncat(cgiEnvString,"\rDATE_LOCAL=",strlen(cgiEnvString)+13);
	getRFC822LocalTime(&cgiEnvString[strlen(cgiEnvString)],HTTP_RESPONSE_DATE_DIM);

	strncat(cgiEnvString,"\rDOCUMENT_ROOT=",strlen(cgiEnvString)+16);
	strncat(cgiEnvString,((vhost*)(td->connection->host))->documentRoot,strlen(cgiEnvString+1));

	strncat(cgiEnvString,"\rDOCUMENT_URI=/",strlen(cgiEnvString)+15);
	lstrcpyn(&cgiEnvString[strlen(cgiEnvString)],td->request.URI,(int)(strlen(td->request.URI)-strlen(td->pathInfo)+1));
	
	strncat(cgiEnvString,"\rDOCUMENT_NAME=",strlen(cgiEnvString)+16);
	strncat(cgiEnvString,td->filenamePath,strlen(cgiEnvString)+strlen(td->filenamePath)+1);

	if(td->identity[0])
	{
  		strncat(cgiEnvString,"\rREMOTE_IDENT=",strlen(cgiEnvString)+15);
    	strncat(cgiEnvString,td->identity,strlen(cgiEnvString)+strlen(td->identity)+1);
	}
#ifdef WIN32
	if(processEnv)
	{
		strncat(cgiEnvString,"\r",strlen(cgiEnvString)+2);
  		LPTSTR lpszVariable; 
		LPVOID lpvEnv; 
		lpvEnv = lserver->envString; 
		for (lpszVariable = (LPTSTR) lpvEnv; *lpszVariable; lpszVariable++) 
		{ 
			if(((char*)lpszVariable)[0] !='=')
			{
				strncat(cgiEnvString,(char*)lpszVariable,strlen(cgiEnvString)+strlen(lpszVariable)+1);
				strncat(cgiEnvString,"\r",strlen(cgiEnvString)+2);
			}
			while (*lpszVariable)*lpszVariable++;
		} 
	}
#endif

	strncat(cgiEnvString,"\r\0\0\0\0\0",strlen(cgiEnvString)+7);
 	size_t max=strlen(cgiEnvString);
	for(size_t i=0;i<max;i++)
		if(cgiEnvString[i]=='\r')
			cgiEnvString[i]='\0';
}
