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

#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include "../stdafx.h"
#include "../include/filemanager.h"
#include "../include/threads.h"

class MYSERVER_LOG_MANAGER
{
private:
  MYSERVER_FILE file;
  /*!
   *loaded is used to store if the file object is initialized correctly.
   */
  int loaded;
  int type;
  int max_size;
	myserver_mutex mutex;
public:
  const static int TYPE_CONSOLE;
  const static int TYPE_FILE;
  MYSERVER_FILE *getFile();
  MYSERVER_LOG_MANAGER();
  ~MYSERVER_LOG_MANAGER();
  int setMaxSize( int );
  int getMaxSize();
  int requestAccess();
  int terminateAccess();
  int load( char *filename );
  int close();
  int preparePrintError();
  int endPrintError();
  void setType( int );
  int getType();
  int write( char *str, int len = 0 );
  int writeln(char *);
  int getLogSize();
};
#endif
