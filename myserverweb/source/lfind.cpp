/*
MyServer
Copyright (C) 2002 The MyServer Team
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
  DirName.empty();
  dh = 0;
#endif
}

/*!
 *free class members.
 */
FindData::~FindData()
{
#ifdef NOT_WIN
  DirName.empty();
#endif
}

/*!
 *find the first file using its name.
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
   string TempName;


   DirName.assign(filename);
   
   if(DirName[DirName.length() - 1] == '/')
     DirName[DirName.length() - 1] = '\0';
     
   dh = opendir(DirName.c_str());
   if(dh == NULL)
     return -1;
   
   dirInfo = readdir(dh);

   TempName.assign(DirName);
   TempName.append("/");
   TempName.append(dirInfo->d_name);
   
   name = dirInfo->d_name;

   stat(TempName.c_str(), &F_Stats);
   if(S_ISDIR(F_Stats.st_mode))
     attrib = FILE_ATTRIBUTE_DIRECTORY;
   time_write = F_Stats.st_mtime;
   size = F_Stats.st_size;
   return 0;
#endif
}

/*!
 *find the next file in the directory.
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
   string TempName;
      
   dirInfo = readdir(dh);
   
   if(dirInfo == NULL)
     return -1;
   TempName.assign(DirName);
   TempName.append("/");
   TempName.append(dirInfo->d_name);
   
   name = dirInfo->d_name;

   stat(TempName.c_str(), &F_Stats);
   if(S_ISDIR(F_Stats.st_mode))
     attrib = FILE_ATTRIBUTE_DIRECTORY;
   else
     attrib = 0;
   time_write = F_Stats.st_mtime;
   size = F_Stats.st_size;
   return 0;
#endif
}

/*!
 *free the used resources.
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
   DirName.empty();
   return 0;
#endif
}
