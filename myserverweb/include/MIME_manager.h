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

#include <string>

using namespace std;

/*!
*This enum describes all the way that a file is handled by the server.
*/
enum CGI_COMMANDS
{
	/*! Sends the file as it is; for example an HTML page.  */
	CGI_CMD_SEND,
			
	/*! Run the cgi_manager program.  */
	CGI_CMD_RUNCGI,
			
	/*! Run the ISAPI module.  */
	CGI_CMD_RUNISAPI,
			
	/*! Run the file as a self ISAPI module.  */
	CGI_CMD_EXECUTEISAPI,
			
	/*! Run the the file as a MSCGI script.  */
	CGI_CMD_RUNMSCGI,
			
	/*! Handle the file as an executable.  */
	CGI_CMD_EXECUTE,
			
	/*! Send the file included  in the file.  */
	CGI_CMD_SENDLINK,
			
	/*! Send the file as a WinCGI.  */
	CGI_CMD_EXECUTEWINCGI,
	
	/*! Send the file using the specified FastCGI server.  */
	CGI_CMD_RUNFASTCGI,
	
	/*! Send the file as a FastCGI.  */
	CGI_CMD_EXECUTEFASTCGI	
};


class MimeManager
{
public:
	struct MimeRecord
	{
		string extension;
		string mime_type;
		int command;
		string cgi_manager;
		MimeRecord* next;
	};
private:
  int loaded;
	MimeRecord *data;
	u_long numMimeTypesLoaded;
	string filename;
public:
	const char *getFilename();
	MimeManager();
  ~MimeManager();
	void addRecord(MimeManager::MimeRecord);
	MimeManager::MimeRecord *getRecord(string const &ext);
	void removeAllRecords();
	void removeRecord(const string& ext);
	u_long getNumMIMELoaded();
	int load(const char *filename);
	int loadXML(const char *filename);
	int saveXML(const char *filename);
	int save(const char *filename);
	int getMIME(char* ext,char *dest,char **dest2);
	int getMIME(int id,char* ext,char *dest,char **dest2);
  int getMIME(string& ext,string& dest,string& dest2);
  int getMIME(int id,string& ext,string& dest,string& dest2);
	void clean();
  int isLoaded();
};
#endif 
