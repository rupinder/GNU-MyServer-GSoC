/*
MyServer
Copyright (C) 2002, 2003, 2004, 2008 Free Software Foundation, Inc.
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


#include "../stdafx.h"
#include "../include/utility.h"
#include "../include/mutex.h"

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
 *Constructor for the mutex class.
 */
Mutex::Mutex()
{
  initialized = 0;
  init();
}
/*!
 *Initialize a mutex.
 */
int Mutex::init()
{
  int ret = 0;
  if(initialized)
  {
    destroy();
    initialized = 0;
  }
#ifdef HAVE_PTHREAD


#if 0
  pthread_mutexattr_t mta;
  pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_NORMAL);
  ret = pthread_mutex_init(&mutex, &mta);
#else
  ret = pthread_mutex_init(&mutex,(pthread_mutexattr_t*) NULL);
#endif


#else
  mutex = CreateMutex(0, 0, 0);
  ret = !mutex;
#endif
  initialized = 1;
  return ret ? 1 : 0;
}

/*!
 *Destroy a mutex.
 */
int Mutex::destroy()
{
#ifdef HAVE_PTHREAD
  if(initialized)
    pthread_mutex_destroy(&mutex);
#else
  if(initialized)
    CloseHandle(mutex);
#endif
  initialized = 0;
  return 0;
}

/*!
 *Lock the mutex.
 */
int Mutex::lock(u_long /*id*/)
{
#ifdef HAVE_PTHREAD
#ifdef PTHREAD_ALTERNATE_LOCK
  pthread_mutex_lock(&mutex);
#else
  while(pthread_mutex_trylock(&mutex) == EBUSY)
  {
    Thread::wait(1);
  }
#endif

#else  
  WaitForSingleObject(mutex, INFINITE);
#endif
  locked = true;
  return 0;
}

/*!
 *Check if the mutex is already locked. 
 *
 *\return Return true if the mutex is currently locked.
 */
bool Mutex::isLocked()
{
  return locked;
}

/*!
*Unlock the mutex access.
*/
int Mutex::unlock(u_long/*! id*/)
{
#ifdef HAVE_PTHREAD
  int err;
  err = pthread_mutex_unlock(&mutex);
#else  
  ReleaseMutex(mutex);
#endif
  locked = false;
  return 0;
}

/*!
*Destroy the object.
*/
Mutex::~Mutex()
{
  destroy();
}

/*!
 *Get the handle for the mutex.
 */
MutexHandle Mutex::getHandle()
{
  return mutex;
}
