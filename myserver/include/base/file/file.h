/* -*- mode: cpp-mode */
/*
MyServer
Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.
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

#ifndef FILE_H
#define FILE_H

#include "stdafx.h"
#include <include/filter/stream.h>
#include <string>

using namespace std;

class File : public Stream
{
protected:
	FileHandle handle;
	string filename;
public:
	static const u_long MYSERVER_OPEN_READ;
	static const u_long MYSERVER_OPEN_WRITE;
	static const u_long MYSERVER_OPEN_TEMPORARY;
	static const u_long MYSERVER_OPEN_HIDDEN;
	static const u_long MYSERVER_OPEN_ALWAYS;
	static const u_long MYSERVER_OPEN_IFEXISTS;
	static const u_long MYSERVER_OPEN_APPEND;
	static const u_long MYSERVER_CREATE_ALWAYS;
	static const u_long MYSERVER_NO_INHERIT;

	File();
  File(char *,int);
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

	virtual time_t getLastModTime();
	virtual time_t getCreationTime();
	virtual time_t getLastAccTime();
	virtual const char *getFilename();
	virtual int setFilename(const char*);
  virtual int setFilename(string const &name)
    {return setFilename(name.c_str());}

	virtual int operator =(File);
	virtual int closeFile();

  /*! Inherithed from Stream. */
  virtual int read(char* buffer, u_long len, u_long *nbr);
  virtual int write(const char* buffer, u_long len, u_long *nbw);

};
#endif
