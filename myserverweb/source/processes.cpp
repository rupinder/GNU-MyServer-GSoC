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
int Process::execHiddenProcess(StartProcInfo *spi,u_long timeout)
{
	int ret=0;
#ifdef NOT_WIN
  u_long count;
#endif
  pid=0;
#ifdef WIN32
    /*!
    *Set the standard output values for the CGI process.
    */
	STARTUPINFO si;
  PROCESS_INFORMATION pi;
	char *cwd;
	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	si.hStdInput = (HANDLE)spi->stdIn;
	si.hStdOutput =(HANDLE)spi->stdOut;
	si.hStdError= (HANDLE)spi->stdError;
	si.dwFlags=STARTF_USESHOWWINDOW;
	if(si.hStdInput||si.hStdOutput||si.hStdError)
		si.dwFlags|=STARTF_USESTDHANDLES;
	si.wShowWindow = SW_HIDE;

  cwd=spi->cwd.length() ? (char*)spi->cwd.c_str() : 0;
	ZeroMemory( &pi, sizeof(pi) );
	ret = CreateProcess(NULL, (char*)spi->cmdLine.c_str(), NULL, NULL, TRUE,0,
                      spi->envString, cwd, &si, &pi);
	if(!ret)
		return (-1);
  pid = pi.hProcess;
	/*!
	*Wait until the process stops its execution.
	*/
	ret=WaitForSingleObject(pi.hProcess, timeout);
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
	pid = fork();
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
		if(spi->cwd.length())
		{
			ret=chdir((const char*)(spi->cwd.c_str()));
			if(ret == -1)
				exit(1);
		}
		// If stdOut is -1, pipe to /dev/null
		if((long)spi->stdOut == -1)
			spi->stdOut = (FileHandle)open("/dev/null", O_WRONLY);
		// map stdio to files
		ret=close(0); // close stdin
		if(ret == -1)
			exit (1);
		ret=dup2(spi->stdIn, 0);
		if(ret == -1)
			exit (1);
		ret=close(spi->stdIn);
		if(ret == -1)
			exit (1);
		ret=close(1); // close stdout
		if(ret == -1)
			exit (1);
		ret=dup2(spi->stdOut, 1);
		if(ret == -1)
			exit (1);
		ret=close(spi->stdOut);
		if(ret == -1)
			exit(1);
		//close(2); // close stderr
		//dup2((int)spi->stdError, 2);
		// Run the script
		if(spi->arg.length())
		{
			ret=execle((const char*)(spi->cmd.c_str()), 
                 (const char*)(spi->cmd.c_str()), 
                 (const char*)(spi->arg.c_str()), NULL, envp);

			if(ret == -1)
				exit(1);
		}
		else
		{
			ret=execle((const char*)(spi->cmd.c_str()), 
                 (const char*)(spi->cmd.c_str()), 
                 NULL, envp);

			if(ret == -1)
				exit(1);	
		}
		exit(1);
	} // end else if(pid == 0)
	// Parent
	// Wait till child dies
  count = 0;
  for( ;  ; count++)
  {
    if(count >= timeout)
    {
      kill(pid, SIGKILL);
      ret = -1;
      break;
    }
    ret = waitpid(pid, NULL, WNOHANG);
    if(ret == -1)
    {
      return (-1);
    }
    else if(ret != 0)
    {
      break;
    }
    wait(1);
  }

	if(ret == -1)
		return (-1);
	return 0;

#endif	

}

/*!
 *Start a process that runs simultaneously with the MyServer process.
 *Return -1 on fails.
 *Return the new process identifier on success.
 */
int Process::execConcurrentProcess(StartProcInfo* spi)
{
	int ret;
	pid=0;
#ifdef WIN32
	/*!
   *Set the standard output values for the CGI process.
   */
	STARTUPINFO si;
  PROCESS_INFORMATION pi;
  char* cwd;   
	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	si.hStdInput = (HANDLE)spi->stdIn;
	si.hStdOutput =(HANDLE)spi->stdOut;
	si.hStdError= (HANDLE)spi->stdError;
	si.dwFlags=(u_long)STARTF_USESHOWWINDOW;
  cwd =(char*)spi->cwd.length() ? (char*)spi->cwd.c_str() : 0;
	if(si.hStdInput||si.hStdOutput||si.hStdError)
		si.dwFlags|=STARTF_USESTDHANDLES;
	si.wShowWindow = SW_HIDE;
	ZeroMemory( &pi, sizeof(pi) );

	ret=CreateProcess(NULL, (char*)spi->cmdLine.c_str(), NULL, NULL, TRUE, 0, 
                    spi->envString, cwd, &si, &pi);
	if(!ret)
		return (-1);	
	pid = (*((int*)&pi.hProcess));
  return pid;
#endif
#ifdef NOT_WIN
	pid = fork();
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
		if(spi->cwd.length())
			chdir((const char*)(spi->cwd.c_str()));
		// If stdOut is -1, pipe to /dev/null
		if((long)spi->stdOut == -1)
			spi->stdOut = (FileHandle)open("/dev/null",O_WRONLY);
		// map stdio to files
		ret = close(0); // close stdin
		if(ret == -1)
			exit (1);
		ret=dup2((long)spi->stdIn, 0);
		if(ret == -1)
			exit (1);	
		ret=close((long)spi->stdIn);
		if(ret == -1)
			exit (1);	
		ret=close(1); // close stdout
		if(ret == -1)
			exit (1);
		ret=dup2((long)spi->stdOut, 1);
		if(ret == -1)
			exit (1);	
		ret=close((long)spi->stdOut);
		if(ret == -1)
			exit (1);	
		//close(2); // close stderr
		//dup2((int)spi->stdError, 2);
		// Run the script
    if(spi->arg.length())
		{
			ret = execle((const char*)(spi->cmd.c_str()), 
                   (const char*)(spi->cmd.c_str()), 
                   (const char*)(spi->arg.c_str()), NULL, envp);
      
			if(ret == -1)
				exit (1);
		}
		else	
		{
			ret=execle((const char*)(spi->cmd.c_str()), 
                 (const char*)(spi->cmd.c_str()), NULL, envp);
			if(ret == -1)
				exit (1);
		}
		exit(1);
	} // end else if(pid == 0)
	return pid;

#endif	

}

/*!
 *Create the object.
 */
Process::Process()
{
  pid = 0;
}

/*!
 *Destroy the object.
 */
Process::~Process()
{

}

/*!
 *Terminate a process.
 *Return 0 on success.
 *Return nonzero on fails.
 */
int Process::terminateProcess()
{
	int ret;
  if(pid == 0)
    return 0;
#ifdef WIN32
	ret = TerminateProcess(*((HANDLE*)&pid),0);
  pid = 0;
	return (!ret);
#endif
#ifdef NOT_WIN
	ret = kill((pid_t)pid, SIGKILL);
  pid = 0;
	return ret;
#endif	
}
