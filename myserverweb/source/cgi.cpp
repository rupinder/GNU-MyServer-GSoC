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
*Run the standard CGI and send the result to the client.
*/
int sendCGI(httpThreadContext* td,LPCONNECTION s,char* scriptpath,char* /*ext*/,char *cgipath,int cmd)
{
	/*
	*Change the owner of the thread to the creator of the process.
	*This because anonymous users cannot go through our files.
	*/
	if(lserver->mustUseLogonOption())
		revertToSelf();
	/*
	*Use this variable to determine if the CGI executable is nph(Non Parsed Header).
	*/
	int nph;
	char cmdLine[MAX_PATH*3+1];
	char filename[MAX_PATH];
	lstrcpy(td->scriptPath,scriptpath);
	splitPath(scriptpath,td->scriptDir,td->scriptFile);

	splitPath(cgipath,td->cgiRoot,td->cgiFile);

	/*
	*If the cmd is equal to CGI_CMD_EXECUTE then we must execute the
	*scriptpath file as an executable.
	*Then to determine if is a nph CGI we must use the scriptpath
	*string.
	*/
	if(cmd==CGI_CMD_EXECUTE)
	{
		getFilename(scriptpath,filename);
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
		if(!ms_FileExists(cgipath))
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
    *Every thread has it own tmp file name(stdOutFilePath),
    *so use this name for the file that is going to be
    *created because more threads can access more CGI at the same time.
    */
	char currentpath[MAX_PATH];
	char stdOutFilePath[MAX_PATH];
	char stdInFilePath[MAX_PATH];
	ms_getdefaultwd(currentpath,MAX_PATH);
	sprintf(stdOutFilePath,"%s/stdOutFile_%u",currentpath,td->id);

		
	/*
	*Standard CGI uses standard output to output the result and the standard 
	*input to get other params like in a POST request.
	*/
	MYSERVER_FILE_HANDLE stdOutFile = ms_CreateTemporaryFile(stdOutFilePath);
	MYSERVER_FILE_HANDLE stdInFile;
	
	if(td->inputData==0)/*If no exists the stdin file create one and copy in all the necessary informations*/
	{
		u_long nbw;/*Number of bytes written to the stdin file if any*/
		sprintf(stdInFilePath,"%s/stdInFile_%u",currentpath,td->id);
		stdInFile = ms_CreateTemporaryFile(stdInFilePath);
		if(td->request.URIOPTSPTR)
			ms_WriteToFile(stdInFile,td->request.URIOPTSPTR,atoi(td->request.CONTENTS_DIM),&nbw);
		else
			ms_WriteToFile(stdInFile,td->request.URIOPTS,lstrlen(td->request.URIOPTS),&nbw);

		char *endFileStr="\r\n\r\n";
		ms_WriteToFile(stdInFile,endFileStr,lstrlen(endFileStr),&nbw);
		ms_setFilePointer(stdInFile,0);
	}
	else
	{
		stdInFile = td->inputData;
	}

	/*
	*Build the environment string used by the CGI started
	*by the execHiddenProcess(...) function.
	*Use the td->buffer2 to build the environment string.
	*/
	buildCGIEnvironmentString(td,td->buffer2);


	/*
	*With this code we execute the CGI process.
	*/
	START_PROC_INFO spi;
	spi.cmdLine = cmdLine;
	spi.cwd=td->scriptDir;
	spi.stdError = (MYSERVER_FILE_HANDLE)stdOutFile;
	spi.stdIn = (MYSERVER_FILE_HANDLE)stdInFile;
	spi.stdOut = (MYSERVER_FILE_HANDLE)stdOutFile;
	spi.envString=td->buffer2;
	execHiddenProcess(&spi);

	/*
	*Read the CGI output.
	*/
	u_long nBytesRead=0;
	if(!ms_setFilePointer(stdOutFile,0))
		ms_ReadFromFile(stdOutFile,td->buffer2,td->buffersize2,&nBytesRead);
	else
		td->buffer2[0]='\0';

	int yetoutputted=0;
	/*
	*Standard CGI can include an extra HTTP header so do not 
	*terminate with \r\n the default myServer header.
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
		else
			if(!strncmp(&td->buffer2[i],"Location",8))
			{
				char nURL[MAX_PATH];
				int j=0;
				while(td->buffer2[i+j+10]!='\r')
				{
					nURL[j]=td->buffer2[i+j+10];
					nURL[j+1]='\0';
					j++;
				}
				if(!yetoutputted)
					sendHTTPRedirect(td,s,nURL);
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
			resetHTTPResponse(&(td->response));
		}
		buildHTTPResponseHeaderStruct(td,td->buffer2);
		/*
		*Always specify the size of the HTTP contents.
		*/
		sprintf(td->response.CONTENTS_DIM,"%u",nBytesRead-headerSize);
		buildHTTPResponseHeader(td->buffer,&td->response);
		ms_send(s->socket,td->buffer,strlen(td->buffer), 0);
		ms_send(s->socket,(char*)(td->buffer2+headerSize),nBytesRead-headerSize, 0);

	}
	
	/*
	*Close and delete the stdin and stdout files used by the CGI.
	*/
	ms_CloseFile(stdOutFile);
	ms_CloseFile(stdInFile);
	td->inputData=(MYSERVER_FILE_HANDLE)0;

	/*
	*Restore security on the current thread.
	*/
	if(lserver->mustUseLogonOption())
		impersonateLogonUser(&td->hImpersonation);
	/*
	*By default don't close the connection.
	*/
	return 1;
}
/*
*Write the string that contain the CGI environment to cgiEnvString.
*/
void buildCGIEnvironmentString(httpThreadContext* td,char *cgiEnvString)
{
	/*
	*The Environment string is a null-terminated block of null-terminated strings.
	*For no errors with the function lstrcat we use the character \r for the \0 character
	*and at the end we change every \r in \0.
	*/
	lstrcpy(cgiEnvString,"SERVER_SOFTWARE=myServer");
	lstrcat(cgiEnvString,versionOfSoftware);
#ifdef WIN32
	lstrcat(cgiEnvString," (Win32)");
#endif

	lstrcat(cgiEnvString,"\rSERVER_NAME=");
	lstrcat(cgiEnvString,lserver->getServerName());

	lstrcat(cgiEnvString,"\rSERVER_SIGNATURE=");
	lstrcat(cgiEnvString,"<address>myServer ");
	lstrcat(cgiEnvString,versionOfSoftware);
	lstrcat(cgiEnvString,"</address>");

	lstrcat(cgiEnvString,"\rSERVER_PROTOCOL=HTTP/");
	lstrcat(cgiEnvString,td->request.VER);		

	lstrcat(cgiEnvString,"\rSERVER_PORT=");
	sprintf(&cgiEnvString[lstrlen(cgiEnvString)],"%u",lserver->port_HTTP);

	lstrcat(cgiEnvString,"\rREQUEST_METHOD=");
	lstrcat(cgiEnvString,td->request.CMD);

	lstrcat(cgiEnvString,"\rQUERY_STRING=");
	lstrcat(cgiEnvString,td->request.URIOPTS);

	lstrcat(cgiEnvString,"\rGATEWAY_INTERFACE=CGI/1.1");
	    

	if(td->request.CONTENTS_TYPE[0])
	{
		lstrcat(cgiEnvString,"\rCONTENT_TYPE=");
		lstrcat(cgiEnvString,td->request.CONTENTS_TYPE);
	}
	
	if(td->request.CONTENTS_DIM[0])
	{
		lstrcat(cgiEnvString,"\rCONTENT_LENGTH=");
		lstrcat(cgiEnvString,td->request.CONTENTS_DIM);
	}

	if(td->request.COOKIE[0])
	{
		lstrcat(cgiEnvString,"\rHTTP_COOKIE=");
		lstrcat(cgiEnvString,td->request.COOKIE);
	}

	if(td->request.REFERER[0])
	{
		lstrcat(cgiEnvString,"\rHTTP_REFERER=");
		lstrcat(cgiEnvString,td->request.REFERER);
	}

	lstrcat(cgiEnvString,"\rREQUEST_URI=/");
	lstrcpyn(&cgiEnvString[lstrlen(cgiEnvString)],td->request.URI,lstrlen(td->request.URI)-lstrlen(td->pathInfo)+1);

	if(td->request.ACCEPT[0])
	{
		lstrcat(cgiEnvString,"\rHTTP_ACCEPT=");
		lstrcat(cgiEnvString,td->request.ACCEPT);
	}
	if(td->cgiRoot[0])
	{
		lstrcat(cgiEnvString,"\rCGI_ROOT=");
		lstrcat(cgiEnvString,td->cgiRoot);
	}
	if(td->connection->ipAddr[0])
	{
		lstrcat(cgiEnvString,"\rHTTP_HOST=");
		lstrcat(cgiEnvString,td->connection->ipAddr);
	}
	if(td->connection->ipAddr[0])
	{
		lstrcat(cgiEnvString,"\rREMOTE_ADDR=");
		lstrcat(cgiEnvString,td->connection->ipAddr);
	}
	if(td->connection->port)
	{
		lstrcat(cgiEnvString,"\rREMOTE_PORT=");
		sprintf(&cgiEnvString[lstrlen(cgiEnvString)],"%u",td->connection->port);
	}

	if(td->connection->login[0])
	{
		lstrcat(cgiEnvString,"\rREMOTE_USER=");
		lstrcat(cgiEnvString,td->connection->login);
	}

	if(td->request.CONNECTION[0])
	{
		lstrcat(cgiEnvString,"\rHTTP_CONNECTION=");
		lstrcat(cgiEnvString,td->request.CONNECTION);
	}
	if(td->request.AUTH[0])
	{
		lstrcat(cgiEnvString,"\rAUTH_TYPE=");
		lstrcat(cgiEnvString,td->request.AUTH);
	}

	if(td->request.USER_AGENT[0])
	{
		lstrcat(cgiEnvString,"\rHTTP_USER_AGENT=");
		lstrcat(cgiEnvString,td->request.USER_AGENT);
	}
	if(td->request.ACCEPTENC[0])
	{
		lstrcat(cgiEnvString,"\rHTTP_ACCEPT_ENCODING=");
		lstrcat(cgiEnvString,td->request.ACCEPTENC);	
	}
	if(td->request.ACCEPTLAN[0])
	{
		lstrcat(cgiEnvString,"\rHTTP_ACCEPT_LANGUAGE=");
		lstrcat(cgiEnvString,td->request.ACCEPTLAN);	
	}

	if(td->pathInfo[0])
	{
		lstrcat(cgiEnvString,"\rPATH_INFO=");
		lstrcat(cgiEnvString,td->pathInfo);

		lstrcat(cgiEnvString,"\rPATH_TRANSLATED=");
		lstrcat(cgiEnvString,td->pathTranslated);
	}
	else
	{
	/*
	*Several packages have chosen to interpret PATH_TRANSLATED as the physical translated path 
	*of the request. To accomodate these applications, PATH_TRANSLATED is mapped to this 
	*interpretation if no PATH_INFO is provided. 	
	*/
		lstrcat(cgiEnvString,"\rPATH_TRANSLATED=");
		getPath(td->pathTranslated,td->request.URI,FALSE);
		lstrcat(cgiEnvString,td->pathTranslated);	
	}

	lstrcat(cgiEnvString,"\rSCRIPT_FILENAME=");
	lstrcat(cgiEnvString,td->filenamePath);

	lstrcat(cgiEnvString,"\rDOCUMENT_NAME=");
	lstrcat(cgiEnvString,td->filenamePath);

	/*
	*For the DOCUMENT_URI and SCRIPT_NAME Just copy the requested URI without any pathInfo.
	*/
	lstrcat(cgiEnvString,"\rSCRIPT_NAME=/");
	lstrcpyn(&cgiEnvString[lstrlen(cgiEnvString)],td->request.URI,lstrlen(td->request.URI)-lstrlen(td->pathInfo)+1);

	lstrcat(cgiEnvString,"\rDOCUMENT_URI=/");
	lstrcpyn(&cgiEnvString[lstrlen(cgiEnvString)],td->request.URI,lstrlen(td->request.URI)-lstrlen(td->pathInfo)+1);

	lstrcat(cgiEnvString,"\rDATE_GMT=");
	lstrcat(cgiEnvString,getRFC822GMTTime());

	lstrcat(cgiEnvString,"\rDATE_LOCAL=");
	lstrcat(cgiEnvString,getRFC822LocalTime());

	lstrcat(cgiEnvString,"\rDOCUMENT_ROOT=");
	lstrcat(cgiEnvString,lserver->getPath());

	lstrcat(cgiEnvString,"\rSERVER_ADMIN=");
	lstrcat(cgiEnvString,lserver->getServerAdmin());

	if(td->identity[0])
	{
		lstrcat(cgiEnvString,"\rREMOTE_IDENT=");
		lstrcat(cgiEnvString,td->identity);
	}

	lstrcat(cgiEnvString,"\r\0\0");
	int max=lstrlen(cgiEnvString);
	for(int i=0;i<max;i++)
		if(cgiEnvString[i]=='\r')
			cgiEnvString[i]='\0';
}