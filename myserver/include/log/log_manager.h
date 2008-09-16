/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.
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

#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include "stdafx.h"
#include <include/base/file/file.h>
#include <include/base/thread/thread.h>
#include <include/base/sync/mutex.h>

#include <string>
using namespace std;

class LogManager
{
private:
  File file;
  int type;
  /*!
   *loaded is used to store if the file object is initialized correctly.
   */
  int loaded;
  u_long maxSize;
	Mutex mutex;
  int cycleLog;
  int gzipLog;
public:
  void setGzip(bool);
  int getGzip();
  void setCycleLog(int);
  int getCycleLog();
  const static int TYPE_CONSOLE;
  const static int TYPE_FILE;
  File *getFile();
  LogManager();
  ~LogManager();
  u_long setMaxSize( u_long );
  u_long getMaxSize();
  int requestAccess();
  int terminateAccess();

  int load(const char *filename );
  int load(string const &filename)
    {return load(filename.c_str());}

  int close();
  int preparePrintError();
  int endPrintError();
  void setType( int );
  int getType();
  int write( const char *str, int len = 0 );
  int writeln(const char *);

  int write(string const &str, int len = 0 )
    {return write(str.c_str(), len);}
  int writeln(string const& str)
    {return writeln(str.c_str());}

  int getLogSize();
  int storeFile();
};
#endif
