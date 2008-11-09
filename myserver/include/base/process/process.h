/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008 Free Software Foundation, Inc.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PROCESS_H
#define PROCESS_H

#include "stdafx.h"

#include <include/base/file/file.h>
#include <include/base/sync/mutex.h>
#include <include/base/process/fork_server.h>
#include <include/base/string/stringutils.h>

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
public:
#ifdef HAVE_PTHREAD
	static Mutex forkMutex;
	static void forkPrepare();
	static void forkParent();
	static void forkChild();
#endif
	static void initialize();
  int exec (StartProcInfo *spi, bool waitEnd = false);
  int terminateProcess();
  int isProcessAlive();
  static int setuid(u_long);
  static int setgid(u_long);
	static int setAdditionalGroups(u_long len, u_long *groups);
  Process();
  ~Process();

  /*! Return the process ID.  */
  int getPid (){return pid;}

  /*! Change the process ID.  */
  void setPid (int pid){this->pid = pid;}

  static int generateEnvString (const char **envp, char *envString);
  static int generateArgList (const char **args, const char *proc, string &additionalArgs);

  static ForkServer *getForkServer (){return &forkServer;}
private:
  int pid;
  static ForkServer forkServer;
};
#endif
