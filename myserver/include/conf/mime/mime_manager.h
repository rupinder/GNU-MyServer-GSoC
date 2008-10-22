/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007 Free Software Foundation, Inc.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MIME_MANAGER_H
#define MIME_MANAGER_H

#include <include/base/utility.h>
#include <include/base/hash_map/hash_map.h>
#include <include/base/sync/read_write_lock.h>

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
#include <map>
#include <list>

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
	CGI_CMD_EXECUTEFASTCGI,	

	/*! Send the file using the specified SCGI server.  */
	CGI_CMD_RUNSCGI,
	
	/*! Send the file as a SCGI.  */
	CGI_CMD_EXECUTESCGI,	

	/*! Use an external plugin to handle this MIME type.  */
	CGI_CMD_EXTERNAL
};


struct MimeRecord
{
	list<string> filters;
	string extension;
	string mimeType;
	string cmdName;
	int command;
	string cgiManager;
	unsigned int extensionHashCode;
	MimeRecord()
	{filters.clear(); extension.assign(""); 
		mimeType.assign(""); cgiManager.assign(""); cmdName.assign("");
		command=extensionHashCode = 0;}
	MimeRecord(MimeRecord&);
	int addFilter(const char*, int acceptDuplicate = 1);
	~MimeRecord();
	void clear();
};

class MimeManager
{
public:
	MimeManager();
  ~MimeManager();
	u_long getNumMIMELoaded();

	int loadXML(const char *filename);
	int loadXML(string &filename)
    {return loadXML(filename.c_str());}

	int saveXML(const char *filename);
	int saveXML(string &filename)
    {return saveXML(filename.c_str());}

	int getMIME(char* ext,char *dest,char **dest2);
	int getMIME(int id,char* ext,char *dest,char **dest2);
  int getMIME(string& ext,string& dest,string& dest2);
  int getMIME(int id,string& ext,string& dest,string& dest2);
  int isLoaded();
	MimeRecord *getRecord(string const &ext);
	void clean();
protected:
	const char *getFilename();
	int addRecord(MimeRecord& record);
	void removeAllRecords();
	void removeRecord(const string& ext);
private:
  int loaded;
  HashMap<string, MimeRecord*> *data;
	u_long numMimeTypesLoaded;
	string *filename;
	ReadWriteLock rwLock;
};

#endif 
