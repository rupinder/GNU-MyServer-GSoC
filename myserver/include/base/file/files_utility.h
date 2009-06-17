/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008, 2009 Free Software Foundation, Inc.
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

#ifndef FILES_UTILITY_H
#define FILES_UTILITY_H

#include "stdafx.h"
#include <include/filter/stream.h>
#include <include/base/file/file.h>
#include <string>

using namespace std;

class FilesUtility
{
private:
  /*! Don't allow instances for this class.  */
  FilesUtility();

public:
  static int getPathRecursionLevel(const char*);
  static int getPathRecursionLevel(string& filename)
  {return getPathRecursionLevel(filename.c_str()); }

  static time_t getLastModTime(const char *filename);
  static time_t getLastModTime(string const &filename)
    {return getLastModTime(filename.c_str());}

  static time_t getCreationTime(const char *filename);
  static time_t getCreationTime(string const &filename)
    {return getCreationTime(filename.c_str());}

  static time_t getLastAccTime(const char *filename);
  static time_t getLastAccTime(string const &filename)
    {return getLastAccTime(filename.c_str());}

  static int chown(const char* filename, int uid, int gid);
  static int chown(string const &filename, int uid, int gid)
    {return chown(filename.c_str(), uid, gid);}

  static int completePath(char**, int *size, int dontRealloc=0);
  static int completePath(string &fileName);

  static int isDirectory(const char*);
  static int isDirectory(const string& dir){return isDirectory(dir.c_str());}

  static int isLink(const char*);
  static int isLink(const string& dir){return isLink(dir.c_str());}

  static int getShortFileName(char*,char*,int);

  static int fileExists(const char * );
  static int fileExists(string const &file)
    {return fileExists(file.c_str());}

  static int deleteFile(const char * );
  static int deleteFile(string const &file)
    {return deleteFile(file.c_str());}

    static int renameFile(const char*, const char*);
  static int renameFile(string const &before, string const &after)
    {return renameFile(before.c_str(), after.c_str());}

  static int copyFile(const char*, const char*, int overwrite);
  static int copyFile(File&, File&);
  static int copyFile(string const &src, string const &dest, int overwrite)
  {return copyFile(src.c_str(), dest.c_str(), overwrite);}

  static void getFileExt(char* ext,const char* filename);
  static void getFileExt(string& ext, string const &filename);

  static void splitPathLength(const char *path, int *dir, int *filename);
  static void splitPath(const char* path, char* dir, char*filename);
  static void splitPath(string const &path, string& dir, string& filename);

  static int getFilenameLength(const char*, int *);
  static void getFilename(const char* path, char* filename);
  static void getFilename(string const &path, string& filename);

  static int rmdir (const char *path);
  static int rmdir (string const &path)
  { return rmdir (path.c_str()); }

  static int mkdir (const char *path);
  static int mkdir (string const &path)
  { return mkdir (path.c_str()); }

  static void temporaryFileName(u_long tid, string &out);

  static void resetTmpPath();
  static void setTmpPath(string & path){tmpPath.assign(path);}

private:
	static string tmpPath;
};
#endif
