/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007, 2008, 2009 Free Software Foundation, Inc.
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
#include <include/base/utility.h>
#include <include/base/string/securestr.h>

extern "C"
{
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "nproc.h"
#ifndef WIN32
# include <unistd.h>
# include <signal.h>
# include <sys/types.h>
# include <sys/wait.h>
# include <sys/time.h>
# include <errno.h>

# ifdef ERRORH
#  include <error.h>
# endif
#endif
}

#ifdef WIN32
# include <direct.h>
#endif

#include <iostream>
using namespace std;

/*
 * Various utility functions.
 */
static char *currentPath = 0;

/*!
 * Returns the number of processors available on the local machine.
 */
u_long getCPUCount ()
{
  return num_processors ();
}

/*!
 * Save the current working directory.
 * Return -1 on fails.
 * Return 0 on success.
 */
int setcwdBuffer ()
{
#ifdef WIN32
  /* Under windows there is MAX_PATH, we will use it.  */
  currentPath = new char [MAX_PATH];
  if (currentPath == 0)
    return (-1);
  char* ret =(char*) _getcwd (currentPath,MAX_PATH);
  if (ret == 0)
    return -1;

  ret = 0;

  for (u_long i = 0; i<(u_long)strlen (currentPath); i++)
    if (currentPath[i] == '\\')
      currentPath[i] = '/';

  if (currentPath[strlen (currentPath)] == '/')
    currentPath[strlen (currentPath)] = '\0';
  return 0;
#endif

#ifdef __OpenBSD__
  currentPath = getcwd (0, 0);
  return currentPath ? 0 : 1;
#endif

#ifndef WIN32
  int size = 16;
  char *ret = 0;
  currentPath = new char[size];
  do
  {
    /*! Allocation problem is up.  */
    if (currentPath == 0)
      return -1;

    ret = getcwd (currentPath, size);
    /* Realloc the buffer if it cannot contain the current directory.  */
    if (ret == 0)
      {
        size *= 2;
        delete [] currentPath;
        currentPath = new char[size];
      }
  }
  while ((ret == 0) && (errno == ERANGE));

  if (currentPath[strlen (currentPath)] == '/')
    currentPath[strlen (currentPath)] = '\0';
  return 0;
#endif
}

/*!
 * Get the defult directory using a string as output.
 * Return 0 on success.
 * \param out The string where write.
 */
int getdefaultwd (string& out)
{
  char *wd = getdefaultwd (NULL, NULL);

  if (wd == 0)
    return -1;

  out.assign (wd);
  return 0;
}

/*!
 * Free the cwd buffer.
 */
int freecwdBuffer ()
{
  delete [] currentPath;
  return 0;
}

/*!
 *Get the default working directory length.
 */
int getdefaultwdlen ()
{
  return strlen (currentPath) + 1;
}

/*!
 *Get the default working directory (Where is the main executable).
 *\param path The buffer where write.
 *\param len The length of the buffer.
 */
char *getdefaultwd (char *path, int len)
{
  if (path)
    {
    /* If len is equal to zero we assume no limit.  */
      if (len)
        myserver_strlcpy (path, currentPath, len);
      else
        strcpy (path, currentPath);
    }
  return currentPath;
}

/*!
 * Set the current working directory. Returns Zero if successful.
 * \param dir The current working directory.
 */
int setcwd (const char *dir)
{
#ifdef WIN32
  return _chdir (dir);
#endif

#ifndef WIN32
  return chdir (dir);
#endif
}

/*!
 * Set the text color to red on black.
 * Return 0 on success.
 */
int preparePrintError ()
{
#ifndef WIN32
  cout << "\033[31;1m";
  return 0;
#else
  int ret = SetConsoleTextAttribute (GetStdHandle (STD_OUTPUT_HANDLE),
                                     FOREGROUND_RED | FOREGROUND_INTENSITY);
  if (ret)
    return 0;
#endif

  return -1;
}

/*!
 * Set the text color to white on black.
 * Return 0 on success.
 */
int endPrintError ()
{
#ifndef WIN32
  cout << "\033[0m";
  return 0;
#else
  int ret = SetConsoleTextAttribute (GetStdHandle (STD_OUTPUT_HANDLE),
                                     FOREGROUND_RED |
                                     FOREGROUND_GREEN |
                                     FOREGROUND_BLUE);
  if (ret)
    return 0;
#endif

  return -1;
}

/*!
 * Return the ticks count in milliseconds.
 * Return 0 on errors.
 */
u_long getTicks ()
{
#ifdef WIN32
  return GetTickCount ();
#else
  struct timeval tval;
  int ret = gettimeofday (&tval, 0);
  if (ret == -1)
    return 0;
  return  (tval.tv_sec * 1000) + (tval.tv_usec / 1000);
#endif
}

/*!
 * Read a file handle from a socket.
 * \param s Socket handle to read from.
 * \param File handle received.
 * \return 0 on success.
 */
int readFileHandle (SocketHandle s, Handle* fd)
{
#ifdef WIN32
  return -1;
#else
  struct msghdr mh;
  struct cmsghdr cmh[2];
  struct iovec iov;
  int ret;
  char tbuf[4];
  mh.msg_name = 0;
  mh.msg_namelen = 0;
  mh.msg_iov = &iov;
  mh.msg_iovlen = 1;
  mh.msg_control = (caddr_t)&cmh[0];
  mh.msg_controllen = sizeof (cmh[0]) * 2;
  iov.iov_base = tbuf;
  iov.iov_len = 4;
  cmh[0].cmsg_len = sizeof (cmh[0]) + sizeof (int);
  ret = recvmsg (s, &mh, 0);

  if (ret < 0)
    return ret;

  *fd = *(FileHandle *)&cmh[1];

  return 0;
#endif
}

/*!
 * Write a file handle to a socket.
 * \param s Socket handle to write to.
 * \param File handle received.
 * \return 0 on success.
 */
int writeFileHandle (SocketHandle s, Handle fd)
{
#ifdef WIN32
  return -1;
#else
  struct msghdr mh;
  struct cmsghdr cmh[2];
  struct iovec iov;
  char tbuf[4];
  memset (&mh,0,sizeof (mh));
  mh.msg_name = 0;
  mh.msg_namelen = 0;
  mh.msg_iov = &iov;
  mh.msg_iovlen = 1;
  mh.msg_control = (caddr_t)&cmh[0];
  mh.msg_controllen = sizeof (cmh[0]) + sizeof (int);
  mh.msg_flags = 0;
  iov.iov_base = tbuf;
  iov.iov_len = 4;
  cmh[0].cmsg_level = SOL_SOCKET;
  cmh[0].cmsg_type = SCM_RIGHTS;
  cmh[0].cmsg_len = sizeof (cmh[0]) + sizeof (int);
  *(int *)&cmh[1] = fd;
  return sendmsg (s, &mh, 0);
#endif
}
