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

#ifndef WIN32
#ifndef LFIND_H
#define LFIND_H

#include "../stdafx.h"

extern "C" 
{
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <dirent.h>
#include <limits.h>
}


typedef  int intptr_t;

using namespace std;


#ifndef EACCES
#define EACCES 1
#endif
#define MAX_NAME NAME_MAX
#define FILE_ATTRIBUTE_DIRECTORY 1

class _finddata_t 
{ 
 public:
   char * name;
   int attrib;
   time_t time_write;
   off_t size;
   int findfirst(const char filename[]);
   int findnext();
   int findclose();
   _finddata_t();
   ~_finddata_t();
 private:
   char *DirName;
   DIR * dh;
};

intptr_t _findfirst(const char filename[], _finddata_t * fdat );
int _findnext(intptr_t crap, _finddata_t * fdat );
int _findclose(intptr_t fdat);

#endif

#endif

