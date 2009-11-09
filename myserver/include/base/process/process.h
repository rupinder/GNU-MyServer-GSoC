/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008, 2009 Free Software Foundation, Inc.
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
# define PROCESS_H

# include "stdafx.h"

# include <include/base/file/file.h>
# include <include/base/sync/mutex.h>
# include <include/base/process/fork_server.h>
# include <include/base/string/stringutils.h>

# include <string>

/*!
 *Structure used for start a new process.
 */
struct StartProcInfo
{
  StartProcInfo ()
  {
    envString = NULL;
    handlesToClose = NULL;
  }

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

  /*! Group id for the new process.  */
  string gid;

  /*! User id for the new process.  */
  string uid;

  void *envString;

  /*! Pointer to a NULL terminated array of
   *  file pointers to close.  */
  FileHandle *handlesToClose;

  /* If the length > 0 then change the current root
     directory to the specified value before execute
     the process.  */
  string chroot;
};

class Process
{
public:
# ifdef HAVE_PTHREAD
  static Mutex forkMutex;
  static void forkPrepare ();
  static void forkParent ();
  static void forkChild ();
# endif
  static void initialize ();
  int exec (StartProcInfo *spi, bool waitEnd = false);
  int terminateProcess ();
  int isProcessAlive ();
  static int chroot (const char *root);
  static int setuid (const char*);
  static int setgid (const char*);
  static int setAdditionalGroups (u_long len, u_long *groups);
  Process ();
  ~Process ();

  /*! Return the process ID.  */
  int getPid (){return pid;}

  /*! Change the process ID.  */
  void setPid (int pid){this->pid = pid;}

  static uid_t getUid (const char *user);
  static gid_t getGid (const char *group);

  static int generateEnvString (const char **envp, size_t size,
                                char *envString);
  static int generateArgList (const char **args, size_t size, const char *proc,
                              string &additionalArgs);

  static ForkServer *getForkServer (){return &forkServer;}
private:
  int pid;
  static ForkServer forkServer;
};
#endif
