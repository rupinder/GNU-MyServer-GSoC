/*
*myServer
*Copyright (C) 2002 The MyServer team
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
#include "../include/cgi.h"
#include "../include/AMMimeUtils.h"
#include "../include/filemanager.h"
#include "../include/sockets.h"
#include "../include/utility.h"

extern "C" {
#ifdef WIN32
#include <direct.h>
#endif
}

/*
*Sends the MyServer CGI; differently form standard CGI this don't need a new process to run
*so it is faster.
*/
int sendMSCGI(httpThreadContext* td,LPCONNECTION s,char* exec,char* cmdLine)
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
	cgi_data data;
	data.envString=td->request.URIOPTSPTR?td->request.URIOPTSPTR:td->buffer;
	data.envString+=atoi(td->request.CONTENT_LENGTH);
	
	data.td = td;
	data.errorPage=0;
	strcpy(td->scriptPath,exec);
	MYSERVER_FILE::splitPath(exec,td->scriptDir,td->scriptFile);
	MYSERVER_FILE::splitPath(exec,td->cgiRoot,td->cgiFile);
	buildCGIEnvironmentString(td,data.envString);
	char outFile[MAX_PATH];
	getdefaultwd(outFile,MAX_PATH);
	sprintf(&outFile[strlen(outFile)],"/stdOutFile_%u",td->id);

	data.stdOut.openFile(outFile,MYSERVER_FILE_CREATE_ALWAYS|MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_WRITE);
    hinstLib = LoadLibrary(exec); 
	if (hinstLib) 
    { 
		ProcMain = (CGIMAIN) GetProcAddress(hinstLib, "main"); 
		if(ProcMain)
		{
			(ProcMain)(cmdLine,&data);
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
	if(data.errorPage)
	{
		int errID=getErrorIDfromHTTPStatusCode(data.errorPage);
		if(errID!=-1)
			return raiseHTTPError(td,s,errID);
	}
	data.stdOut.setFilePointer(0);
	/*
	*Compute the response length.
	*/

	sprintf(td->response.CONTENT_LENGTH,"%u",data.stdOut.getFileSize());
	buildHTTPResponseHeader(td->buffer,&td->response);
	s->socket.send(td->buffer,strlen(td->buffer), 0);
	u_long nbr,nbs;
	do
	{
		data.stdOut.readFromFile(td->buffer,td->buffersize,&nbr);
		nbs=s->socket.send(td->buffer,nbr,0);
	}while(nbr && nbs);
	data.stdOut.closeFile();
	MYSERVER_FILE::deleteFile(outFile);
	return 1;
#else
	/*
	*On the platforms that is not available the support for the MSCGI send a 
	*non implemented error.
	*/
	return raiseHTTPError(td,s,e_501);
#endif
}
/*
*Store the MSCGI library module handle.
*/
#ifdef WIN32
static HMODULE mscgiModule=0;
#endif

/*
*Map the library in the address space of the application.
*/
int loadMSCGILib()
{
#ifdef WIN32
	mscgiModule=LoadLibrary("CGI-LIB\\CGI-LIB.dll");
	return (mscgiModule)?1:0;
#else
	return 0;
#endif
}
/*
*Free the memory allocated by the MSCGI library.
*/
int freeMSCGILib()
{
#ifdef WIN32
	/*
	*Return 1 if FreeLibrary returns successfully.
	*/
	return((mscgiModule)?(FreeLibrary(mscgiModule)?1:0):0);
#else
	return 0;
#endif
}
