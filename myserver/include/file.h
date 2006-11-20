/*
MyServer
Copyright (C) 2002, 2003, 2004 The MyServer Team
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

#ifndef FILE_H
#define FILE_H

#include "../stdafx.h"
#include "../include/stream.h"
#include <string>

#ifdef WIN32
typedef void* FileHandle;
#endif
#ifdef NOT_WIN
typedef long  FileHandle;
#endif

using namespace std;

class File : public Stream
{
private:
	FileHandle handle;
	string filename;
public:
	static const u_long OPEN_READ = (1<<0);
	static const u_long OPEN_WRITE = (1<<1);
	static const u_long OPEN_TEMPORARY = (1<<2);
	static const u_long OPEN_HIDDEN = (1<<3);
	static const u_long OPEN_ALWAYS = (1<<4);
	static const u_long OPEN_IFEXISTS = (1<<5);
	static const u_long OPEN_APPEND = (1<<6);
	static const u_long CREATE_ALWAYS = (1<<7);
	static const u_long NO_INHERIT = (1<<8);

	File();
  File(char *,int);
	FileHandle getHandle();
	int setHandle(FileHandle);
	int readFromFile(char* ,u_long ,u_long* );
	int writeToFile(const char* ,u_long ,u_long* );
	int createTemporaryFile(const char* );

	int openFile(const char*, u_long );
  int openFile(string const &file, u_long opt)
    {return openFile(file.c_str(), opt);}

	u_long getFileSize();
	int setFilePointer(u_long);

	time_t getLastModTime();
	time_t getCreationTime();
	time_t getLastAccTime();
	const char *getFilename();
	int setFilename(const char*);
  int setFilename(string const &name)
    {return setFilename(name.c_str());}

	int operator =(File);
	int closeFile();

  /*! Inherithed from Stream. */
  virtual int read(char* buffer, u_long len, u_long *nbr);
  virtual int write(const char* buffer, u_long len, u_long *nbw);

};
#endif
