/*
*myServer
*Copyright (C) 2002 Giuseppe Scrivano
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
typedef void *MYSERVER_FILE_HANDLE;


#define MYSERVER_FILE_OPEN_READ (1<<0)
#define MYSERVER_FILE_OPEN_WRITE (1<<1)
#define MYSERVER_FILE_OPEN_TEMPORARY (1<<2)
#define MYSERVER_FILE_OPEN_HIDDEN (1<<3)
#define MYSERVER_FILE_OPEN_ALWAYS (1<<4)
#define MYSERVER_FILE_OPEN_IFEXISTS (1<<5)
#define MYSERVER_FILE_OPEN_APPEND (1<<6)
#define MYSERVER_FILE_CREATE_ALWAYS (1<<7)

class MYSERVER_FILE
{
private:
	MYSERVER_FILE_HANDLE handle;
	char filename[MAX_PATH];
public:
	MYSERVER_FILE();
	MYSERVER_FILE_HANDLE ms_GetHandle();
	int ms_ReadFromFile(char * ,u_long ,u_long * );
	int ms_WriteToFile(char * ,u_long ,u_long * );
	int ms_CreateTemporaryFile(char * );
	int ms_OpenFile(char * ,u_long );
	int ms_SetHandle(MYSERVER_FILE_HANDLE);
	u_long ms_getFileSize();
	int ms_setFilePointer(u_long);
	static int ms_getPathRecursionLevel(char*);
	static time_t ms_GetLastModTime(char *filename);
	static time_t ms_GetCreationTime(char *filename);
	static time_t ms_GetLastAccTime(char *filename);
	time_t ms_GetLastModTime();
	time_t ms_GetCreationTime();
	time_t ms_GetLastAccTime();
	char *ms_GetFilename();
	int operator =(MYSERVER_FILE);
	static int ms_IsFolder(char*);
	int ms_CloseFile();
	static int ms_FileExists(char * );
	static int ms_DeleteFile(char * );
};
#endif
