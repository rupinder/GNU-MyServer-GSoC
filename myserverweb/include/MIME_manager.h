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
#include "..\include\utility.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <io.h>

#ifndef MAX_MIME_TYPES
#define MAX_MIME_TYPES 250
#endif

/*
MIME_Manager::data is formated from this fields
1)	extension
2)	MIME type
3)	CGI manager
*/
class MIME_Manager
{
	DWORD numMimeTypesLoaded;
	char data[MAX_MIME_TYPES][3][MAX_PATH];
public:
	DWORD getNumMIMELoaded();
	HRESULT load(char *filename);
	BOOL getMIME(char* a,char * b,char* c=NULL);
	VOID clean();
	VOID dumpToFILE(char*);
};

