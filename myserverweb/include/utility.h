/*
*myServer
*Copyright (C) 2002 Giuseppe Scrivano
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

#ifndef UTILITY_H
#define UTILITY_H

#include "../stdafx.h"
#include "../include/filemanager.h"
#include "../include/stringutils.h"
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
#define OS_LINUX	      100  // Add room for future windows
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
	char *cwd;
	// added for unix support
	char *cmd;
	char *arg;
	
	void *envString;
};
#endif

void preparePrintError();
void endPrintError();

int getOSVersion();
u_long getCPUCount();
void wait(u_long);
u_long execHiddenProcess(START_PROC_INFO* spi,u_long timeout=0xFFFFFFFF);
u_long execConcurrentProcess(START_PROC_INFO* spi);
int terminateProcess(u_long);
int setcwdBuffer();
char *getdefaultwd(char* dwd,int maxlen);
int setcwd(char * cwd);

/*
*These functions are a simple trasposition of the mutex mechanism.
*/
int requestAccess(u_long*,u_long);
int terminateAccess(u_long*,u_long);

#endif
