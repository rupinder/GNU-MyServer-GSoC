/*
MyServer
Copyright (C) 2006 The MyServer Team
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

#ifndef CACHED_FILE_H
#define CACHED_FILE_H

#include "../stdafx.h"
#include "../include/stream.h"
#include "../include/file.h"
#include "../include/cached_file_buffer.h"
#include <string>

using namespace std;

class CachedFile : public File
{
protected:
	u_long fseek;
	CachedFileBuffer* buffer;
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

};
#endif
