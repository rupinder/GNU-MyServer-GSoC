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

#ifndef UTILITY_H
#define UTILITY_H

#include "../stdafx.h"
#include "../include/filemanager.h"
#include "../include/stringutils.h"
#include "../include/processes.h"
#include "../include/threads.h"
/*!
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


int preparePrintError();
int endPrintError();

int getOSVersion();
u_long getCPUCount();
u_long get_ticks();
int setcwdBuffer();
char *getdefaultwd(char* dwd,int maxlen);
int setcwd(char * cwd);


#endif
