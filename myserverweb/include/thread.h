/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006 The MyServer Team
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

#ifndef THREAD_H
#define THREAD_H

#include "../stdafx.h"
#include "../include/file.h"
#include "../include/stringutils.h"

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
	static ThreadID threadID();
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
