/*
*MyServer
*Copyright (C) 2002 The MyServer Team
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

#ifndef LFIND_H
#define LFIND_H

#include "../stdafx.h"

extern "C" 
{
#ifdef WIN32
#include <io.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <dirent.h>
#include <limits.h>
}

#ifndef EACCES
#define EACCES 1
#endif
#define MAX_NAME NAME_MAX
#define FILE_ATTRIBUTE_DIRECTORY 1

#ifndef intptr_t
#define intptr_t int
#endif

class myserver_finddata_t 
{ 
 public:
   char * name;
   int attrib;
   time_t time_write;
   off_t size;
   int findfirst(const char filename[]);
   int findnext();
   int findclose();
   myserver_finddata_t();
   ~myserver_finddata_t();
 private:
#ifdef WIN32
	_finddata_t fd;
   intptr_t  ff;
#endif
   char *DirName;
   DIR *dh;
};

#endif


