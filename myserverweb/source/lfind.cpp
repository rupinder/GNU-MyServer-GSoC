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
#include <string.h>
}

/*!
 *Initialize class members.
 */
FindData::FindData()
{
#ifdef WIN32
  ff = 0;
#endif
#ifdef NOT_WIN
  DirName = 0;
  dh = 0;
#endif
}

/*!
 *Free class members.
 */
FindData::~FindData()
{
#ifdef NOT_WIN
  if(DirName)
    delete [] DirName;
  DirName = 0;
#endif
}

/*!
 *Find the first file using its name.
 *Return -1 or errors.
 */
int FindData::findfirst(const char filename[])
{
#ifdef WIN32
  ff = _findfirst(filename, &fd );
  if(ff!=-1)
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

/*!
 *Find the next file in the directory.
 */
int FindData::findnext()
{
#ifdef WIN32
  if(!ff)
    return -1;
  int ret = _findnext(ff, &fd)? -1 : 0 ;
  if(ret!=-1)
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

/*!
 *Free the used resources.
 */
int FindData::findclose()
{
#ifdef WIN32
  int ret;
  if(!ff)
    return -1;
  ret = _findclose(ff);
  ff = 0;
  return ret;
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

