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


#include "../stdafx.h"
#include "../include/utility.h"
#include "../include/sockets.h"

extern "C" {
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef WIN32
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif
}

#ifdef WIN32
#include <direct.h>
#endif

/*!
*Various utility functions.
*/
extern int mustEndServer; 
static char currentPath[MAX_PATH];

/*!
*Returns the version of the operating system.
*/
int getOSVersion()
{
	int ret=0;
	/*!
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
#else
#ifdef __linux__
        ret = OS_LINUX;
#else
	ret = 0;
#endif
#endif
	return ret;
}	

/*!
*Returns the number of processors available on the local machine.
*/
u_long getCPUCount()
{
	u_long ret=1;
#ifdef WIN32
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	ret=si.dwNumberOfProcessors;
#endif
#ifdef NOT_WIN
	ret=sysconf(_SC_NPROCESSORS_CONF); 
#endif
	return ret;
}
/*!
*Execute an hidden process and wait until it ends itself or its execution
*time is greater than the timeout value.
*Returns the process exit code.
*/
u_long execHiddenProcess(START_PROC_INFO *spi,u_long timeout)
{
#ifdef WIN32
    /*!
    *Set the standard output values for the CGI process.
    */
    STARTUPINFO si;
	
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    si.hStdInput = (HANDLE)spi->stdIn;
    si.hStdOutput =(HANDLE)spi->stdOut;
    si.hStdError= (HANDLE)spi->stdError;
    si.dwFlags=STARTF_USESHOWWINDOW;
	if(si.hStdInput||si.hStdOutput||si.hStdError)
		si.dwFlags|=STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi;
    ZeroMemory( &pi, sizeof(pi) );
    CreateProcess(NULL, spi->cmdLine,NULL, NULL, TRUE,0,spi->envString,spi->cwd,&si, &pi);
	/*!
	*Wait until it's ending by itself.
	*/
	WaitForSingleObject(pi.hProcess,timeout);
	u_long exitCode;
	GetExitCodeProcess(pi.hProcess,&exitCode);
	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );
	return exitCode;
#endif
#ifdef NOT_WIN
	int pid = fork();
	if(pid < 0) // a bad thing happend
		return 0;
	else if(pid == 0) // child
	{	
		// Set env vars
		int i = 0;
		int index = 0;
		char * envp[100];
	
		if(spi->envString != NULL)
		{
			while(*((char *)(spi->envString) + i) != '\0')
			{
				envp[index] = ((char *)(spi->envString) + i);
				index++;
				
				while(*((char *)(spi->envString) + i) != '\0')
					i++;
				i++;
			}
			envp[index] = NULL;
		}
		else
			envp[0] = NULL;
		
		// change to working dir
		if(spi->cwd)
			chdir((const char*)(spi->cwd));
		// If stdOut is -1, pipe to /dev/null
		if((int)spi->stdOut == -1)
			spi->stdOut = (MYSERVER_FILE_HANDLE)open("/dev/null",O_WRONLY);
		// map stdio to files
		close(0); // close stdin
		dup2((int)spi->stdIn, 0);
		close((int)spi->stdIn);
		close(1); // close stdout
		dup2((int)spi->stdOut, 1);
		close((int)spi->stdOut);
		//close(2); // close stderr
		//dup2((int)spi->stdError, 2);
		// Run the script
		if(spi->arg != NULL)
			execle((const char*)(spi->cmd), (const char*)(spi->cmd), (const char*)(spi->arg), NULL, envp);
		else
			execle((const char*)(spi->cmd), (const char*)(spi->cmd), NULL, envp);
		exit(1);
	} // end else if(pid == 0)
	// Parent
	// Wait till child dies
	waitpid(pid, NULL, 0);
	return 0;

#endif	

}
/*!
*Start a process runned simultaneously with the MyServer process.
*/
u_long execConcurrentProcess(START_PROC_INFO* spi)
{
#ifdef WIN32
    /*!
    *Set the standard output values for the CGI process.
    */
    STARTUPINFO si;
	
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    si.hStdInput = (HANDLE)spi->stdIn;
    si.hStdOutput =(HANDLE)spi->stdOut;
    si.hStdError= (HANDLE)spi->stdError;
    si.dwFlags=(u_long)STARTF_USESHOWWINDOW;
	if(si.hStdInput||si.hStdOutput||si.hStdError)
		si.dwFlags|=STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi;
    ZeroMemory( &pi, sizeof(pi) );
    CreateProcess(NULL, spi->cmdLine, NULL, NULL, TRUE,0,spi->envString,spi->cwd,&si, &pi);
	return (*((u_long*)&pi.hProcess));
#endif
#ifdef NOT_WIN
	int pid = fork();
	if(pid < 0) // a bad thing happend
		return 0;
	else if(pid == 0) // child
	{	
		// Set env vars
		int i = 0;
		int index = 0;
		char * envp[100];
		
		if(spi->envString != NULL)
		{
			while(*((char *)(spi->envString) + i) != '\0')
			{
				envp[index] = ((char *)(spi->envString) + i);
				index++;
				
				while(*((char *)(spi->envString) + i) != '\0')
					i++;
				i++;
			}
			envp[index] = NULL;
		}
		else
			envp[0] = NULL;
		
		// change to working dir
		if(spi->cwd)
			chdir((const char*)(spi->cwd));
		// If stdOut is -1, pipe to /dev/null
		if((int)spi->stdOut == -1)
			spi->stdOut = (MYSERVER_FILE_HANDLE)open("/dev/null",O_WRONLY);
		// map stdio to files
		close(0); // close stdin
		dup2((int)spi->stdIn, 0);
		close((int)spi->stdIn);
		close(1); // close stdout
		dup2((int)spi->stdOut, 1);
		close((int)spi->stdOut);
		//close(2); // close stderr
		//dup2((int)spi->stdError, 2);
		// Run the script
		if(spi->arg != NULL)
			execle((const char*)(spi->cmd), (const char*)(spi->cmd), (const char*)(spi->arg), NULL, envp);
		else	
			execle((const char*)(spi->cmd), (const char*)(spi->cmd), NULL, envp);
		exit(1);
	} // end else if(pid == 0)
	return (u_long)pid;

#endif	

}
/*!
*Terminate a process.
*/
int terminateProcess(u_long id)
{
#ifdef WIN32
	return TerminateProcess(*((HANDLE*)&id),0);
#endif
#ifdef NOT_WIN
	/*!
	*id is the process id
	*/
	return kill((pid_t)id, SIGTERM);
#endif	
}


