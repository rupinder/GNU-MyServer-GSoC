/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007, 2008, 2009, 2010 Free Software
Foundation, Inc.
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


#include "myserver.h"
#include <include/base/utility.h>
#include <include/base/string/securestr.h>
extern "C"
{
#include <chdir-long.h>
}
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

#include <include/base/exceptions/checked.h>

#ifdef WIN32
# include <direct.h>
#endif

#include <iostream>
using namespace std;

static char *currentPath = NULL;
static size_t currentPathLen;

/*!
 * Returns the number of processors available on the local machine.
 */
u_long getCPUCount ()
{
  return num_processors (NPROC_CURRENT);
}

/*!
 * Save the current working directory.
 * Return -1 on fails.
 * Return 0 on success.
 */
static int initializeCwd ()
{
  if (currentPath)
    return 0;

  currentPath = checked::getcwd (NULL, 0);
  if (!currentPath)
    return -1;

  currentPathLen = strlen (currentPath) + 1;
  return 0;
}

/*!
 * Get the defult directory using a string as output.
 * Return 0 on success.
 * \param out The string where write.
 */
int getdefaultwd (string& out)
{
  char *wd = getdefaultwd (NULL, 0);
  if (wd == NULL)
    return -1;

  out.assign (wd);
  return 0;
}

/*!
 * Free the cwd buffer.
 */
int freecwd ()
{
  if (!currentPath)
    return 0;

  currentPathLen = 0;
  return 0;
}

/*!
 *Get the default working directory length.
 */
int getdefaultwdlen ()
{
  if (!currentPathLen && initializeCwd ())
    return 0;

  return currentPathLen;
}

/*!
 *Get the default working directory.
 *\param path The buffer where write.
 *\param len The length of the buffer.
 */
char *getdefaultwd (char *path, int len)
{
  if (!currentPath && initializeCwd ())
    return NULL;

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
  int ret;
  char *tmp = checked::strdup (dir);
  if (!tmp)
    return -1;

  ret = chdir_long (tmp);
  free (tmp);
  return ret;
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
  int ret = checked::gettimeofday (&tval, 0);
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
  union
  {
    struct cmsghdr cmh;
    char buffer[CMSG_SPACE (sizeof (int))];
  } cmh;
  char tbuf[4];
  struct iovec iov;
  int ret;
  struct cmsghdr *cmsg;

  memset (&mh, 0, sizeof (mh));
  mh.msg_iov = &iov;
  mh.msg_iovlen = 1;
  mh.msg_control = (caddr_t) &cmh;
  mh.msg_controllen = sizeof (cmh);
  iov.iov_base = tbuf;
  iov.iov_len = 4;

  ret = recvmsg (s, &mh, 0);
  if (ret < 0)
    return ret;

  cmsg = CMSG_FIRSTHDR (&mh);
  *fd = *((int *) CMSG_DATA (cmsg));
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
  union
  {
    struct cmsghdr cmh;
    char buffer[CMSG_SPACE (sizeof (int))];
  } cmh;
  struct cmsghdr *cmsg;
  char tbuf[4];
  struct iovec iov;
  iov.iov_base = tbuf;
  iov.iov_len = 4;

  memset (&mh, 0, sizeof (mh));
  mh.msg_iov = &iov;
  mh.msg_iovlen = 1;
  mh.msg_control = (caddr_t) &cmh;
  mh.msg_controllen = sizeof (cmh);

  cmsg = CMSG_FIRSTHDR (&mh);
  cmsg->cmsg_len = CMSG_LEN (sizeof (int));
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  *(int *)CMSG_DATA (cmsg) = fd;

  return sendmsg (s, &mh, 0);
#endif
}
