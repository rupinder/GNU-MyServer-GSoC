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
#pragma once

#include "..\stdafx.h"
#include "..\include\FileManager.h"
#include "..\include\Stringutils.h"
/*
*Macros to do simple transformations.
*/
#define KB(x) (x*1024)
#define MB(x) (KB(x)*1024)
#define SEC(x) (x*1000)
#define my_intabs(x)((x<0)?(-x):(x))
#define OS_WINDOWS_9X		1	
#define OS_WINDOWS_2000		2
#define OS_WINDOWS_NT3		3
#define OS_WINDOWS_XP		4
#undef min
#undef max
#define min(a,b)		((a<b)?a:b)
#define max(a,b)		((a>b)?a:b)

/*
*Structure used for start a new process.
*/
#ifndef START_PROC_INFO_IN
#define START_PROC_INFO_IN
struct START_PROC_INFO
{
	MYSERVER_FILE_HANDLE stdError;
	MYSERVER_FILE_HANDLE stdOut;
	MYSERVER_FILE_HANDLE stdIn;
	char *cmdLine;
	void *envString;
};
#endif

INT getOSVersion();
DWORD getCPUCount();
void gotoNextLine(char*);
DWORD execHiddenProcess(START_PROC_INFO*,DWORD=0xFFFFFFFF);
VOID getComputerName(char*,DWORD);
int ms_setcwd();
char *ms_getcwd(char*,int);

/*
*These functions are a simple trasposition of the mutex mechanism.
*/
INT requestAccess(DWORD*,DWORD);
INT terminateAccess(DWORD*,DWORD);
