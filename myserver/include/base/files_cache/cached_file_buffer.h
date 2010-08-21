/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2006, 2007, 2008, 2009, 2010 Free Software Foundation,
  Inc.
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

#ifndef CACHED_FILE_BUFFER_H
# define CACHED_FILE_BUFFER_H

# include "myserver.h"
# include <include/filter/stream.h>
# include <include/base/file/file.h>
# include <include/base/utility.h>
# include <include/base/sync/mutex.h>
# include <string>

using namespace std;

class CachedFileFactory;

class CachedFileBuffer
{
public:
  void addRef ();
  void decRef ();
  u_long getReferenceCounter ();

  void setFactoryToNotify (CachedFileFactory *cff);
  CachedFileFactory* getFactoryToNotify (){return factoryToNotify;}

  off_t getFileSize (){return fileSize;}
  CachedFileBuffer (const char* filename);
  CachedFileBuffer (File* file);
  CachedFileBuffer (const char* buffer, u_long size);
  ~CachedFileBuffer ();

  const char* getFilename (){return filename.c_str ();}
  const char* getBuffer (){return buffer;}

  time_t getMtime () {return mtime;}

  bool isSymlink () {return S_ISLNK (fstat.st_mode);}

  bool revalidate (const char *res);

  void refreshExpireTime (u_long now) {expireTime = now + MYSERVER_SEC (5);}
  u_long getExpireTime () {return expireTime;}

protected:
  /*! Last mtime for this file.  */
  time_t mtime;

  /*! Specify when to check again the entry validity.  */
  u_long expireTime;

  struct stat fstat;

  Mutex mutex;
  char *buffer;
  u_long refCounter;
  size_t fileSize;
  CachedFileFactory *factoryToNotify;
  string filename;
};
#endif
