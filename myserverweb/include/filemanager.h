/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
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

#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "../stdafx.h"
#include <string>

#ifdef WIN32
typedef void* FileHandle;
#endif
#ifdef NOT_WIN
typedef long  FileHandle;
#endif

using namespace std;

const u_long FILE_OPEN_READ = (1<<0);
const u_long FILE_OPEN_WRITE = (1<<1);
const u_long FILE_OPEN_TEMPORARY = (1<<2);
const u_long FILE_OPEN_HIDDEN = (1<<3);
const u_long FILE_OPEN_ALWAYS = (1<<4);
const u_long FILE_OPEN_IFEXISTS = (1<<5);
const u_long FILE_OPEN_APPEND = (1<<6);
const u_long FILE_CREATE_ALWAYS = (1<<7);
const u_long FILE_NO_INHERIT = (1<<8);

class File
{
private:
	FileHandle handle;
	string filename;
public:
	File();
  File(char *,int);
	FileHandle getHandle();
	int setHandle(FileHandle);
	int readFromFile(char* ,u_long ,u_long* );
	int writeToFile(const char* ,u_long ,u_long* );
	int createTemporaryFile(char* );

	int openFile(const char*, u_long );
  int openFile(string& file, u_long opt)
    {return openFile(file.c_str(), opt);}

	u_long getFileSize();
	int setFilePointer(u_long);

	static int getPathRecursionLevel(const char*);
  static int getPathRecursionLevel(string& filename)
    {return getPathRecursionLevel(filename.c_str()); }

	static time_t getLastModTime(const char *filename);
	static time_t getLastModTime(string& filename)
    {return getLastModTime(filename.c_str());}

	static time_t getCreationTime(const char *filename);
	static time_t getCreationTime(string& filename)
    {return getCreationTime(filename.c_str());}

	static time_t getLastAccTime(const char *filename);
	static time_t getLastAccTime(string& filename)
    {return getLastAccTime(filename.c_str());}

	time_t getLastModTime();
	time_t getCreationTime();
	time_t getLastAccTime();
	const char *getFilename();
	int setFilename(const char*);
  int setFilename(const string& name)
    {return setFilename(name.c_str());}

	int operator =(File);
	int closeFile();
  int getShortFileName(char*,int);

	static int completePath(char**, int *size, int dontRealloc=0);
  static int completePath(string &fileName);

	static int isDirectory(const char*);
  static int isDirectory(string& dir){return isDirectory(dir.c_str());}
  static int getShortFileName(char*,char*,int);

	static int fileExists(const char * );
	static int fileExists(string& file)
    {return fileExists(file.c_str());}

	static int deleteFile(const char * );
	static int deleteFile(string& file)
    {return deleteFile(file.c_str());}

  static int renameFile(const char*, const char*);
	static int renameFile(string& before, string& after)
    {return renameFile(before.c_str(), after.c_str());}

	static void getFileExt(char* ext,const char* filename);
	static void getFileExt(string& ext,const string& filename);

  static void splitPathLength(const char *path, int *dir, int *filename);
	static void splitPath(const char* path, char* dir, char*filename);
	static void splitPath(const string& path, string& dir, string& filename);

  static int getFilenameLength(const char*, int *);
	static void getFilename(const char* path, char* filename);
	static void getFilename(const string& path, string& filename);

};
#endif
