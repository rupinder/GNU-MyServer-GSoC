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
#pragma once

#include "..\stdafx.h"
typedef void *MYSERVER_FILE_HANDLE;
DWORD accessesLogWrite(char*);
void setAccessesLogFile(MYSERVER_FILE_HANDLE);

DWORD warningsLogWrite(char*);
void setWarningsLogFile(MYSERVER_FILE_HANDLE);
DWORD getFileSize(MYSERVER_FILE_HANDLE);
BOOL setFilePointer(MYSERVER_FILE_HANDLE,DWORD);
int getPathRecursionLevel(char*);

#define MYSERVER_FILE_OPEN_READ (1<<0)
#define MYSERVER_FILE_OPEN_WRITE (1<<1)
#define MYSERVER_FILE_OPEN_TEMPORARY (1<<2)
#define MYSERVER_FILE_OPEN_HIDDEN (1<<3)
#define MYSERVER_FILE_OPEN_ALWAYS (1<<4)
#define MYSERVER_FILE_OPEN_IFEXISTS (1<<5)
#define MYSERVER_FILE_OPEN_APPEND (1<<6)
#define MYSERVER_FILE_CREATE_ALWAYS (1<<7)

INT	ms_ReadFromFile(MYSERVER_FILE_HANDLE,char*,DWORD,DWORD*);
INT	ms_WriteToFile(MYSERVER_FILE_HANDLE,char*,DWORD,DWORD*);
MYSERVER_FILE_HANDLE ms_CreateTemporaryFile(char*);
MYSERVER_FILE_HANDLE ms_OpenFile(char*,DWORD);
INT	ms_IsFolder(char*);
INT ms_CloseFile(MYSERVER_FILE_HANDLE);
INT ms_FileExists(char*);
INT ms_DeleteFile(char*);