/*
MyServer
Copyright (C) 2006, 2008 Free Software Foundation, Inc.
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


#include "stdafx.h"
#include <include/base/file/file.h>
#include <include/base/home_dir/home_dir.h>
#include <include/base/utility.h>
#include <include/base/file/files_utility.h>

#ifdef WIN32
#include <userenv.h>
#endif
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
#ifdef WIN32
  data.assign("");
#else
  data.clear();
#endif
  timestamp = 0;
  loaded = 0;

  loadMutex.init();
}

/*!
 *Destroy th object.
 */
HomeDir::~HomeDir()
{
  clear();
  loadMutex.destroy();
}

/*!
 *Clear the used memory.
 */
void HomeDir::clear()
{
#ifdef WIN32
  data.assign("");
#else
  HashMap<string, string*>::Iterator i = data.begin();
  for( ; i != data.end(); i++)
  {
    string* val = *i;
    if(val)
      delete val;
  }
  data.clear();
  timestamp = 0;
#endif
  loaded = 0;
}

/*!
 *Public wrapper to loadImpl.
 */
int HomeDir::load()
{
  int ret;
  loadMutex.lock();
  
  ret = loadImpl();

  loadMutex.unlock();
  return ret;
}

/*!
 *Load the internal buffer.
 */
int HomeDir::loadImpl()
{
#ifdef WIN32
  DWORD len = 64;
  char *buf;
  buf = new char[len];

  if(GetProfilesDirectory(buf, &len) == FALSE)
  {
    delete [] buf;
    buf = new char[len];
    if(GetProfilesDirectory(buf, &len) == FALSE)
    {
      delete [] buf; 
      return 1;                               
    }
  }
  data.assign(buf);
#else
  File usersFile;
  u_long size;
  char* buffer;
  u_long read;
  u_long counter;

  clear();

  if(usersFile.openFile("/etc/passwd", File::MYSERVER_OPEN_READ | 
                        File::MYSERVER_OPEN_IFEXISTS))
    return 1;
  size = usersFile.getFileSize();
  timestamp = usersFile.getLastModTime();
  if(size == (u_long) -1)
  {
    usersFile.close();
    return 1;
  }

  buffer = new char[size+1]; 

  usersFile.read(buffer, size, &read);
  buffer[read] = '\0';

 for(counter = 0; counter < read; counter++)
   if(buffer[counter] == ':')
     buffer[counter] = '\0';


  for(counter = 0; counter < size;)
  {
    char *username = 0;
    char *home = 0;
    string sUsername;
    string *sHome;
    string *old;
     
    if(buffer[counter] == '#')
      while(buffer[counter++] != '\n');

    if(counter >= size)
      break;

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

    while(buffer[counter++] != '\n');
    /* Next tuple.  */
  }
  delete [] buffer;
  usersFile.close();
#endif

  loaded = 1;
  return 0;
}

/*!
 *Get the home directory for a specified user.
 *\param userName The user name.
 */
const string *HomeDir::getHomeDir(string& userName)
{
#ifdef WIN32
  static string retString;
  retString.assign(data);
  retString.append("/");
  retString.append(userName);
  return &retString;
#else
  if(!loaded)
    return 0;
  /* TODO: don't check always but wait some time before.  */
  if(FilesUtility::getLastModTime("/etc/passwd") != timestamp)
    load();

  return data.get(userName);
#endif
}


/*!
 *Get the home directory for a specified user and write
 *it directly to the supplied buffer.
 *\param userName The user name.
 *\param out The buffer where write.
 */
void HomeDir::getHomeDir(string& userName, string& out)
{
#ifdef WIN32
  out.assign(data);
  out.append("/");
  out.append(userName);
#else
  static u_long lastCheckTime = 0;
  string *res = 0;
  if(!loaded)
    return;
  if(getTicks() - lastCheckTime > MYSERVER_SEC(1))
  {
    if(FilesUtility::getLastModTime("/etc/passwd") != timestamp)
      load();
    lastCheckTime = getTicks();
  }
  res = data.get(userName);
  if(res)
  {
    out.assign(*res);
  }
  else
    out.assign("");
#endif
}
