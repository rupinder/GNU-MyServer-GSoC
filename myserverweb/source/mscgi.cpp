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
#ifdef HAVE_DL
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
#ifndef DO_NOT_USE_MSCGI
	static HMODULE hinstLib; 
   	static CGIMAIN ProcMain;
	u_long nbr=0,nbs=0;
	cgi_data data;
	data.envString=td->request.URIOPTSPTR?td->request.URIOPTSPTR:(char*)td->buffer->GetBuffer();
	
	data.td = td;
	data.errorPage=0;
	strncpy(td->scriptPath,exec,MAX_PATH);
	MYSERVER_FILE::splitPath(exec,td->scriptDir,td->scriptFile);
	MYSERVER_FILE::splitPath(exec,td->cgiRoot,td->cgiFile);
	cgi::buildCGIEnvironmentString(td,data.envString);
	
	char outDataPath[MAX_PATH];
	
	if(!td->appendOutputs)
	{	
		getdefaultwd(outDataPath,MAX_PATH);	
		sprintf(&(outDataPath)[strlen(outDataPath)],"/stdOutFileMSCGI_%u",(u_int)td->id);
		data.stdOut.openFile(outDataPath,MYSERVER_FILE_CREATE_ALWAYS|MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_WRITE);
	}
	else
	{
		data.stdOut.setHandle(td->outputData.getHandle());
	}

#ifdef WIN32
	hinstLib = LoadLibrary(exec);
#endif
#ifdef HAVE_DL
	hinstLib = dlopen(exec, RTLD_LAZY);
#endif
	if (hinstLib) 
	{ 
		/*
		*Set the working directory to the MSCGI file one.
		*/
		setcwd(td->scriptDir);
		td->buffer2->GetAt(0)='\0';
#ifdef WIN32
		ProcMain = (CGIMAIN) GetProcAddress((HMODULE)hinstLib, "main"); 
#endif
#ifdef HAVE_DL
		ProcMain = (CGIMAIN) dlsym(hinstLib, "main");
#endif
		if(ProcMain)
		{
			(ProcMain)(cmdLine,&data);
		}
#ifdef WIN32
		FreeLibrary((HMODULE)hinstLib); 
#endif
#ifdef HAVE_DL
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
				data.stdOut.closeFile();
				MYSERVER_FILE::deleteFile(outDataPath);
				return ((http*)td->lhttp)->raiseHTTPError(td,s,e_403);
			}
			else
			{
				s->nTries++;
				if(!td->appendOutputs)
				{
					data.stdOut.closeFile();
					MYSERVER_FILE::deleteFile(outDataPath);
				}
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
		if(!td->appendOutputs)
		{
			data.stdOut.closeFile();
			MYSERVER_FILE::deleteFile(outDataPath);
		}
		int errID=getErrorIDfromHTTPStatusCode(data.errorPage);
		if(errID!=-1)
			return ((http*)td->lhttp)->raiseHTTPError(td,s,errID);
	}
	/*!
	*Compute the response length.
	*/
	sprintf(td->response.CONTENT_LENGTH,"%u",(u_int)data.stdOut.getFileSize());
	/*Send all the data to the client if we haven't to append the output*/
	if(!td->appendOutputs)
	{
		char *buffer = (char*)td->buffer2->GetBuffer();
		u_long bufferSize= td->buffer2->GetRealLength();
		data.stdOut.setFilePointer(0);
		http_headers::buildHTTPResponseHeader(buffer,&(td->response));
		if(s->socket.send(buffer,(int)strlen(buffer), 0)==SOCKET_ERROR)
		{
			if(!td->appendOutputs)
			{
				data.stdOut.closeFile();
				MYSERVER_FILE::deleteFile(outDataPath);
			}
			return 0;
		}
		do
		{
			data.stdOut.readFromFile(buffer,bufferSize,&nbr);
			if(nbr)
			{
				nbs=(u_long)s->socket.send(buffer,nbr,0);
				if(nbs==(u_long)SOCKET_ERROR)
				{
					if(!td->appendOutputs)
					{
						data.stdOut.closeFile();
						MYSERVER_FILE::deleteFile(outDataPath);
					}
					return 0;
				}		
			}
		}while(nbr && nbs);
		if(!td->appendOutputs)
		{
			data.stdOut.closeFile();
			MYSERVER_FILE::deleteFile(outDataPath);
		}
	}
	return 1;
#else
	/*!
	*On the platforms where is not available the MSCGI support send a 
	*non implemented error.
	*/
	return ((http*)td->lhttp)->raiseHTTPError(td,s,e_501);
#endif
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
#endif
#ifdef HAVE_DL
	char mscgi_path;
	
	if(MYSERVER_FILE::fileExists("cgi-lib/cgi-lib.so"))
	{
		strcpy(mscgi_path, "cgi-lib/cgi-lib.so");
	}
	else
	{
		strcpy(mscgi_path, "/usr/lib/myserver/cgi-lib.so");
	}
	mscgiModule=dlopen(mscgi_path, RTLD_NOW | RTLD_GLOBAL);
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
	return((mscgiModule)?(FreeLibrary((HMODULE)mscgiModule)?1:0):0);
#else
#ifdef HAVE_DL
	return((mscgiModule)?(dlclose(mscgiModule)?1:0):0);
#else
	return 1;
#endif
#endif
}
