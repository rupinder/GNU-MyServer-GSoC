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

#ifndef MIME_Manager_IN
#define MIME_Manager_IN
/*
*This enum describes all the way that a file is treated by the server.
*/
enum CGI_COMMANDS
{
	CGI_CMD_SEND,		/*Sends the file as it is; for example an HTML page*/
	CGI_CMD_RUNCGI,		/*Run the cgi_manager program*/
	CGI_CMD_RUNMSCGI,	/*Run the the file as a MSCGI script*/
	CGI_CMD_EXECUTE		/*Handle the file as an executable*/
};
class MIME_Manager
{
	struct mime_record
	{
		char extension[10];
		char mime_type[16];
		int command;
		char cgi_manager[MAX_PATH];
		mime_record* next;
	};
	mime_record *data;
	DWORD numMimeTypesLoaded;
public:
	MIME_Manager();
	VOID addRecord(MIME_Manager::mime_record);
	VOID removeAllRecords();
	VOID removeRecord(char*);
	DWORD getNumMIMELoaded();
	HRESULT load(char *filename);
	int getMIME(char* a,char * b,char* c=NULL);
	VOID clean();
};
#endif 