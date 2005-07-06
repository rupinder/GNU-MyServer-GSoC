/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef THREADS_H
#define THREADS_H

#include "../stdafx.h"
#include "../include/filemanager.h"
#include "../include/stringutils.h"

#ifdef HAVE_PTHREAD
	typedef pthread_mutex_t MutexHandle;
#else
	typedef HANDLE MutexHandle;
#endif

class Mutex
{
private:
	int initialized;
	MutexHandle mutex;
public:
	Mutex();
	~Mutex();
	int init();
	int destroy();
	int lock(u_long id=0);
	int unlock(u_long id=0);
};

#ifdef WIN32
	typedef unsigned int ThreadID;
#endif
#ifdef HAVE_PTHREAD
	typedef pthread_t ThreadID;
#endif

class Thread
{
public:
  static void wait(u_long);
#ifdef WIN32
	static int create(ThreadID*  thread, 
             unsigned int (_stdcall *start_routine)(void *), void * arg);
#endif
#ifdef HAVE_PTHREAD
	static int create(ThreadID*  thread, void * (*start_routine)(void *), 
                    void * arg);
#endif
	static void terminate();  
};
#endif
