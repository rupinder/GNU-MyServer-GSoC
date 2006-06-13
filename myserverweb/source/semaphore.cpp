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
#include "../include/utility.h"
#include "../include/sockets.h"
#include "../include/semaphore.h"

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
 *Constructor for the semaphore class.
 */
Semaphore::Semaphore(int n)
{
	initialized = 0;
	init(n);
}

/*!
 *Initialize the semaphore.
 */
int Semaphore::init(int n)
{
  int ret = 0;
	if(initialized)
	{
		destroy();
		initialized = 0;
	}
#ifdef HAVE_PTHREAD
	ret = sem_init(&semaphore, 0, n);
#else
	semaphore = CreateSemaphore(0, 0, n, 0);
  ret = !semaphore;
#endif
	initialized = 1;
	return ret ? 1 : 0;
}

/*!
 *Destroy the semaphore.
 */
int Semaphore::destroy()
{
#ifdef HAVE_PTHREAD
  if(initialized)
    sem_destroy(&semaphore);
#else
  if(initialized)
    CloseHandle(semaphore);
#endif
	initialized = 0;
	return 0;
}

/*!
 *Lock the semaphore (p).
 */
int Semaphore::lock(u_long /*id*/)
{
#ifdef HAVE_PTHREAD
#ifdef PTHREAD_ALTERNATE_LOCK
	sem_wait(&semaphore);
#else
	while(sem_trywait(&semaphore))
	{
		Thread::wait(1);
	}
#endif

#else	
	WaitForSingleObject(semaphore, INFINITE);
#endif
	return 0;
}

/*!
*Unlock the semaphore access (n).
*/
int Semaphore::unlock(u_long/*! id*/)
{
#ifdef HAVE_PTHREAD
	int err;
	err = sem_post(&semaphore);
#else	
	ReleaseSemaphore(semaphore, 1, NULL);
#endif
	return 1;
}

/*!
*Destroy the object.
*/
Semaphore::~Semaphore()
{
	destroy();
}
