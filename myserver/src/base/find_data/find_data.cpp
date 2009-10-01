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
#include <include/base/find_data/find_data.h>

extern "C"
{
#include <stdio.h>
#include <string.h>

#ifdef HAVE_OPENAT
# include <fcntl.h>
#endif
}

using namespace std;

/*!
 * Initialize class members.
 */
FindData::FindData ()
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
FindData::~FindData ()
{

}

/*!
 * Find the first file using its name.
 * Return -1 or errors.
 */
int FindData::findfirst (const char *filename)
{
#ifdef WIN32
  string filenameStar;
  filenameStar.assign (filename);
  // trim ending '/' or '\'
  string::size_type slashBackSlash = filenameStar.find_last_not_of ("/\\");
  filenameStar.erase (slashBackSlash + 1);
  filenameStar.append ("\\*");

  ff = _findfirst (filenameStar.c_str (), &fd );
  if (ff!=-1)
    {
      name = fd.name;
      attrib = fd.attrib;
      time_write = fd.time_write;
      size = fd.size ;
      return 0;
    }
  else
    return ff;

#else
   struct dirent * dirInfo;

   dirName.assign (filename);
   if (dirName[dirName.length () - 1] == '/')
     dirName.erase (dirName.length () - 1);

   dh = opendir (dirName.c_str ());

   if (dh == NULL)
     return -1;

   dirInfo = readdir (dh);

   if (dirInfo == NULL)
     return -1;

   name = dirInfo->d_name;

# ifdef HAVE_FSTATAT
   if (fstatat (dirfd (dh), dirInfo->d_name, &stats, 0))
     return -1;
# else
   string tempName = filename;

   tempName.assign (dirName);
   tempName.append ("/");
   tempName.append (dirInfo->d_name);

   if (stat (tempName.c_str (), &stats))
     return -1;
# endif

   if (S_ISDIR (stats.st_mode))
     attrib = FILE_ATTRIBUTE_DIRECTORY;

   time_write = stats.st_mtime;
   size = stats.st_size;
   return 0;
#endif
}

/*!
 * Find the next file in the directory.
 */
int FindData::findnext ()
{
#ifdef WIN32
  if (!ff)
    return -1;
  int ret = _findnext (ff, &fd) ? -1 : 0 ;
  if (ret != -1)
  {
    name = fd.name;
    attrib = fd.attrib;
    time_write = fd.time_write;
    size = fd.size ;
  }
  return ret;
#else
   struct dirent * dirInfo;
   string tempName;

   dirInfo = readdir (dh);

   if (dirInfo == NULL)
     return -1;

   name = dirInfo->d_name;

# ifdef HAVE_FSTATAT
   if (fstatat (dirfd (dh), dirInfo->d_name, &stats, 0))
     return -1;
# else
   tempName.assign (dirName);
   tempName.append ("/");
   tempName.append (dirInfo->d_name);

   if (stat (tempName.c_str (), &stats))
     return -1;
# endif

   if (S_ISDIR (stats.st_mode))
     attrib = FILE_ATTRIBUTE_DIRECTORY;
   else
     attrib = 0;
   time_write = stats.st_mtime;
   size = stats.st_size;
   return 0;
#endif
}

/*!
 * Free the used resources.
 */
int FindData::findclose ()
{
#ifdef WIN32
  int ret;
  if (!ff)
    return -1;
  ret = _findclose (ff);
  ff = 0;
  return ret;
#else
  if (dh)
    closedir (dh);
  dirName.empty ();
  return 0;
#endif
}