/*!
*This function is similar to the Windows API WaitForSingleObject(..)
*/
int requestAccess(u_long* ac,u_long id)
{
	/*!
	*If the access ID is equal to the thread ID we don't do nothing.
	*/
	if(*ac==id)
		return 0;
	/*!
	*if the access doesn't belong to any thread then set that it belongs to the caller thread
	*and check if we have the access now.
	*/
	do
	{
		if(*ac==0)
		{
			*ac=id;
			requestAccess(ac,id);
			return 0;
		}
	}
	/*!
	*Wait until another thread ends the access, then set our access.
	*/
	while(*ac);


	*ac=id;
	requestAccess(ac,id);
	return 0;
}
/*!
*Reset the owner of the access.
*/
int terminateAccess(u_long* ac,u_long/*! id*/)
{
	/*!
	*Only set to Zero the owner of the access.
	*/
	*ac=0;
	return 0;
}
/*!
*Save the current working directory.
*/
int setcwdBuffer()
{
	int retval=0;
#ifdef WIN32	
	_getcwd(currentPath,MAX_PATH);
	retval=1;
	for(u_long i=0;i<(u_long)strlen(currentPath);i++)
		if(currentPath[i]=='\\')
			currentPath[i]='/';
	if(currentPath[strlen(currentPath)]=='/')
		currentPath[strlen(currentPath)]='\0';
#endif
#ifdef NOT_WIN
	getcwd(currentPath,MAX_PATH);
	retval=1;
	if(currentPath[strlen(currentPath)]=='/') 
		currentPath[strlen(currentPath)]='\0';
#endif
	return retval;
}
/*!
*Get the default working directory(Where is the main executable).
*/
char *getdefaultwd(char *path,int len)
{
	if(path)
#ifdef WIN32
		lstrcpyn(path,currentPath,len);
#endif
#ifdef NOT_WIN
		strncpy(path,currentPath,len);
#endif
	return currentPath;
}


/*!
*Set the current working directory. Returns Zero if successful.
*/
int setcwd(char *dir)
{
#ifdef WIN32	
	return _chdir(dir);
#endif
#ifdef NOT_WIN
	return chdir(dir);
#endif
}
/*!
*Sleep the caller thread.
*/
void wait(u_long time)
{
#ifdef WIN32
	Sleep(time);
#endif
#ifdef NOT_WIN
	usleep(time);
#endif

}
/*!
*Set the text color to red on black.
*/
void preparePrintError()
{
#ifdef WIN32
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_RED|FOREGROUND_INTENSITY);
#endif
}
/*!
**Set the text color to white on black.
*/
void endPrintError()
{
#ifdef WIN32
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
#endif

}
