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
#include "../include/lfind.h"

extern "C"
{
#include <stdio.h>
#include <unistd.h>
#include <string.h>
}

#ifndef PATH_MAX
#define PATH_MAX 255
#endif

void * _alloca(size_t size)
{
	return malloc(size);
}

myserver_finddata_t::myserver_finddata_t()
{
#ifdef NOT_WIN
  DirName = 0;
  dh = 0;
#endif
}
myserver_finddata_t::~myserver_finddata_t()
{
#ifdef NOT_WIN
  if(DirName)
    delete [] DirName;
  DirName = 0;
#endif
}

/*!
 *Return zero on success.
 */
int myserver_finddata_t::findfirst(const char filename[])
{
#ifdef WIN32
  ff = _findfirst(filename, &fd )  ? -1 : 0;
  if(ff==0)
  {
    name = fd.name;
    attrib = fd.attrib;
    time_write = fd.time_write;
    size = fd.size ;
  }
  return ff;
#endif
#ifdef NOT_WIN
   struct dirent * dirInfo;
   struct stat F_Stats;
   char *TempName=0;

   if(DirName)
     delete [] DirName;
   DirName= new char[strlen(filename)+1];
   if(DirName == 0)
     return -1;

   strcpy(DirName, filename);
   
   if(DirName[strlen(DirName) - 1] == '/')
     DirName[strlen(DirName) - 1] = '\0';
     
   dh = opendir(DirName);
   if(dh == NULL)
     return -1;
   
   dirInfo = readdir(dh);

   TempName = new char[strlen(DirName) + strlen(dirInfo->d_name) + 2];
   if(TempName == 0)
     return -1;

   sprintf(TempName, "%s/%s", DirName, dirInfo->d_name);
   
   name = dirInfo->d_name;

   stat(TempName, &F_Stats);
   if(S_ISDIR(F_Stats.st_mode))
     attrib = FILE_ATTRIBUTE_DIRECTORY;
   time_write = F_Stats.st_mtime;
   size = F_Stats.st_size;
   delete [] TempName;
   return 0;
#endif
}

int myserver_finddata_t::findnext()
{
#ifdef WIN32
  int ret = _findnext(ff, &fd)? -1 : 0 ;
  if(ret==0)
  {
    name = fd.name;
    attrib = fd.attrib;
    time_write = fd.time_write;
    size = fd.size ;
  }
  return ret;
#endif
#ifdef NOT_WIN
   struct dirent * dirInfo;
   struct stat F_Stats;
   char *TempName;
      
   dirInfo = readdir(dh);
   
   if(dirInfo == NULL)
     return -1;
   TempName = new char[strlen(DirName) + strlen(dirInfo->d_name) + 2];
   if(TempName == 0)
     return -1;
   sprintf(TempName, "%s/%s", DirName, dirInfo->d_name);
   
   name = dirInfo->d_name;

   stat(TempName, &F_Stats);
   if(S_ISDIR(F_Stats.st_mode))
     attrib = FILE_ATTRIBUTE_DIRECTORY;
   else
     attrib = 0;
   time_write = F_Stats.st_mtime;
   size = F_Stats.st_size;
   delete [] TempName;
   return 0;
#endif
}

int myserver_finddata_t::findclose()
{
#ifdef WIN32
  _findclose(ff);
#endif
#ifdef NOT_WIN
  if(dh)
    closedir(dh);
   if(DirName)
     delete [] DirName;
   DirName = 0;
   return 0;
#endif
}


