/*
MyServer
Copyright (C) 2006, 2008 Free Software Foundation, Inc.
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

#ifndef CACHED_FILE_H
#define CACHED_FILE_H

#include "stdafx.h"
#include <include/filter/stream.h>
#include <include/base/file/file.h>
#include <include/base/files_cache/cached_file_buffer.h>
#include <string>

using namespace std;

class CachedFile : public File
{
public:
	CachedFile(CachedFileBuffer* buffer);
	virtual FileHandle getHandle();
	virtual int setHandle(FileHandle);
	virtual int readFromFile(char* ,u_long ,u_long* );
	virtual int writeToFile(const char* ,u_long ,u_long* );
	virtual int createTemporaryFile(const char* );

	virtual int openFile(const char*, u_long );
  virtual int openFile(string const &file, u_long opt)
    {return openFile(file.c_str(), opt);}

	virtual u_long getFileSize();
	virtual int setFilePointer(u_long);

	virtual int operator =(CachedFile);
	virtual int closeFile();

  /*! Inherithed from Stream. */
  virtual int write(const char* buffer, u_long len, u_long *nbw);
protected:
	u_long fseek;
	CachedFileBuffer* buffer;
};
#endif
