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

extern int mustEndServer; 

/*!
*Execute an hidden process and wait until it ends itself or its execution
*time is greater than the timeout value.
*Return -1 on fails.
*Return 0 on success.
*/
u_long execHiddenProcess(START_PROC_INFO *spi,u_long timeout)
{
	int ret;
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
	ret = CreateProcess(NULL, spi->cmdLine,NULL, NULL, TRUE,0,
                      spi->envString,spi->cwd,&si, &pi);
	if(!ret)
		return (u_long)(-1);
	/*!
	*Wait until it's ending by itself.
	*/
	ret=WaitForSingleObject(pi.hProcess,timeout);
	if(ret == WAIT_FAILED)
		return (u_long)(-1);
	ret=CloseHandle( pi.hProcess );
	if(!ret)
		return (u_long)(-1);
	ret=CloseHandle( pi.hThread );
	if(!ret)
		return (u_long)(-1);	
	return 0;
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
				if(index + 1 == 100  )
				{
					break;
				}
				
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
		{
			ret=chdir((const char*)(spi->cwd));
			if(ret == -1)
				return (u_long)(-1);
		}
		// If stdOut is -1, pipe to /dev/null
		if((int)spi->stdOut == -1)
			spi->stdOut = (MYSERVER_FILE_HANDLE)open("/dev/null", O_WRONLY);
		// map stdio to files
		ret=close(0); // close stdin
		if(ret == -1)
			return (u_long)(-1);
		ret=dup2((int)spi->stdIn, 0);
		if(ret == -1)
			return (u_long)(-1);
		ret=close((int)spi->stdIn);
		if(ret == -1)
			return (u_long)(-1);
		ret=close(1); // close stdout
		if(ret == -1)
			return (u_long)(-1);
		ret=dup2((int)spi->stdOut, 1);
		if(ret == -1)
			return (u_long)(-1);
		ret=close((int)spi->stdOut);
		if(ret == -1)
			return (u_long)(-1);
		//close(2); // close stderr
		//dup2((int)spi->stdError, 2);
		// Run the script
		if(spi->arg != NULL)
		{
			ret=execle((const char*)(spi->cmd), (const char*)(spi->cmd), 
                 (const char*)(spi->arg), NULL, envp);
			if(ret == -1)
				return (u_long)(-1);
		}
		else
		{
			ret=execle((const char*)(spi->cmd), (const char*)(spi->cmd), 
                 NULL, envp);
			if(ret == -1)
				return (u_long)(-1);	
		}
		exit(1);
	} // end else if(pid == 0)
	// Parent
	// Wait till child dies
	ret = waitpid(pid, NULL, 0);
	if(ret == -1)
		return (u_long)(-1);
	return 0;

#endif	

}

/*!
*Start a process runned simultaneously with the MyServer process.
*Return -1 on fails.
*Return the new process handle on success.
*/
u_long execConcurrentProcess(START_PROC_INFO* spi)
{
	int ret;
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
	
	ret=CreateProcess(NULL, spi->cmdLine, NULL, NULL, TRUE,0,spi->envString,spi->cwd,&si, &pi);
	if(!ret)
		return (u_long)(-1);	
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
		ret = close(0); // close stdin
		if(ret == -1)
			return (u_long)(-1);
		ret=dup2((int)spi->stdIn, 0);
		if(ret == -1)
			return (u_long)(-1);	
		ret=close((int)spi->stdIn);
		if(ret == -1)
			return (u_long)(-1);	
		ret=close(1); // close stdout
		if(ret == -1)
			return (u_long)(-1);
		ret=dup2((int)spi->stdOut, 1);
		if(ret == -1)
			return (u_long)(-1);	
		ret=close((int)spi->stdOut);
		if(ret == -1)
			return (u_long)(-1);	
		//close(2); // close stderr
		//dup2((int)spi->stdError, 2);
		// Run the script
		if(spi->arg != NULL)
		{
			ret = execle((const char*)(spi->cmd), (const char*)(spi->cmd), (const char*)(spi->arg), NULL, envp);
			if(ret == -1)
				return (u_long)(-1);
		}
		else	
		{
			ret=execle((const char*)(spi->cmd), (const char*)(spi->cmd), NULL, envp);
			if(ret == -1)
				return (u_long)(-1);
		}
		exit(1);
	} // end else if(pid == 0)
	return (u_long)pid;

#endif	

}

/*!
*Terminate a process.
*Return 0 on success.
*Return nonzero on fails.
*/
int terminateProcess(u_long id)
{
	int ret;
#ifdef WIN32
	ret = TerminateProcess(*((HANDLE*)&id),0);
	return (!ret);
#endif
#ifdef NOT_WIN
	/*!
	*id is the process id
	*/
	ret = kill((pid_t)id, SIGTERM);
	return ret;
#endif	
}
