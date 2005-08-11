/*
MyServer
Copyright (C) 2002, 2003, 2004 The MyServer Team
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "../stdafx.h"
#include "../include/utility.h"
#include "../include/sockets.h"

extern "C" {
#include <errno.h>
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
  pid = (u_long)pi.hProcess;
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
		const char *envp[100];
    const char *args[100];

    /*! Build the args vector. */
    args[0]=spi->cmd.c_str();
    {
      int count=1;
      int len=spi->cmd.length();
      for(int i=1; i<len; i++)
        if(spi->cmd[i]==' ' && spi->cmd[i-1]!='\\')
        {
          if(count < 99)
          {
            if((const char*)&(spi->cmd.c_str())[i+1])
              args[count++]=(const char*)&(spi->cmd.c_str())[i+1];
            spi->cmd[i]='\0';
          }
          else
            break;
        }
      args[count++]=spi->arg.c_str();
      len=spi->arg.length();
      for(int i=1; i<len; i++)
        if(spi->arg[i]==' ' && spi->arg[i-1]!='\\')
        {
          if(count < 100)
          {
            args[count++]=(const char*)&(spi->arg.c_str())[i+1];
            spi->arg[i]='\0';
          }
          else
            break;
        }
      args[count]=NULL;
    }

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
    ret=execve((const char*)(spi->cmd.c_str()),
               (char* const*)args,(char* const*) envp);
    printf("\r\n\r\n%i\n", ret);
    exit(0);

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
      if(errno == ECHILD)
        return 0;
      else
        return (-1);
    }
    else if(ret != 0)
    {
      break;
    }
    Thread::wait(1);
  }

	if(ret == -1)
		return (-1);
	return 0;

#endif	

}

/*!
 *Return a nonzero value if the process is still alive. A return value of zero
 *means the process is a zombie.
 */
int Process::isProcessAlive()
{
  if(pid == 0)
    return 0;
#ifdef WIN32
  u_long ec;
  int ret = GetExitCodeProcess(*((HANDLE*)&pid), &ec);
  if(ret == 0)
    return 0;
  if(ec == STILL_ACTIVE)
    return 1;
  return 0; 
#endif

#ifdef NOT_WIN

#ifdef PROCESS_ALIVE_USEWAITPID
  int status = 0;
  int ret;
  do
  {
    ret = waitpid(pid, &status, WNOHANG);
  }
  while(!ret && errno==EINTR);
  if(!ret)
    return 1;
  return 0;
#else
  /*! Send the pseudo-signal 0 to check if the process is alive. */
  int ret;
  do
  {
    ret=kill(pid, 0);
  }
  while(!ret && errno==EINTR);
  if(ret == 0)
    return 1;
  return 0;
#endif


#endif
  return 0;
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
		const char *envp[100];
    const char *args[100];

   /*! Build the args vector. */
    args[0]=spi->cmd.c_str();
    {
      int count=1;
      int len=spi->cmd.length();
      for(int i=1; i<len; i++)
        if(spi->cmd[i]==' ' && spi->cmd[i-1]!='\\')
        {
          if(count < 99)
          {
            if((const char*)&(spi->cmd.c_str())[i+1])
              args[count++]=(const char*)&(spi->cmd.c_str())[i+1];
            spi->cmd[i]='\0';
          }
          else
            break;
        }
      args[count++]=spi->arg.c_str();
      len=spi->arg.length();
      for(int i=1; i<len; i++)
        if(spi->arg[i]==' ' && spi->arg[i-1]!='\\')
        {
          if(count < 100)
          {
            args[count++]=(const char*)&(spi->arg.c_str())[i+1];
            spi->arg[i]='\0';
          }
          else
            break;
        }
      args[count]=NULL;
    }
		
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

    ret = execve((const char*)(spi->cmd.c_str()), 
                 (char* const*)args,(char* const*) envp);
    
    
		exit(1);
	} // end else if(pid == 0)
 else
 {
    /*! 
     *Avoid to become a zombie process. This is needed by the
     *Process::isProcessAlive routine.
     */
   struct sigaction sa;
   memset(&sa, 0, sizeof(sa));
   sa.sa_handler = SIG_IGN;
   sa.sa_flags   = SA_RESTART;
   sigaction(SIGCHLD,&sa, (struct sigaction *)NULL);
	return pid;
}
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

/*!
 *Set the user identity for the process. Returns 0 on success.
 */
int Process::setuid(u_long uid)
{
#ifdef NOT_WIN
  return ::setuid(uid);
#endif
  return 0;
}

/*!
 *Set the group identity for the process. Returns 0 on success.
 */
int Process::setgid(u_long gid)
{
#ifdef NOT_WIN
  return ::setgid(gid);
#endif
  return 0;
}
