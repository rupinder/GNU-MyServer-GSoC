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
#include <stdio.h>
#ifndef WIN32
#include <unistd.h>
#include <signal.h>
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include <sys/types.h>
#include <sys/wait.h>
#endif
}

#ifdef WIN32
#include <direct.h>
#endif


/*!
*This function is similar to the Windows API WaitForSingleObject(..)
*/
int myserver_mutex_lock(myserver_mutex* ac,u_long id)
{
#ifdef HAVE_PTHREAD
	pthread_mutex_lock(ac);
#else	
	/*!
	*If the access ID is equal to the thread ID we don't do nothing.
	*/
	if(*ac==id)
		return 0;
	/*!
	*if the access doesn't belong to any thread then set that it belongs to the caller thread
	*and check if we have the access now.
	*/
	do
	{
		if(*ac==0)
		{
			*ac=id;
			myserver_mutex_lock(ac,id);
			return 0;
		}
	}
	/*!
	*Wait until another thread ends the access, then set our access.
	*/
	while(*ac);


	*ac=id;
	myserver_mutex_lock(ac,id);
#endif
	return 0;
}
/*!
*Reset the owner of the access.
*/
int myserver_mutex_unlock(myserver_mutex* ac,u_long/*! id*/)
{
#ifdef HAVE_PTHREAD
	pthread_mutex_unlock(ac);
#else		
	/*!
	*Only set to Zero the owner of the access.
	*/
	*ac=0;
#endif
	return 0;
}

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
int	myserver_mutex_init(myserver_mutex* mutex)
{
#ifdef HAVE_PTHREAD
	pthread_mutex_init(mutex, NULL);
#else
	*mutex=0;
#endif
	
	return 1;
}
/*!
*Destroy a mutex.
*/
int myserver_mutex_destroy(myserver_mutex *mutex)
{
#ifdef HAVE_PTHREAD
	pthread_mutex_destroy(mutex);	
#endif
	return 1;
}
