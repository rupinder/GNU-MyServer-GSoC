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

#include "..\stdafx.h"
#include "..\include\utility.h"
#include <string.h>
#include <direct.h>
#include <stdlib.h>
#include <stdio.h>

/*
*Various utility functions.
*/
extern BOOL mustEndServer; 
static char currentPath[MAX_PATH];

/*
*Returns the version of the operating system.
*/
INT getOSVersion()
{
	int ret=0;
	/*
	*This is the code for the win32 platform.
	*/
#ifdef WIN32
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize=sizeof(osvi);
	GetVersionEx(&osvi);
	switch(osvi.dwMinorVersion)
	{
	case 0:
		if(osvi.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS)
			ret=OS_WINDOWS_9X;
		else
			ret=OS_WINDOWS_2000;
		break;
	case 10:
		ret=OS_WINDOWS_9X;
		break;	
	case 90:
		ret=OS_WINDOWS_9X;
		break;
	case 51:
		ret=OS_WINDOWS_NT3;
		break;
	case 1:
		ret=OS_WINDOWS_XP;
		break;
	}
#endif
	return ret;
}	

/*
*Returns the number of processors that are on the local machine.
*/
DWORD getCPUCount()
{
	DWORD ret=1;
#ifdef WIN32
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	ret=si.dwNumberOfProcessors;
#endif
	return ret;
}
/*
*Execute an hidden process and wait until it ends itself or its execution
*time is greater than the timeout value.
*Returns 1 if the process ended itself or returns 0 if the process was
*closed by the server
*/
DWORD execHiddenProcess(START_PROC_INFO *spi,DWORD timeout)
{
#ifdef WIN32
    /*
    *Set the standard output values for the CGI process.
    */
    STARTUPINFO si;
	
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    si.hStdInput = (HANDLE)spi->stdIn;
    si.hStdOutput =(HANDLE)spi->stdOut;
    si.hStdError= (HANDLE)spi->stdError;
    si.dwFlags=STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi;
    ZeroMemory( &pi, sizeof(pi) );
    CreateProcess(NULL, spi->cmdLine, NULL, NULL, TRUE,CREATE_NEW_CONSOLE,spi->envString,NULL,&si, &pi);
	/*
	*Wait until it's ending by itself.
	*/
	DWORD ret=WaitForSingleObject(pi.hProcess,timeout);
	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );
	return (ret==WAIT_TIMEOUT)?0:1;
#endif
}

/*
*Get the local machine name.
*/
VOID getComputerName(char *dest,DWORD maxLen)
{
#ifdef WIN32
	/*
	*If we report error using the GetComputerName function 
	*then we use the default name "localhost".
	*/
	if(GetComputerNameA(dest,&maxLen)==0)
	{
		lstrcpy(dest,"localhost");
	}
#endif
}


/*
*This function is similar to the Windows API WaitForSingleObject(..)
*/
INT requestAccess(DWORD* ac,DWORD id)
{
	/*
	*If the access ID is equal to the thread ID we don't do nothing.
	*/
	if(*ac==id)
		return 0;
	/*
	*if the access doesn't belong to any thread then set that it belongs to the caller thread
	*and check if we have the access now.
	*/
	if(*ac==0)
	{
		*ac=id;
		requestAccess(ac,id);
		return 0;
	}
	/*
	*Wait until another thread ends the access, then set our access.
	*/
	while(*ac!=id);
	*ac=id;
	requestAccess(ac,id);
	return 0;
}
INT terminateAccess(DWORD* ac,DWORD/* id*/)
{
	/*
	*Only set to Zero the owner of the access.
	*/
	*ac=0;
	return 0;
}
/*
*Save the current working directory.
*/
int ms_setcwdBuffer()
{
	int retval=0;
#ifdef WIN32	
	_getcwd(currentPath,MAX_PATH);
	retval=1;
#endif
	for(DWORD i=0;i<(DWORD)lstrlen(currentPath);i++)
		if(currentPath[i]=='\\')
			currentPath[i]='/';
	if(currentPath[lstrlen(currentPath)]=='/')
		currentPath[lstrlen(currentPath)]='\0';
	return retval;
}
/*
*Get the default working directory(Where is the program myserver.exe).
*/
char *ms_getdefaultwd(char *path,int/*len*/)
{
	if(path)
		lstrcpy(path,currentPath);
	return path;
}

/*
*Set the current working directory. Returns Zero if successful.
*/
int ms_setcwd(char *dir)
{
#ifdef WIN32	
	return _chdir(dir);
#endif

}