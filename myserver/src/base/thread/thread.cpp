/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008, 2009 Free Software Foundation, Inc.
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
#include <include/base/thread/thread.h>

extern "C"
{
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
 * Sleep the caller thread for [TIME] microseconds.
 */
void Thread::wait (u_long time)
{
#ifdef WIN32
  Sleep (time / 1000);
#else
  usleep (time);
#endif

}

/*!
 *Create a new thread.
 *\param ID Pointer to ThreadID to receive the new thread identifier.
 *\param startRoutine Start routine for the new thread.
 *\param arg Argument to pass to the new created thread.
 */
#ifdef WIN32
int Thread::create (ThreadID*  ID,
                    unsigned int  (_stdcall *startRoutine)(void *),
                    void * arg)
#endif
#ifdef HAVE_PTHREAD
int Thread::create (ThreadID*  ID, void * (*startRoutine)(void *),
                    void * arg)
#endif
{
#ifdef WIN32
  *ID = _beginthreadex (NULL, 0, startRoutine, arg, 0, NULL);

  return !(*ID);
#endif
#ifdef HAVE_PTHREAD
  return pthread_create ((pthread_t*)ID, NULL, startRoutine, (void *)(arg));
#endif
}

/*!
 *Get the calling thread ID.
 */
ThreadID Thread::threadID ()
{
#ifdef WIN32
  return GetCurrentThreadId ();
#endif
#ifdef HAVE_PTHREAD
  return pthread_self ();
#endif
}

/*!
 *Terminate the caller thread.
 */
void Thread::terminate ()
{
#ifdef WIN32
  _endthread ();
#endif
#ifdef HAVE_PTHREAD
  pthread_exit (0);
#endif
}

/*!
 *Sleep until the thread identified by tid complete its execution.
 */
int Thread::join (ThreadID tid)
{
#ifdef WIN32
  return WaitForSingleObject ((void*)tid, INFINITE) != WAIT_OBJECT_0;
#endif
#ifdef HAVE_PTHREAD
  return pthread_join (tid, NULL);
#endif
}
