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

#ifdef WIN32
typedef void* FileHandle;
#endif
#ifdef NOT_WIN
typedef long  FileHandle;
#endif

class File
{
private:
	FileHandle handle;
	char *filename;
public:
	File();
  File(char *,int);
  static const u_long OPEN_READ = (1<<0);
  static const u_long OPEN_WRITE = (1<<1);
  static const u_long OPEN_TEMPORARY = (1<<2);
  static const u_long OPEN_HIDDEN = (1<<3);
  static const u_long OPEN_ALWAYS = (1<<4);
  static const u_long OPEN_IFEXISTS = (1<<5);
  static const u_long OPEN_APPEND = (1<<6);
  static const u_long CREATE_ALWAYS = (1<<7);
  static const u_long NO_INHERIT = (1<<8);
	FileHandle getHandle();
	int setHandle(FileHandle);
	int readFromFile(char* ,u_long ,u_long* );
	int writeToFile(char* ,u_long ,u_long* );
	int createTemporaryFile(char* );
	int openFile(char*, u_long );
	u_long getFileSize();
	int setFilePointer(u_long);
	static int getPathRecursionLevel(char*);
	static time_t getLastModTime(char *filename);
	static time_t getCreationTime(char *filename);
	static time_t getLastAccTime(char *filename);
	time_t getLastModTime();
	time_t getCreationTime();
	time_t getLastAccTime();
	char *getFilename();
	int setFilename(char*);
	int operator =(File);
	int closeFile();
	int getShortFileName(char*,int);
	static int completePath(char**, int *size, int dontRealloc=0);
	static int isDirectory(char*);
	static int getShortFileName(char*,char*,int);
	static int fileExists(char * );
	static int deleteFile(char * );
	static void getFileExt(char* ext,const char* filename);
  static void splitPathLength(const char *path, int *dir, int *filename);
	static void splitPath(const char* path, char* dir, char*filename);
  static int getFilenameLength(const char*, int *);
	static void getFilename(const char* path, char* filename);

};
#endif
