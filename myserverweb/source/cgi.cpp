/*
*myServer
*Copyright (C) 2002 Giuseppe Scrivano
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

#ifndef WIN32
#define lstrcpy strcpy
#define lstrlen strlen
#define lstrcpyn strncpy
#define strcat strcat
#define strnicmp strncmp
#endif

/*
*Run the standard CGI and send the result to the client.
*/
int sendCGI(httpThreadContext* td,LPCONNECTION s,char* scriptpath,char* /*ext*/,char *cgipath,int cmd)
{
	/*
	*Use this variable to determine if the CGI executable is nph(Non Parsed Header).
	*/
	int nph;
	char cmdLine[MAX_PATH*3+1];
	char filename[MAX_PATH];
	lstrcpy(td->scriptPath,scriptpath);
	MYSERVER_FILE::splitPath(scriptpath,td->scriptDir,td->scriptFile);

	MYSERVER_FILE::splitPath(cgipath,td->cgiRoot,td->cgiFile);

	/*
	*If the cmd is equal to CGI_CMD_EXECUTE then we must execute the
	*scriptpath file as an executable.
	*Then to determine if is a nph CGI we must use the scriptpath
	*string.
	*/
	if(cmd==CGI_CMD_EXECUTE)
	{
		MYSERVER_FILE::getFilename(scriptpath,filename);
#ifdef WIN32
		/*
		*Under the windows platform to run a file like an executable
		*use the sintact "cmd /c filename".
		*/
		sprintf(cmdLine,"cmd /c %s",td->scriptFile);
		nph=(strnicmp("nph-",td->scriptFile, 4)==0)?1:0;
#endif
	}
	else if(cmd==CGI_CMD_RUNCGI)
	{
		if(!MYSERVER_FILE::fileExists(cgipath))
		{
			return raiseHTTPError(td,s,e_500);
		}
		sprintf(cmdLine,"%s %s",cgipath,td->scriptFile);
		nph=(strnicmp("nph-",td->cgiFile, 4)==0)?1:0;
	}
	else
	{
		/*
		*If the command was not recognized send an 501 page error.
		*/
		return raiseHTTPError(td,s,e_501);
	}



    /*
    *Use a temporary file to store CGI output.
    *Every thread has it own tmp file name(td->outputDataPath),
    *so use this name for the file that is going to be
    *created because more threads can access more CGI at the same time.
    */
	char currentpath[MAX_PATH];
	getdefaultwd(currentpath,MAX_PATH);
	sprintf(td->outputDataPath,"%s/stdOutFile_%u",currentpath,td->id);

		
	/*
	*Standard CGI uses STDOUT to output the result and the STDIN 
	*to get other params like in a POST request.
	*/
	MYSERVER_FILE stdOutFile;
	stdOutFile.createTemporaryFile(td->outputDataPath);
	MYSERVER_FILE stdInFile;
	td->inputData.closeFile();	
	stdInFile.openFile(td->inputDataPath,MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_ALWAYS);

	/*
	*Build the environment string used by the CGI started
	*by the execHiddenProcess(...) function.
	*Use the td->buffer2 to build the environment string.
	*/
	td->buffer2[0]='\0';
	buildCGIEnvironmentString(td,td->buffer2);


	/*
	*With this code we execute the CGI process.
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
	spi.envString=td->buffer2;
	execHiddenProcess(&spi);
	td->buffer2[0]='\0';
	/*
	*Read the CGI output.
	*/
	u_long nBytesRead=0;
	if(!stdOutFile.setFilePointer(0))
		stdOutFile.readFromFile(td->buffer2,KB(5),&nBytesRead);
	else
		td->buffer2[0]='\0';
	int yetoutputted=0;
	if(nBytesRead==0)
	{
		raiseHTTPError(td,s,e_500);
		yetoutputted=1;
	}
	/*
	*Standard CGI can include an extra HTTP header.
	*/
	u_long headerSize=0;

	for(u_long i=0;i<nBytesRead;i++)
	{
		if((td->buffer2[i]=='\r')&&(td->buffer2[i+1]=='\n')&&(td->buffer2[i+2]=='\r')&&(td->buffer2[i+3]=='\n'))
		{
			/*
			*The HTTP header ends with a \r\n\r\n sequence so 
			*determinate where it ends and set the header size
			*to i + 4.
			*/
			headerSize=i+4;
			break;
		}
		else if(!strncmp(&td->buffer2[i],"Location",8))
			{
				char nURL[MAX_PATH];
				nURL[0]='\0';
				u_long j;
				j=0;

				int start=(int)strlen(nURL);
				while(td->buffer2[i+j+10]!='\r')
				{

					nURL[j+start]=td->buffer2[i+j+10];
					nURL[j+start+1]='\0';
					j++;
				}
				if(!yetoutputted)
				{
					sendHTTPRedirect(td,s,nURL);
				}
				yetoutputted=1;
			}
	}
	if(!yetoutputted)
	{
		/*
		*Don't send any other HTTP header if the CGI executable has the nph-.... form name.
		*/
		if(nph)
		{
			/*
			*Resetting the structure we send only the information gived by the CGI.
			*/
			resetHTTPResponse(&(td->response));
		}
		if(headerSize)
			buildHTTPResponseHeaderStruct(&td->response,td,td->buffer2);
		/*
		*Always specify the size of the HTTP contents.
		*/
		sprintf(td->response.CONTENT_LENGTH,"%u",stdOutFile.getFileSize()-headerSize);
		buildHTTPResponseHeader(td->buffer,&td->response);
		s->socket.send(td->buffer,(int)strlen(td->buffer), 0);
		s->socket.send((char*)(td->buffer2+headerSize),nBytesRead-headerSize, 0);
		while(stdOutFile.readFromFile(td->buffer2,td->buffersize2,&nBytesRead))
		{
			if(nBytesRead)
				s->socket.send((char*)td->buffer2,nBytesRead, 0);
			else
				break;
		}
		
	}
	
	/*
	*Close and delete the stdin and stdout files used by the CGI.
	*/
	stdOutFile.closeFile();
	stdInFile.closeFile();
	MYSERVER_FILE::deleteFile(td->inputDataPath);

	/*
	*By default don't close the connection.
	*/
	return 1;
}
/*
*Write the string that contain the CGI environment to cgiEnvString.
*This function is used by other server side protocols too.
*/
void buildCGIEnvironmentString(httpThreadContext* td,char *cgiEnvString,int processEnv)
{
	/*
	*The Environment string is a null-terminated block of null-terminated strings.
	*For no errors with the function strcat we use the character \r for the \0 character
	*and at the end we change every \r in \0.
	*/
	strcat(cgiEnvString,"SERVER_SOFTWARE=myServer");
	strcat(cgiEnvString,versionOfSoftware);

#ifdef WIN32
	strcat(cgiEnvString," (WIN32)");
#endif
#ifdef __linux__
	strcat(cgiEnvString," (Linux)");
#endif
	// Must use REDIRECT_STATUS for php and others
	strcat(cgiEnvString,"\rREDIRECT_STATUS=TRUE");

	strcat(cgiEnvString,"\rSERVER_NAME=");
	strcat(cgiEnvString,lserver->getServerName());

	strcat(cgiEnvString,"\rSERVER_SIGNATURE=");
	strcat(cgiEnvString,"<address>myServer ");
	strcat(cgiEnvString,versionOfSoftware);
	strcat(cgiEnvString,"</address>");

	switch(((vhost*)(td->connection->host))->protocol)
	{
		case PROTOCOL_HTTP:
			strcat(cgiEnvString,"\rSERVER_PROTOCOL=HTTP/");
			strcat(cgiEnvString,td->request.VER);		
			break;
	}
	strcat(cgiEnvString,"\rSERVER_PORT=");
	sprintf(&cgiEnvString[strlen(cgiEnvString)],"%u",td->connection->localPort);

	strcat(cgiEnvString,"\rSERVER_ADMIN=");
	strcat(cgiEnvString,lserver->getServerAdmin());

	strcat(cgiEnvString,"\rREQUEST_METHOD=");
	strcat(cgiEnvString,td->request.CMD);

	strcat(cgiEnvString,"\rREQUEST_URI=/");
	lstrcpyn(&cgiEnvString[strlen(cgiEnvString)],td->request.URI,strlen(td->request.URI)-strlen(td->pathInfo)+1);

	strcat(cgiEnvString,"\rQUERY_STRING=");
	strcat(cgiEnvString,td->request.URIOPTS);

	strcat(cgiEnvString,"\rGATEWAY_INTERFACE=CGI/1.1");

	if(td->request.CONTENT_TYPE[0])
	{
		strcat(cgiEnvString,"\rCONTENT_TYPE=");
		strcat(cgiEnvString,td->request.CONTENT_TYPE);
	}
	
	if(td->request.CONTENT_LENGTH[0])
	{
		strcat(cgiEnvString,"\rCONTENT_LENGTH=");
		strcat(cgiEnvString,td->request.CONTENT_LENGTH);
	}

	if(td->request.COOKIE[0])
	{
		strcat(cgiEnvString,"\rHTTP_COOKIE=");
		strcat(cgiEnvString,td->request.COOKIE);
	}

	if(td->request.REFERER[0])
	{
		strcat(cgiEnvString,"\rHTTP_REFERER=");
		strcat(cgiEnvString,td->request.REFERER);
	}
	if(td->request.CACHE_CONTROL[0])
	{
		strcat(cgiEnvString,"\rHTTP_CACHE_CONTROL=");
		strcat(cgiEnvString,td->request.CACHE_CONTROL);
	}
	if(td->request.ACCEPT[0])
	{
		strcat(cgiEnvString,"\rHTTP_ACCEPT=");
		strcat(cgiEnvString,td->request.ACCEPT);
	}
	if(td->cgiRoot[0])
	{
		strcat(cgiEnvString,"\rCGI_ROOT=");
		strcat(cgiEnvString,td->cgiRoot);
	}
	if(td->connection->ipAddr[0])
	{
		strcat(cgiEnvString,"\rHTTP_HOST=");
		strcat(cgiEnvString,td->connection->ipAddr);
	}
	if(td->connection->ipAddr[0])
	{
		strcat(cgiEnvString,"\rREMOTE_ADDR=");
		strcat(cgiEnvString,td->connection->ipAddr);
	}
	if(td->connection->port)
	{
		strcat(cgiEnvString,"\rREMOTE_PORT=");
		sprintf(&cgiEnvString[strlen(cgiEnvString)],"%u",td->connection->port);
	}

	if(td->connection->login[0])
	{
		strcat(cgiEnvString,"\rREMOTE_USER=");
		strcat(cgiEnvString,td->connection->login);
	}

	if(td->request.CONNECTION[0])
	{
		strcat(cgiEnvString,"\rHTTP_CONNECTION=");
		strcat(cgiEnvString,td->request.CONNECTION);
	}
	if(td->request.AUTH[0])
	{
		strcat(cgiEnvString,"\rAUTH_TYPE=");
		strcat(cgiEnvString,td->request.AUTH);
	}
	if(td->request.USER_AGENT[0])
	{
		strcat(cgiEnvString,"\rHTTP_USER_AGENT=");
		strcat(cgiEnvString,td->request.USER_AGENT);
	}
	if(td->request.ACCEPTENC[0])
	{
		strcat(cgiEnvString,"\rHTTP_ACCEPT_ENCODING=");
		strcat(cgiEnvString,td->request.ACCEPTENC);	
	}
	if(td->request.ACCEPTLAN[0])
	{
		strcat(cgiEnvString,"\rHTTP_ACCEPT_LANGUAGE=");
		strcat(cgiEnvString,td->request.ACCEPTLAN);	
	}
	if(td->pathInfo[0])
	{
		strcat(cgiEnvString,"\rPATH_INFO=");
		strcat(cgiEnvString,td->pathInfo);

		strcat(cgiEnvString,"\rPATH_TRANSLATED=");
		strcat(cgiEnvString,td->pathTranslated);
	}

	strcat(cgiEnvString,"\rSCRIPT_FILENAME=");
	strcat(cgiEnvString,td->filenamePath);

	/*
	*For the DOCUMENT_URI and SCRIPT_NAME Just copy the requested URI without any pathInfo.
	*/
	strcat(cgiEnvString,"\rSCRIPT_NAME=/");
	lstrcpyn(&cgiEnvString[strlen(cgiEnvString)],td->request.URI,strlen(td->request.URI)-strlen(td->pathInfo)+1);

	strcat(cgiEnvString,"\rSCRIPT_URL=/");
	lstrcpy(&cgiEnvString[strlen(cgiEnvString)],td->request.URI);

	strcat(cgiEnvString,"\rDATE_GMT=");
	getRFC822GMTTime(&cgiEnvString[strlen(cgiEnvString)],HTTP_RESPONSE_DATE_DIM);

	strcat(cgiEnvString,"\rDATE_LOCAL=");
	getRFC822LocalTime(&cgiEnvString[strlen(cgiEnvString)],HTTP_RESPONSE_DATE_DIM);

	strcat(cgiEnvString,"\rDOCUMENT_ROOT=");
	strcat(cgiEnvString,((vhost*)(td->connection->host))->documentRoot);

	strcat(cgiEnvString,"\rDOCUMENT_URI=/");
	lstrcpyn(&cgiEnvString[strlen(cgiEnvString)],td->request.URI,strlen(td->request.URI)-strlen(td->pathInfo)+1);

	strcat(cgiEnvString,"\rDOCUMENT_NAME=");
	strcat(cgiEnvString,td->filenamePath);

	if(td->identity[0])
	{
		strcat(cgiEnvString,"\rREMOTE_IDENT=");
		strcat(cgiEnvString,td->identity);
	}

#ifdef WIN32
	if(processEnv)
	{
		strcat(cgiEnvString,"\r");
		LPTSTR lpszVariable; 
		LPVOID lpvEnv; 
		 
		lpvEnv = lserver->envString; 

		for (lpszVariable = (LPTSTR) lpvEnv; *lpszVariable; lpszVariable++) 
		{ 
			if(((char*)lpszVariable)[0] !='=')
			{
				strcat(cgiEnvString,(char*)lpszVariable);
				strcat(cgiEnvString,"\r");
			}
			while (*lpszVariable)*lpszVariable++;
		} 
	}

#endif

	strcat(cgiEnvString,"\r\0\0\0\0\0");
	int max=strlen(cgiEnvString);
	for(int i=0;i<max;i++)
		if(cgiEnvString[i]=='\r')
			cgiEnvString[i]='\0';
}
