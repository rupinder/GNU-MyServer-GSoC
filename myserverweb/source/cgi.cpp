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
*Sends the myServer CGI; differently form standard CGI this don't need a new process to run
*so it is faster.
*/
BOOL sendMSCGI(httpThreadContext* td,LPCONNECTION s,char* exec,char* cmdLine)
{
	/*
	*This is the code for manage a .mscgi file.
	*This files differently from standard CGI don't need a new process to run
	*but are allocated in the caller process virtual space.
	*Usually these files are faster than standard CGI.
	*Actually myServerCGI(.mscgi) is only at an alpha status.
	*/
#ifdef WIN32
	static HMODULE hinstLib; 
    static CGIMAIN ProcMain;
	static CGIINIT ProcInit;
 
    hinstLib = LoadLibrary(exec); 
	td->buffer2[0]='\0';
	if (hinstLib) 
    { 
		ProcInit = (CGIINIT) GetProcAddress(hinstLib, "initialize");
		ProcMain = (CGIMAIN) GetProcAddress(hinstLib, "main"); 
		if(ProcInit && ProcMain)
		{
			(ProcInit)((LPVOID)td->buffer,(LPVOID)td->buffer2,(LPVOID)&(td->response),(LPVOID)&(td->request));
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
				return raiseHTTPError(td,s,e_403);
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
	static int len;
	len=lstrlen(td->buffer2);
	sprintf(td->response.CONTENTS_DIM,"%u",len);
	buildHTTPResponseHeader(td->buffer,&td->response);
	ms_send(s->socket,td->buffer,lstrlen(td->buffer), 0);
	ms_send(s->socket,td->buffer2,len, 0);
	return 1;
#elif
	/*
	*On the platforms that is not available the support for the MSCGI send a 
	*non implemented error.
	*/
	return raiseHTTPError(td,s,e_501);
#endif
}

/*
*Sends the standard CGI to a client.
*/
BOOL sendCGI(httpThreadContext* td,LPCONNECTION s,char* scriptpath,char* /*ext*/,char *execpath,int cmd)
{
	/*
	*Change the owner of the thread to the creator of the process.
	*This because anonymous users cannot go through our files.
	*/
	if(lserver->mustUseLogonOption())
		revertToSelf();
	char cmdLine[MAX_PATH*2];
	char filename[MAX_PATH];

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
		lstrcpy(execpath,"cmd /c");
#endif
	}
	else if(cmd==CGI_CMD_RUNCGI)
	{
		getFilename(execpath,filename);
	}
	else
	{
		/*
		*If the command was not recognized send an 501 page error.
		*/
		return raiseHTTPError(td,s,e_501);
	}
	/*
	*Determine if the CGI executable is nph.
	*/
	BOOL nph=(strnicmp("nph-",filename, 4)==0)?1:0;

	sprintf(cmdLine,"%s \"%s\"",execpath,scriptpath);
    /*
    *Use a temporary file to store CGI output.
    *Every thread has it own tmp file name(stdOutFilePath),
    *so use this name for the file that is going to be
    *created because more threads can access more CGI in the same time.
    */

	char currentpath[MAX_PATH];
	char stdOutFilePath[MAX_PATH];
	char stdInFilePath[MAX_PATH];
	ms_getcwd(currentpath,MAX_PATH);
	static DWORD id=0;
	id++;
	sprintf(stdOutFilePath,"%s/stdOutFile_%u",currentpath,id);
	sprintf(stdInFilePath,"%s/stdInFile_%u",currentpath,id);
		
	/*
	*Standard CGI uses standard output to output the result and the standard 
	*input to get other params like in a POST request.
	*/
	MYSERVER_FILE_HANDLE stdOutFile = ms_CreateTemporaryFile(stdOutFilePath);
	MYSERVER_FILE_HANDLE stdInFile = ms_CreateTemporaryFile(stdInFilePath);
	
	DWORD nbw;
	if(td->request.URIOPTSPTR)
	{
		ms_WriteToFile(stdInFile,td->request.URIOPTSPTR,atoi(td->request.CONTENTS_DIM),&nbw);
		char *endFileStr="\r\n\r\n\0";
		ms_WriteToFile(stdInFile,endFileStr,lstrlen(endFileStr),&nbw);
	}

	/*
	*With this code we execute the CGI process.
	*Use the td->buffer2 to build the environment string.
	*/
	START_PROC_INFO spi;
	spi.cmdLine = cmdLine;
	spi.stdError = (MYSERVER_FILE_HANDLE)0;
	spi.stdIn = (MYSERVER_FILE_HANDLE)stdInFile;
	spi.stdOut = (MYSERVER_FILE_HANDLE)stdOutFile;
	spi.envString=td->buffer2;
	/*
	*Build the environment string used by the CGI started
	*by the execHiddenProcess(...) function.
	*/
	buildCGIEnvironmentString(td,td->buffer2);
	execHiddenProcess(&spi);

	/*
	*Read the CGI output.
	*/
	ms_CloseFile(stdOutFile);
	stdOutFile=ms_OpenFile(stdOutFilePath,MYSERVER_FILE_OPEN_IFEXISTS|MYSERVER_FILE_OPEN_READ);
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
	/*
	*In the buffer2 there are the CGI HTTP header and the 
	*contents of the page requested through the CGI.
	*If the client do an HEAD request send only the HTTP header.
	*/
	if(!lstrcmpi(td->request.CMD,"HEAD"))
		ms_send(s->socket,td->buffer2,headerSize, 0);
	else
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
void buildCGIEnvironmentString(httpThreadContext* td,char *cgiEnvString)
{
	cgiEnvString[0]='\0';
	/*
	*The Environment string is a null-terminated block of null-terminated strings.
	*Cause we use the function lstrcat we use the character \r for the \0 character
	*and at the end we change every \r in \0.
	*/
	lstrcat(cgiEnvString,"SERVER_SOFTWARE=myServer");
	lstrcat(cgiEnvString,versionOfSoftware);

	lstrcat(cgiEnvString,"\rSERVER_NAME=");
	lstrcat(cgiEnvString,lserver->getServerName());

	lstrcat(cgiEnvString,"\rQUERY_STRING=");
	lstrcat(cgiEnvString,td->request.URIOPTS);

	lstrcat(cgiEnvString,"\rGATEWAY_INTERFACE=CGI/1.1");
	
	lstrcat(cgiEnvString,"\rSERVER_PROTOCOL=HTTP/1.1");
	
	lstrcat(cgiEnvString,"\rSERVER_PORT=");
	sprintf(&cgiEnvString[lstrlen(cgiEnvString)],"%u",lserver->port_HTTP);
    
	lstrcat(cgiEnvString,"\rREQUEST_METHOD=");
	lstrcat(cgiEnvString,td->request.CMD);

	lstrcat(cgiEnvString,"\rHTTP_USER_AGENT=");
	lstrcat(cgiEnvString,td->request.USER_AGENT);

	lstrcat(cgiEnvString,"\rHTTP_ACCEPT=");
	lstrcat(cgiEnvString,td->request.ACCEPT);
	
	lstrcat(cgiEnvString,"\rCONTENT_TYPE=");
	lstrcat(cgiEnvString,td->request.CONTENTS_TYPE);
	
	lstrcat(cgiEnvString,"\rCONTENT_LENGTH=");
	lstrcat(cgiEnvString,td->request.CONTENTS_DIM);

	lstrcat(cgiEnvString,"\rREMOTE_USER=");
	lstrcat(cgiEnvString,td->connection->login);

	lstrcat(cgiEnvString,"\rREMOTE_ADDR=");
	lstrcat(cgiEnvString,td->connection->ipAddr);

	lstrcat(cgiEnvString,"\rAUTH_TYPE=");
	lstrcat(cgiEnvString,td->request.AUTH);

	lstrcat(cgiEnvString,"\rCONTENT_ENCODING=");
	lstrcat(cgiEnvString,td->request.ACCEPTENC);		
/*
	lstrcat(cgiEnvString,"\rPATH_INFO=");
	lstrcat(cgiEnvString,td->request.URI);

	lstrcat(cgiEnvString,"\rREMOTE_HOST=");
	lstrcat(cgiEnvString,td->request.HOST);

	lstrcat(cgiEnvString,"\rPATH_TRANSLATED=");
	lstrcat(cgiEnvString,td->request.URI);

	lstrcat(cgiEnvString,"\rSCRIPT_NAME=");
	lstrcat(cgiEnvString,td->request.URI);

	lstrcat(cgiEnvString,"\rREMOTE_IDENT=");
	lstrcat(cgiEnvString,td->request.HOST);
*/
	
	lstrcat(cgiEnvString,"\r\0\0");
	int max=lstrlen(cgiEnvString);
	for(int i=0;i<max;i++)
		if(cgiEnvString[i]=='\r')
			cgiEnvString[i]='\0';
}