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


#ifndef MIME_MANAGER_H
#define MIME_MANAGER_H

#include "../include/utility.h"
#ifdef WIN32
#include <windows.h>
#endif
extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef WIN32
#include <tchar.h>
#include <io.h>
#endif
}
#ifdef __linux__
using namespace std;
#endif
/*
*This enum describes all the way that a file is handled by the server.
*/
enum CGI_COMMANDS
{
	CGI_CMD_SEND,		/*Sends the file as it is; for example an HTML page*/
	CGI_CMD_RUNCGI,		/*Run the cgi_manager program*/
	CGI_CMD_RUNISAPI,	/*Run the ISAPI module*/
	CGI_CMD_RUNMSCGI,	/*Run the the file as a MSCGI script*/
	CGI_CMD_EXECUTE,	/*Handle the file as an executable*/
	CGI_CMD_SENDLINK,	/*Send the file included  in the file*/
	CGI_CMD_WINCGI		/*Send the file as a WinCGI*/
};
class MIME_Manager
{
public:
	struct mime_record
	{
		char extension[10];
		char mime_type[60];
		int command;
		char cgi_manager[MAX_PATH];
		mime_record* next;
	};
private:
	mime_record *data;
	u_long numMimeTypesLoaded;
	char filename[MAX_PATH];
public:
	char *getFilename();
	MIME_Manager();
	void addRecord(MIME_Manager::mime_record);
	MIME_Manager::mime_record *getRecord(char ext[10]);
	void removeAllRecords();
	void removeRecord(char*);
	u_long getNumMIMELoaded();
	int load(char *filename);
	int save(char *filename);
	int getMIME(char* ext,char *dest,char *dest2);
	int getMIME(int id,char* ext,char *dest,char *dest2);
	void clean();
};
#endif 
