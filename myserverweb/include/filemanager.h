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
typedef void *MYSERVER_FILE_HANDLE;
#endif
#ifdef NOT_WIN
typedef FILE*  MYSERVER_FILE_HANDLE;
#endif

#define MYSERVER_FILE_OPEN_READ (1<<0)
#define MYSERVER_FILE_OPEN_WRITE (1<<1)
#define MYSERVER_FILE_OPEN_TEMPORARY (1<<2)
#define MYSERVER_FILE_OPEN_HIDDEN (1<<3)
#define MYSERVER_FILE_OPEN_ALWAYS (1<<4)
#define MYSERVER_FILE_OPEN_IFEXISTS (1<<5)
#define MYSERVER_FILE_OPEN_APPEND (1<<6)
#define MYSERVER_FILE_CREATE_ALWAYS (1<<7)
#define MYSERVER_FILE_NO_INHERIT (1<<8)

class MYSERVER_FILE
{
private:
	MYSERVER_FILE_HANDLE handle;
	char *filename;
public:
	MYSERVER_FILE();
  MYSERVER_FILE(char *,int);

	MYSERVER_FILE_HANDLE getHandle();
	int setHandle(MYSERVER_FILE_HANDLE);
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
	int operator =(MYSERVER_FILE);
	int closeFile();
	int getShortFileName(char*,int);
	static int completePath(char**, int *size, int dontRealloc=0);
	static int isFolder(char*);
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
