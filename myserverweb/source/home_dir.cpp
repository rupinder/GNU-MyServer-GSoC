/*
MyServer
Copyright (C) 2006 The MyServer Team
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "../stdafx.h"
#include "../include/file.h"
#include "../include/home_dir.h"

extern "C" {
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#ifndef WIN32
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include <sys/wait.h>
#endif
}

#include <sys/types.h>

#ifdef WIN32
#include <direct.h>
#endif

/*!
 *Class constructor.
 */
HomeDir::HomeDir()
{
	data.clear();
	timestamp = 0;
	loaded = 0;
}

/*!
 *Destroy th object.
 */
HomeDir::~HomeDir()
{
	clear();
}

/*!
 *Clear the used memory.
 */
void HomeDir::clear()
{
	HashMap<string, string*>::Iterator i = data.begin();
	HashMap<string, string*>::Iterator end = data.end();
	for( ; i != end ; i++)
	{
		string* val = *i;
		if(val)
			delete val;
	}
	data.clear();
	timestamp = 0;
	loaded = 0;
}

/*!
 *Load the internal buffer.
 */
int HomeDir::load()
{

#ifdef NOT_WIN
	File usersFile;
	u_long size;
	char* buffer;
	u_long read;
	u_long counter;

	clear();

	if(usersFile.openFile("/etc/passwd", FILE_OPEN_READ | 
												FILE_OPEN_IFEXISTS))
		return 1;
	size = usersFile.getFileSize();
	timestamp = usersFile.getLastModTime();
	if(size == (u_long) -1)
  {
		usersFile.closeFile();
		return 1;
	}

	buffer = new char[size+1]; 

	usersFile.read(buffer, size, &read);
	buffer[read] = '\0';
	
	for(counter = 0; counter < read; counter++)
		if(buffer[counter] == ':' || buffer[counter] == '\n')
			buffer[counter] = '\0';

	for(counter = 0; buffer[counter] != 0;)
	{
		char *username = 0;
		char *home = 0;
		string sUsername;
		string *sHome;
 		string *old;
		/* Username.  */
 
		username = &buffer[counter];

 		while(buffer[counter++] != '\0');
		/* Password.  */

 		while(buffer[counter++] != '\0');
		/* User ID.  */

 		while(buffer[counter++] != '\0');
		/* Group ID.  */

 		while(buffer[counter++] != '\0');
		/* Info.  */

 		while(buffer[counter++] != '\0');
		/* Home.  */
		

		home = &buffer[counter++];
		sUsername = string(username);
		sHome = new string(home);

		old = data.put(sUsername, sHome);

		if(old)
			delete old;

		while(buffer[counter++] != '\0');
		/* Shell.  */

 		while(buffer[counter++] != '\0');
		/* Next tuple.  */
	}

	delete [] buffer;
	usersFile.closeFile();
#endif
	return 0;

}

/*!
 *Get the home directory for a specified user.
 *\param userName The user name.
 */
const string *HomeDir::getHomeDir(string& userName)
{
	if(!loaded)
		return 0;
	/* TODO: don't check always but wait some time before.  */
	if(File::getLastModTime("/etc/passwd") != timestamp)
		load();

	return data.get(userName);
}
