/*
MyServer
Copyright (C) 2006, 2007, 2008, 2009, 2010 Free Software Foundation,
Inc.
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
#include <include/base/sync/event.h>

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
#  include <time.h>
#  include <sys/time.h>
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
 *Constructor for the event class.
 *\param broadcast If true a signal will be sent to all the waiting threads.
 */
Event::Event (bool broadcast)
{
  initialized = false;
  init (broadcast);
}

/*!
 *Initialize an event.
 *\param broadcast If true a signal will be sent to all the waiting threads.
 *\return nonzero on errors.
 */
int Event::init (bool broadcast)
{
  int ret = 0;
  if (initialized)
  {
    destroy ();
    initialized = false;
  }
  this->broadcast = broadcast;
#ifdef HAVE_PTHREAD
  ret = pthread_mutex_init (&mutex,(pthread_mutexattr_t*) NULL);
  if (!ret)
    ret = pthread_cond_init (&event, NULL);
#else
  event = CreateEvent (NULL, broadcast ? FALSE : TRUE, FALSE, NULL);
  ret = !event;
#endif
  initialized = true;
  return ret ? 1 : 0;
}

/*!
 *Wait for the event.
 *\param id The calling thread id.
 *\param timeout Timeout value in milliseconds.
 */
int Event::wait (u_long id, u_long timeout)
{
  if (!initialized)
    return -1;
#ifdef HAVE_PTHREAD
  int ret;
  if (timeout)
  {
    struct timespec ts;
    struct timeval tp;
    gettimeofday (&tp, NULL);
    ts.tv_sec = tp.tv_sec + tp.tv_usec / 1000000 + timeout / 1000;
    ts.tv_nsec = (tp.tv_usec * 1000 + timeout * 1000000) % 1000000000;

    pthread_mutex_lock (&mutex);
    ret = pthread_cond_timedwait (&event, &mutex, &ts);
    pthread_mutex_unlock (&mutex);

  }
  else
  {
    pthread_mutex_lock (&mutex);
    ret = pthread_cond_wait (&event, &mutex);
    pthread_mutex_unlock (&mutex);

  }
  if (ret == ETIMEDOUT)
    ret = 1;

  return ret;
#else
  DWORD32 ret = WaitForSingleObject (event, timeout ? timeout : INFINITE);
  if (ret == WAIT_TIMEOUT)
    return 1;
  else if (ret == WAIT_OBJECT_0)
    return 0;

  return -1;
#endif

}

/*!
 *Signal the event.
 *The behaviour of this message is influenced by the broadcast flag, if this
 *is a broadcast event then the event will be signaled to all the waiting
 *threads otherwise only one thread will be signaled.
 *\param id The calling thread id.
 */
int Event::signal (u_long id)
{
  if (!initialized)
    return -1;

#ifdef HAVE_PTHREAD
  if (broadcast)
  {
    pthread_mutex_lock (&mutex);
    pthread_cond_broadcast (&event);
    pthread_mutex_unlock (&mutex);
  }
  else
  {
    pthread_mutex_lock (&mutex);
    pthread_cond_signal (&event);
    pthread_mutex_unlock (&mutex);
  }
#else
  SetEvent (event);

  /* Reset the event for later reusability.  */
  ResetEvent (event);

#endif
  return 0;
}


/*!
 *Clean all the used resources.
 */
int Event::destroy ()
{
#ifdef HAVE_PTHREAD
  if (initialized)
  {
    pthread_mutex_lock (&mutex);
    initialized = false;
    pthread_cond_broadcast (&event);
    pthread_mutex_unlock (&mutex);

    pthread_cond_destroy (&event);
    pthread_mutex_destroy (&mutex);
  }
#else
  if (initialized)
    CloseHandle (event);
#endif
  initialized = false;
  return 0;
}

/*!
 *Destroy the object.
 */
Event::~Event ()
{
  destroy ();
}
