/*
*myServer
*Copyright (C) Rocky_10_Balboa
*This library is free software; you can redistribute it and/or
*modify it under the terms of the GNU Library General Public
*License as published by the Free Software Foundation; either
*version 2 of the License, or (at your option) any later version.

*This library is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*Library General Public License for more details.

*You should have received a copy of the GNU Library General Public
*License along with this library; if not, write to the
*Free Software Foundation, Inc., 59 Temple Place - Suite 330,
*Boston, MA  02111-1307, USA.
*/
#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "../stdafx.h"
typedef void *MYSERVER_FILE_HANDLE;
u_long ms_accessesLogWrite(char*);
void ms_setAccessesLogFile(MYSERVER_FILE_HANDLE);

u_long ms_warningsLogWrite(char*);
void ms_setWarningsLogFile(MYSERVER_FILE_HANDLE);
u_long ms_getFileSize(MYSERVER_FILE_HANDLE);
int ms_setFilePointer(MYSERVER_FILE_HANDLE,u_long);
int ms_getPathRecursionLevel(char*);

#define MYSERVER_FILE_OPEN_READ (1<<0)
#define MYSERVER_FILE_OPEN_WRITE (1<<1)
#define MYSERVER_FILE_OPEN_TEMPORARY (1<<2)
#define MYSERVER_FILE_OPEN_HIDDEN (1<<3)
#define MYSERVER_FILE_OPEN_ALWAYS (1<<4)
#define MYSERVER_FILE_OPEN_IFEXISTS (1<<5)
#define MYSERVER_FILE_OPEN_APPEND (1<<6)
#define MYSERVER_FILE_CREATE_ALWAYS (1<<7)

int ms_ReadFromFile(MYSERVER_FILE_HANDLE ,char * ,u_long ,u_long * );
int ms_WriteToFile(MYSERVER_FILE_HANDLE ,char * ,u_long ,u_long * );
MYSERVER_FILE_HANDLE ms_CreateTemporaryFile(char * );
MYSERVER_FILE_HANDLE ms_OpenFile(char * ,u_long );
int ms_IsFolder(char [] );
int ms_CloseFile(MYSERVER_FILE_HANDLE);
int ms_FileExists(char * );
int ms_DeleteFile(char * );
#endif
