/*
MyServer
Copyright (C) 2002, 2003, 2004, 2008, 2009, 2010 Free Software
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
#include <include/base/sync/recursive_mutex.h>

extern "C" {
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#ifndef WIN32
# include <errno.h>
# include <netdb.h>
# include <unistd.h>
# include <signal.h>
# ifdef HAVE_PTHREAD
#  include <pthread.h>
# endif
# include <sys/wait.h>
#endif
}

#include <sys/types.h>

#ifdef WIN32
# include <direct.h>
#endif

/*!
 *Constructor for the recursive mutex class.
 */
RecursiveMutex::RecursiveMutex () : Mutex ()
{
  initialized = 0;
  init ();
}

/*!
 *Initialize a recursive mutex.
 */
int RecursiveMutex::init ()
{
  int ret = 0;
  if (initialized)
  {
    destroy ();
    initialized = 0;
  }
#ifdef HAVE_PTHREAD
  pthread_mutexattr_t mta;
  pthread_mutexattr_init (&mta);
  pthread_mutexattr_settype (&mta, PTHREAD_MUTEX_RECURSIVE);
  ret = pthread_mutex_init (&mutex, &mta);
#else
  return Mutex::init ();
#endif

  initialized = 1;
  return ret ? 1 : 0;
}

/*!
*Destroy the object.
*/
RecursiveMutex::~RecursiveMutex ()
{
  destroy ();
}
