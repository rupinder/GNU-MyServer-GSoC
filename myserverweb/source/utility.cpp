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
#include <sys/time.h>
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
*Return 0 on fails.
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
	ret = GetVersionEx(&osvi);
	if(!ret)
		return 0;	
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
	/*! By default use 1 processor.  */
	u_long ret=1;
#ifdef WIN32
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	ret=si.dwNumberOfProcessors;
#endif
#ifdef NOT_WIN
	ret=(u_long)sysconf(_SC_NPROCESSORS_CONF); 
        /*! Use only a processor if some error happens.  */
        if(ret==(u_long)-1)
          ret = 1;
#endif
	return ret;
}

/*!
*Save the current working directory.
*Return -1 on fails.
*Return 0 on success.
*/
int setcwdBuffer()
{
#ifdef WIN32	
	char* ret =(char*) _getcwd(currentPath,MAX_PATH);
	if(ret == 0)
		return -1;
	ret=0;
	for(u_long i=0;i<(u_long)strlen(currentPath);i++)
		if(currentPath[i]=='\\')
			currentPath[i]='/';
	if(currentPath[strlen(currentPath)]=='/')
		currentPath[strlen(currentPath)]='\0';
#endif
#ifdef NOT_WIN
	char *ret=getcwd(currentPath,MAX_PATH);
	if(!ret)
		return -1;
	ret=0;
	if(currentPath[strlen(currentPath)]=='/') 
		currentPath[strlen(currentPath)]='\0';
#endif
	return (-1);
}
/*!
*Get the default working directory(Where is the main executable).
*/
char *getdefaultwd(char *path,int len)
{
#ifdef WIN32
	if(path)
		lstrcpyn(path,currentPath,len);
#endif
#ifdef NOT_WIN
	if(path)
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
*Set the text color to red on black.
*Return 0 on success.
*/
int preparePrintError()
{
#ifdef WIN32
	int ret = SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_RED|FOREGROUND_INTENSITY);
	if(ret)
		return 0;
#endif
	return -1;
}

/*!
*Set the text color to white on black.
*Return 0 on success.
*/
int endPrintError()
{
#ifdef WIN32
	int ret = SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
	if(ret)
		return 0;
#endif
	return -1;
}
#ifndef WIN32
static struct timeval tval;
#endif

/*!
*Return the ticks count. Used for check time variations.
*Return 0 on errors.
*/
u_long get_ticks()
{
#ifdef WIN32
	return GetTickCount();
#else
	int ret = gettimeofday(&tval, 0);
        if(ret == -1)
          return 0;
	u_long time=(tval.tv_sec * 1000) + (tval.tv_usec / 1000);
	return time;
#endif
}
