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

int _finddata_t::findfirst(const char filename[])
{
   struct dirent * dirInfo;
   struct stat F_Stats;
   char *TempName= new char[strlen(filename)+1];
   if(TempName == 0)
     return -1;
   strcpy(DirName, filename);
   
   if(DirName[strlen(DirName) - 1] == '/')
     DirName[strlen(DirName) - 1] = '\0';
     
   dh = opendir(DirName);
   if(dh == NULL)
     return -1;
   
   dirInfo = readdir(dh);
   snprintf(TempName, PATH_MAX, "%s/%s", DirName, dirInfo->d_name);
   
   name = dirInfo->d_name;

   stat(TempName, &F_Stats);
   if(S_ISDIR(F_Stats.st_mode))
     attrib = FILE_ATTRIBUTE_DIRECTORY;
   time_write = F_Stats.st_mtime;
   size = F_Stats.st_size;
   delete [] TempName;
   return 0;
}

int _finddata_t::findnext()
{
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

}

int _finddata_t::findclose()
{
   closedir(dh);
   return 0;
}

intptr_t _findfirst(const char filename[], _finddata_t * fdat )
{
   return (fdat->findfirst(filename) == 0)? (intptr_t)fdat : (intptr_t)-1;
}

int _findnext(intptr_t crap, _finddata_t * fdat )
{
   return fdat->findnext();
}

int _findclose(intptr_t fdat) // a nasty little hack
{                             // but hey, intptr_t is a void * anyways
   return ((_finddata_t *)fdat)->findclose();
}


#endif
