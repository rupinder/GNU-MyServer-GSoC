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

#ifndef THREADS_H
#define THREADS_H

#include "../stdafx.h"
#include "../include/filemanager.h"
#include "../include/stringutils.h"

#ifdef HAVE_PTHREAD
	typedef pthread_mutex_t myserver_mutex_handle;
#else
	typedef HANDLE myserver_mutex_handle;
#endif

void wait(u_long);
class myserver_mutex
{
private:
	myserver_mutex_handle mutex;
public:
	int myserver_mutex_init();
	int myserver_mutex_destroy();
	/*!
	*These functions are a simple trasposition of the mutex mechanism.
	*/
	int myserver_mutex_lock(u_long id=0);
	int myserver_mutex_unlock(u_long id=0);
};
#ifdef WIN32
	typedef unsigned int myserver_thread_ID;
#endif
#ifdef HAVE_PTHREAD
	typedef pthread_t myserver_thread_ID;
#endif

class myserver_thread
{
public:
	static int  create(myserver_thread_ID*  thread, void * (*start_routine)(void *), void * arg);
	static void   terminate();  
};
#endif
