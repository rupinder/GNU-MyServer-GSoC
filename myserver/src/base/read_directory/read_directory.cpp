/*
  MyServer
  Copyright (C) 2002, 2007, 2008, 2009 Free Software Foundation, Inc.
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

#include "stdafx.h"

#include <include/base/read_directory/read_directory.h>

extern "C"
{
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_OPENAT
# include <fcntl.h>
#endif
}

using namespace std;

#ifndef WIN32
# ifndef HAVE_READDIR_R
Mutex ReadDirectory::mutex;
# endif
#endif

/*!
 * Initialize class members.
 */
ReadDirectory::ReadDirectory ()
{
#ifdef WIN32
  ff = 0;
#else
  dirName.empty ();
  dh = 0;
#endif
}

/*!
 * D'ctor.
 */
ReadDirectory::~ReadDirectory ()
{
 findclose ();
}

/*!
 * Find the first file using its name.
 *
 * \param directory Read the content of this directory.
 * \return 0 if there are other directory entries to be read.
 * \return a positive integer if this is the last entry.
 * \return -1 or errors.
 */
int ReadDirectory::findfirst (const char *directory)
{
  return find (directory);
}

/*!
 * Find the next file in the directory.
 *
 * \return 0 if there are other directory entries to be read.
 * \return a positive integer if this is the last entry.
 * \return -1 or errors.
 */
int ReadDirectory::findnext ()
{
  return find (NULL);
}

/*!
 *\see ReadDirectory#findfirst 
 */
int ReadDirectory::find (const char *filename)
{
#ifdef WIN32
  int ret;

  if (filename)
    {
      string filenameStar;
      filenameStar.assign (filename);
      /* trim trailing '/' or '\'  */
      string::size_type slashBackSlash = filenameStar.find_last_not_of ("/\\");
      filenameStar.erase (slashBackSlash + 1);
      filenameStar.append ("\\*");

      ret = ff = _findfirst (filenameStar.c_str (), &fd );
    }
  else
    ret = _findnext (ff, &fd) ? -1 : 0 ;

  if (ret == -1)
    {
      if (errno == ENOENT)
	return 1;

      return -1;
    }
  else
    {
      name = fd.name;
      attrib = fd.attrib;
      time_write = fd.time_write;
      size = fd.size;
      st_nlink = 1UL;
    }
  return 0;

#else

   struct dirent * dirInfo;

   if (filename)
     {
       dirName.assign (filename);
       if (dirName[dirName.length () - 1] == '/')
         dirName.erase (dirName.length () - 1);

       dh = opendir (dirName.c_str ());

       if (dh == NULL)
         return -1;
     }
   errno = 0;
# ifdef HAVE_READDIR_R
   if (readdir_r (dh, &entry, &dirInfo) || !dirInfo)
     return (errno) ? -1 : 1;
   name = dirInfo->d_name;
# else
   mutex.lock ();
   dirInfo = readdir (dh);
   if (dirInfo == NULL)
     {
       mutex.unlock ();
       return (errno) ? -1 : 1;
     }

   name = dirInfo->d_name;
   mutex.unlock ();
# endif

# ifdef HAVE_FSTATAT
   if (fstatat (dirfd (dh), name.c_str (), &stats, 0))
     return -1;
# else
   string tempName;
   tempName.assign (dirName);
   tempName.append ("/");
   tempName.append (dirInfo->d_name);
   if (stat (tempName.c_str (), &stats))
     return -1;
# endif

   attrib = 0;

   if (S_ISDIR (stats.st_mode))
     attrib = FILE_ATTRIBUTE_DIRECTORY;

   time_write = stats.st_mtime;
   size = stats.st_size;
   st_nlink = stats.st_nlink;
   return 0;
#endif
}

/*!
 * Free the used resources.
 */
int ReadDirectory::findclose ()
{
  dirName.empty ();
#ifdef WIN32
  int ret;
  if (!ff)
    return -1;
  ret = _findclose (ff);
  ff = 0;
  return ret;
#else
  if (!dh)
    return -1;

  closedir (dh);
  dh = NULL;
  return 0;
#endif
}
