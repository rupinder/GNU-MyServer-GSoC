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
	typedef pthread_mutex_t  myserver_mutex;
#else
	typedef (void*) myserver_mutex;
#endif


void wait(u_long);
int	myserver_mutex_init(myserver_mutex*);
int myserver_mutex_destroy(myserver_mutex*);
/*!
*These functions are a simple trasposition of the mutex mechanism.
*/
int myserver_mutex_lock(myserver_mutex*,u_long id=0);
int myserver_mutex_unlock(myserver_mutex*,u_long id=0);

#endif
