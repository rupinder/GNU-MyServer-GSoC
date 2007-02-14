/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006, 2007 The MyServer Team
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

#ifndef PROCESSES_H
#define PROCESSES_H

#include "../stdafx.h"

#include "../include/file.h"
#include "../include/mutex.h"
#include "../include/stringutils.h"

#include <string>

/*!
 *Structure used for start a new process.
 */
struct StartProcInfo
{
	/*! STDIN file for new process.  */
	FileHandle stdIn;	
	
	/*! STDOUT file for new process.  */
	FileHandle stdOut;
	
	/*! STDERR file for new process.  */
	FileHandle stdError;
	
	string cmdLine;
	string cwd;
	
	/*! added for unix support.  */
	string cmd;
	string arg;
	
	void *envString;
};

class Process
{
private:
  int pid;
public:
#ifdef HAVE_PTHREAD
	static Mutex forkMutex;
	static void forkPrepare();
	static void forkParent();
	static void forkChild();
#endif
	static void initialize();
  int execHiddenProcess(StartProcInfo* spi, u_long timeout = 0xFFFFFFFF);
  int execConcurrentProcess(StartProcInfo* spi);
  int terminateProcess();
  int isProcessAlive();
  static int setuid(u_long);
  static int setgid(u_long);
	static int setAdditionalGroups(u_long len, u_long *groups);
  Process();
  ~Process();
};
#endif
