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

#include "../stdafx.h"
#include "../include/utility.h"
#include "../include/sockets.h"

extern "C" {
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif
}

#ifdef WIN32
#include <direct.h>
#endif

/*
*Various utility functions.
*/
extern int mustEndServer; 
static char currentPath[MAX_PATH];

/*
*Returns the version of the operating system.
*/
int ms_getOSVersion()
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
#ifdef __linux__
        ret = OS_LINUX;
#endif
	return ret;
}	

/*
*Returns the number of processors available on the local machine.
*/
u_long ms_getCPUCount()
{
	u_long ret=1;
#ifdef WIN32
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	ret=si.dwNumberOfProcessors;
#endif
	return ret;
}
/*
*Execute an hidden process and ms_wait until it ends itself or its execution
*time is greater than the timeout value.
*Returns the process exit code.
*/
u_long execHiddenProcess(START_PROC_INFO *spi,u_long timeout)
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
    CreateProcess(NULL, spi->cmdLine, NULL, NULL, TRUE,CREATE_NEW_CONSOLE,spi->envString,spi->cwd,&si, &pi);
	/*
	*Wait until it's ending by itself.
	*/
	WaitForSingleObject(pi.hProcess,timeout);
	u_long exitCode;
	GetExitCodeProcess(pi.hProcess,&exitCode);
	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );
	return exitCode;
#else
	int pid = fork();
	if(pid < 0) // a bad thing happend
		return 0;
	else if(pid == 0) // child
	{	
		// Set env vars
		int i = 0;
		int index = 0;
		char * envp[100];
	
		while(*((char *)(spi->envString) + i) != '\0')
		{
			envp[index] = ((char *)(spi->envString) + i);
			index++;
			
			while(*((char *)(spi->envString) + i) != '\0')
				i++;
			i++;
		}
		envp[index] = NULL;
		// change to working dir
		chdir((const char*)(spi->cwd));
		// map stdio to files
		close(0); // close stdin
		dup2((int)spi->stdIn, 0);
		close(1); // close stdout
		dup2((int)spi->stdOut, 1);
		//close(2); // close stderr
		//dup2((int)spi->stdError, 2);
		// Run the script
		execle((const char*)(spi->cmd), (const char*)(spi->cmd), (const char*)(spi->arg), NULL, envp);
		// never should be here
		exit(1);
	} // end else if(pid == 0)
	// Parent
	// Wait till child dies
	waitpid(pid, NULL, 0);
	return 0;
#endif	
}

/*
*This function is similar to the Windows API WaitForSingleObject(..)
*/
int ms_requestAccess(u_long* ac,u_long id)
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
		ms_requestAccess(ac,id);
		return 0;
	}
	/*
	*Wait until another thread ends the access, then set our access.
	*/
	while(*ac!=id);
	*ac=id;
	ms_requestAccess(ac,id);
	return 0;
}
int ms_terminateAccess(u_long* ac,u_long/* id*/)
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
	for(u_long i=0;i<(u_long)lstrlen(currentPath);i++)
		if(currentPath[i]=='\\')
			currentPath[i]='/';
	if(currentPath[lstrlen(currentPath)]=='/')
		currentPath[lstrlen(currentPath)]='\0';
#else
	getcwd(currentPath,MAX_PATH);
	retval=1;
	if(currentPath[strlen(currentPath)]=='/')
		currentPath[strlen(currentPath)]='\0';
#endif
	return retval;
}
/*
*Get the default working directory(Where is the program myserver.exe).
*/
char *ms_getdefaultwd(char *path,int len)
{
	if(path)
#ifdef WIN32
		lstrcpyn(path,currentPath,len);
#else
		strncpy(path,currentPath,len);
#endif
	return path;
}
/*
*Get the local machine name.
*/
void ms_getComputerName(char *dest,u_long maxLen)
{
	ms_gethostname(dest,maxLen);
}

/*
*Set the current working directory. Returns Zero if successful.
*/
int ms_setcwd(char *dir)
{
#ifdef WIN32	
	return _chdir(dir);
#else
	return chdir(dir);
#endif
}
/*
*Sleep the caller thread.
*/
void ms_wait(u_long time)
{
#ifdef WIN32
		Sleep(time);
#else
	    usleep(time);
#endif

}
/*
*Set the text color to red on black.
*/
void preparePrintError()
{
#ifdef WIN32
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_RED|FOREGROUND_INTENSITY);
#endif
}
/*
**Set the text color to white on black.
*/
void endPrintError()
{
#ifdef WIN32
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
#endif
}