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

#ifndef PROCESSES_H
#define PROCESSES_H

#include "../stdafx.h"
#include "../include/filemanager.h"
#include "../include/stringutils.h"

/*!
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

u_long execHiddenProcess(START_PROC_INFO* spi,u_long timeout=0xFFFFFFFF);
u_long execConcurrentProcess(START_PROC_INFO* spi);
int terminateProcess(u_long);
#endif
