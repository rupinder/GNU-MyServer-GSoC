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
*Sends the standard CGI to a client.
*/
BOOL sendCGI(httpThreadContext* td,LPCONNECTION s,char* scriptpath,char* /*ext*/,char *cgipath,int cmd)
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
	BOOL nph;
	char cmdLine[MAX_PATH*3+1];
	char filename[MAX_PATH];

	char scriptfile[MAX_PATH];
	char scriptdir[MAX_PATH];
	splitPath(scriptpath,scriptdir,scriptfile);

	char cgidir[MAX_PATH];
	char cgifile[MAX_PATH];
	splitPath(cgipath,cgidir,cgifile);

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
		sprintf(cmdLine,"cmd /c %s",scriptfile);
		nph=(strnicmp("nph-",scriptfile, 4)==0)?1:0;
#endif
	}
	else if(cmd==CGI_CMD_RUNCGI)
	{
		sprintf(cmdLine,"%s %s",cgipath,scriptfile);
		nph=(strnicmp("nph-",cgifile, 4)==0)?1:0;
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
	sprintf(stdInFilePath,"%s/stdInFile_%u",currentpath,td->id);
		
	/*
	*Standard CGI uses standard output to output the result and the standard 
	*input to get other params like in a POST request.
	*/
	MYSERVER_FILE_HANDLE stdOutFile = ms_CreateTemporaryFile(stdOutFilePath);
	MYSERVER_FILE_HANDLE stdInFile = ms_CreateTemporaryFile(stdInFilePath);
	
	DWORD nbw;
	
	if(td->request.URIOPTSPTR)
		ms_WriteToFile(stdInFile,td->request.URIOPTSPTR,atoi(td->request.CONTENTS_DIM),&nbw);
	else
		ms_WriteToFile(stdInFile,td->request.URIOPTS,lstrlen(td->request.URIOPTS),&nbw);

	char *endFileStr="\r\n\r\n";
	ms_WriteToFile(stdInFile,endFileStr,lstrlen(endFileStr),&nbw);
	setFilePointer(stdInFile,0);
	/*
	*Build the environment string used by the CGI started
	*by the execHiddenProcess(...) function.
	*Use the td->buffer2 to build the environment string.
	*/
	CGI_ENV_STRING_DATA cgiEnvStringData;
	cgiEnvStringData.pathinfo = 0;
	cgiEnvStringData.pathtranslated = 0;
	cgiEnvStringData.cgiroot = cgidir;
	cgiEnvStringData.scriptpath = scriptpath;
	cgiEnvStringData.scriptfile = scriptfile;
	buildCGIEnvironmentString(td,td->buffer2,&cgiEnvStringData);


	/*
	*With this code we execute the CGI process.
	*/
	START_PROC_INFO spi;
	spi.cmdLine = cmdLine;
	spi.cwd=scriptdir;
	spi.stdError = (MYSERVER_FILE_HANDLE)stdOutFile;
	spi.stdIn = (MYSERVER_FILE_HANDLE)stdInFile;
	spi.stdOut = (MYSERVER_FILE_HANDLE)stdOutFile;
	spi.envString=td->buffer2;
	execHiddenProcess(&spi);

	/*
	*Read the CGI output.
	*/
	DWORD nBytesRead=0;
	if(!setFilePointer(stdOutFile,0))
		ms_ReadFromFile(stdOutFile,td->buffer2,td->buffersize2,&nBytesRead);
	else
		td->buffer2[0]='\0';

	/*
	*Standard CGI can include an extra HTTP header so do not 
	*terminate with \r\n the default myServer header.
	*/
	DWORD headerSize=0;
	for(DWORD i=0;i<nBytesRead;i++)
	{
		if(td->buffer2[i]=='\r')
			if(td->buffer2[i+1]=='\n')
				if(td->buffer2[i+2]=='\r')
					if(td->buffer2[i+3]=='\n')
					{
						/*
						*The HTTP header ends with a \r\n\r\n sequence so 
						*determinate where it ends and set the header size
						*to i + 4.
						*/
						headerSize=i+4;
						break;
					}
	}
	sprintf(td->response.CONTENTS_DIM,"%u",nBytesRead-headerSize);
	buildHTTPResponseHeader(td->buffer,&td->response);

	/*
	*If there is an extra header, send lstrlen(td->buffer)-2 because the
	*last two characters are \r\n that terminating the HTTP header.
	*Don't send any header if the CGI executable has the nph-.... form name.
	*/
	if(!nph)
	{
		if(headerSize)
			ms_send(s->socket,td->buffer,lstrlen(td->buffer)-2, 0);
		else
			ms_send(s->socket,td->buffer,lstrlen(td->buffer), 0);
	}
	ms_send(s->socket,td->buffer2,nBytesRead, 0);

	
	/*
	*Close and delete the stdin and stdout files used by the CGI.
	*/
	ms_CloseFile(stdOutFile);
	ms_DeleteFile(stdOutFilePath);
	ms_CloseFile(stdInFile);
	ms_DeleteFile(stdInFilePath);

	/*
	*Restore security on the current thread.
	*/
	if(lserver->mustUseLogonOption())
		impersonateLogonUser(&td->hImpersonation);
	return 1;
}
/*
*Build the string that contain the CGI environment.
*/
void buildCGIEnvironmentString(httpThreadContext* td,char *cgiEnvString,CGI_ENV_STRING_DATA *data)
{
	cgiEnvString[0]='\0';
	/*
	*The Environment string is a null-terminated block of null-terminated strings.
	*Cause we use the function lstrcat we use the character \r for the \0 character
	*and at the end we change every \r in \0.
	*/
	lstrcat(cgiEnvString,"SERVER_SOFTWARE=myServer");

	lstrcat(cgiEnvString,"\rSERVER_VERSION=");
	lstrcat(cgiEnvString,versionOfSoftware);

	lstrcat(cgiEnvString,"\rSERVER_NAME=");
	lstrcat(cgiEnvString,lserver->getServerName());

	lstrcat(cgiEnvString,"\rQUERY_STRING=");
	lstrcat(cgiEnvString,td->request.URIOPTS);

	lstrcat(cgiEnvString,"\rGATEWAY_INTERFACE=CGI/1.1");
	
	lstrcat(cgiEnvString,"\rSERVER_PROTOCOL=HTTP/");
	lstrcat(cgiEnvString,td->request.VER);		

	lstrcat(cgiEnvString,"\rSERVER_PORT=");
	sprintf(&cgiEnvString[lstrlen(cgiEnvString)],"%u",lserver->port_HTTP);
    
	lstrcat(cgiEnvString,"\rREQUEST_METHOD=");
	lstrcat(cgiEnvString,td->request.CMD);

	lstrcat(cgiEnvString,"\rHTTP_USER_AGENT=");
	lstrcat(cgiEnvString,td->request.USER_AGENT);

	lstrcat(cgiEnvString,"\rHTTP_REFERER=");
	lstrcat(cgiEnvString,td->request.URI);

	lstrcat(cgiEnvString,"\rHTTP_ACCEPT=");
	lstrcat(cgiEnvString,td->request.ACCEPT);
	
	lstrcat(cgiEnvString,"\rCONTENT_TYPE=");
	lstrcat(cgiEnvString,td->request.CONTENTS_TYPE[0]?td->request.CONTENTS_TYPE:"0");
	
	lstrcat(cgiEnvString,"\rCONTENT_LENGTH=");
	lstrcat(cgiEnvString,td->request.CONTENTS_DIM[0]?td->request.CONTENTS_DIM:"0");

	lstrcat(cgiEnvString,"\rREMOTE_USER=");
	lstrcat(cgiEnvString,td->connection->login[0]?td->connection->login:"-");

	lstrcat(cgiEnvString,"\rREMOTE_ADDR=");
	lstrcat(cgiEnvString,td->connection->ipAddr);

	lstrcat(cgiEnvString,"\rCGI_ROOT=");
	lstrcat(cgiEnvString,data->cgiroot);
	
	lstrcat(cgiEnvString,"\rREMOTE_HOST=");
	lstrcat(cgiEnvString,td->connection->ipAddr);

	lstrcat(cgiEnvString,"\rAUTH_TYPE=");
	lstrcat(cgiEnvString,td->request.AUTH);

	lstrcat(cgiEnvString,"\rCONTENT_ENCODING=");
	lstrcat(cgiEnvString,td->request.ACCEPTENC);		
	
	if(data->pathinfo)
	{
		lstrcat(cgiEnvString,"\rPATH_INFO=");
		lstrcat(cgiEnvString,data->pathinfo);
	}

	if(data->pathtranslated)
	{
		lstrcat(cgiEnvString,"\rPATH_TRANSLATED=");
		lstrcat(cgiEnvString,data->pathtranslated);
	}
	if(data->scriptfile)
	{
		lstrcat(cgiEnvString,"\rSCRIPT_NAME=");
		lstrcat(cgiEnvString,data->scriptfile);
	}
	if(data->scriptpath)
	{
		lstrcat(cgiEnvString,"\rSCRIPT_PATH=");
		lstrcat(cgiEnvString,data->scriptpath);
	}
/*

	lstrcat(cgiEnvString,"\rREMOTE_IDENT=");
	lstrcat(cgiEnvString,td->request.HOST);
*/
	lstrcat(cgiEnvString,"\r\0\0");
	int max=lstrlen(cgiEnvString);
	for(int i=0;i<max;i++)
		if(cgiEnvString[i]=='\r')
			cgiEnvString[i]='\0';
}