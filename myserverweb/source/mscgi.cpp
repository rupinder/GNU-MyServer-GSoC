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

#include "../include/http.h"
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
}

/*
*Sends the myServer CGI; differently form standard CGI this don't need a new process to run
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
	static CGIINIT ProcInit;
 
    hinstLib = LoadLibrary(exec); 
	td->buffer2[0]='\0';
	if (hinstLib) 
    { 
		ProcInit = (CGIINIT) GetProcAddress(hinstLib, "initialize");
		ProcMain = (CGIMAIN) GetProcAddress(hinstLib, "main"); 
		if(ProcInit && ProcMain)
		{
			(ProcInit)(td,s);
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
	/*
	*Compute the response lenght.
	*/
	static int len;
	len=lstrlen(td->buffer2);
	sprintf(td->response.CONTENTS_DIM,"%u",len);
	buildHTTPResponseHeader(td->buffer,&td->response);
	ms_send(s->socket,td->buffer,lstrlen(td->buffer), 0);
	ms_send(s->socket,td->buffer2,len, 0);
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
#endif
	return 0;
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
#endif
	return 0;
}
