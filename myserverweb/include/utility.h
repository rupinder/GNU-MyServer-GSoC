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
/*
*Macros to do simple transformation
*/
#define KB(x) (x*1024)
#define MB(x) (KB(x)*1024)
#define SEC(x) (x*1000)
#undef getTime
#define my_intabs(x)((x<0)?(-x):(x))
#define getTime() clock()
#define OS_WINDOWS_9X		1	
#define OS_WINDOWS_2000		2
#define OS_WINDOWS_NT3		3
#define OS_WINDOWS_XP		4
/*
*Structure used for start a new process
*/

struct START_PROC_INFO
{
	MYSERVER_FILE_HANDLE stdError;
	MYSERVER_FILE_HANDLE stdOut;
	MYSERVER_FILE_HANDLE stdIn;
	char *cmdLine;
};

INT getOSVersion();
void gotoNextLine(char*);
char gotoNextLine(FILE*);
char *getHTTPFormattedTime(void);
char *getHTTPFormattedTime(tm*);
VOID StrTrim(LPSTR,LPSTR);
DWORD getCPUCount();
DWORD execHiddenProcess(START_PROC_INFO*);
VOID getComputerName(char*,DWORD);

/*
*These functions are a simple trasposition of the mutex mechanism
*/
INT requestAccess(DWORD*,DWORD);
INT terminateAccess(DWORD*,DWORD);
