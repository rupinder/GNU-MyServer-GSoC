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
#include "../include/cgi.h"
#include "../include/AMMimeUtils.h"
#include "../include/filemanager.h"
#include "../include/sockets.h"
#include "../include/utility.h"

extern "C" {
#ifdef WIN32
#include <direct.h>
#else
#include <dlfcn.h>
#define HMODULE void *
#endif
}

/*!
*Sends the MyServer CGI; differently from standard CGI this don't need a new process to run
*so it is faster.
*/
int mscgi::sendMSCGI(httpThreadContext* td,LPCONNECTION s,char* exec,char* cmdLine)
{
	/*!
	*This is the code for manage a .mscgi file.
	*This files differently from standard CGI don't need a new process to run
	*but are allocated in the caller process virtual space.
	*Usually these files are faster than standard CGI.
	*Actually myServerCGI(.mscgi) is only at an alpha status.
	*/

	static HMODULE hinstLib; 
    static CGIMAIN ProcMain;
	cgi_data data;
	data.envString=td->request.URIOPTSPTR?td->request.URIOPTSPTR:td->buffer;
	data.envString+=atoi(td->request.CONTENT_LENGTH);
	
	data.td = td;
	data.errorPage=0;
	strncpy(td->scriptPath,exec,MAX_PATH);
	MYSERVER_FILE::splitPath(exec,td->scriptDir,td->scriptFile);
	MYSERVER_FILE::splitPath(exec,td->cgiRoot,td->cgiFile);
	cgi::buildCGIEnvironmentString(td,data.envString);
	char outFile[MAX_PATH];
	getdefaultwd(outFile,MAX_PATH);
	sprintf(&outFile[strlen(outFile)],"/stdOutFile_%u",td->id);

	data.stdOut.openFile(outFile,MYSERVER_FILE_CREATE_ALWAYS|MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_WRITE);
#ifdef WIN32
	hinstLib = LoadLibrary(exec);
#else
	hinstLib = dlopen(exec, RTLD_LAZY);
#endif
	if (hinstLib) 
    { 
		/*
		*Set the working directory to the MSCGI file one.
		*/
		setcwd(td->scriptDir);
#ifdef WIN32
		ProcMain = (CGIMAIN) GetProcAddress(hinstLib, "main"); 
#else
		ProcMain = (CGIMAIN) dlsym(hinstLib, "main");
#endif
		if(ProcMain)
		{
			(ProcMain)(cmdLine,&data);
		}
#ifdef WIN32
		FreeLibrary(hinstLib); 
#else
		dlclose(hinstLib);
#endif
		/*
		*Restore the working directory.
		*/
		setcwd(getdefaultwd(0,0));		
    } 
	else
	{
#ifdef WIN32
		if(GetLastError()==ERROR_ACCESS_DENIED)
		{
			if(s->nTries > 2)
			{
				return ((http*)td->lhttp)->raiseHTTPError(td,s,e_403);
			}
			else
			{
				s->nTries++;
				return ((http*)td->lhttp)->raiseHTTPError(td,s,e_401AUTH);
			}
		}
		else
		{
#endif
			return  ((http*)td->lhttp)->raiseHTTPError(td,s,e_404);
#ifdef WIN32
		}
#endif
	}
	if(data.errorPage)
	{
		int errID=getErrorIDfromHTTPStatusCode(data.errorPage);
		if(errID!=-1)
			return ((http*)td->lhttp)->raiseHTTPError(td,s,errID);
	}
	data.stdOut.setFilePointer(0);
	/*!
	*Compute the response length.
	*/
	sprintf(td->response.CONTENT_LENGTH,"%u",data.stdOut.getFileSize());
	
	http_headers::buildHTTPResponseHeader(td->buffer,&td->response);
	s->socket.send(td->buffer,(int)strlen(td->buffer), 0);
	u_long nbr,nbs;
	do
	{
		data.stdOut.readFromFile(td->buffer,td->buffersize,&nbr);
		nbs=s->socket.send(td->buffer,nbr,0);
	}while(nbr && nbs);
	data.stdOut.closeFile();
	MYSERVER_FILE::deleteFile(outFile);
	return 1;
	/*!
	*On the platforms that is not available the support for the MSCGI send a 
	*non implemented error.
	*/
	return ((http*)td->lhttp)->raiseHTTPError(td,s,e_501);
}
/*!
*Store the MSCGI library module handle.
*/
static HMODULE mscgiModule=0;

/*!
*Map the library in the application address space.
*/
int mscgi::loadMSCGILib()
{
#ifdef WIN32
	mscgiModule=LoadLibrary("CGI-LIB\\CGI-LIB.dll");
#else
	mscgiModule=dlopen("cgi-lib/cgi-lib.so", RTLD_NOW | RTLD_GLOBAL);
#endif
	return (mscgiModule)?1:0;
}
/*!
*Free the memory allocated by the MSCGI library.
*/
int mscgi::freeMSCGILib()
{
#ifdef WIN32
	/*!
	*Return 1 if FreeLibrary returns successfully.
	*/
	return((mscgiModule)?(FreeLibrary(mscgiModule)?1:0):0);
#else
	return((mscgiModule)?(dlclose(mscgiModule)?1:0):0);
#endif
}
