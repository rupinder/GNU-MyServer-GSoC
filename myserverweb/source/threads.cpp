/*
*MyServer
*Copyright (C) 2002 The MyServer Team
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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "../stdafx.h"
#include "../include/utility.h"
#include "../include/sockets.h"

extern "C" {
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#ifndef WIN32
#include <errno.h>
#include <getopt.h>
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
*Sleep the caller thread.
*/
void wait(u_long time)
{
#ifdef WIN32
	Sleep(time);
#endif
#ifdef NOT_WIN
	usleep(time);
#endif

}
/*!
*Initialize a mutex.
*/
int myserver_mutex::myserver_mutex_init()
{
#ifdef HAVE_PTHREAD
	pthread_mutex_init(&mutex, NULL);
#else
	mutex=CreateMutex(0,0,0);
#endif
	
	return 1;
}
/*!
*Destroy a mutex.
*/
int myserver_mutex::myserver_mutex_destroy()
{
#ifdef HAVE_PTHREAD
	pthread_mutex_destroy(&mutex);	
#else
	CloseHandle(mutex);
#endif
	return 1;
}


/*!
*Lock the mutex
*/
int myserver_mutex::myserver_mutex_lock(u_long id)
{
#ifdef HAVE_PTHREAD

#ifdef PTHREAD_ALTERNATE_LOCK	
	while(pthread_mutex_trylock(&mutex))
	{
		wait(1);
	}
#else
	pthread_mutex_lock(&mutex);
#endif	
		
#else	
	WaitForSingleObject(mutex,INFINITE);
#endif
	return 1;
}
/*!
*Unlock the mutex access.
*/
int myserver_mutex::myserver_mutex_unlock(u_long/*! id*/)
{
#ifdef HAVE_PTHREAD
	pthread_mutex_unlock(&mutex);
#else		
	ReleaseMutex(mutex);
#endif
	return 1;
}
