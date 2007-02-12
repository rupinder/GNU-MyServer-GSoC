/*
MyServer
Copyright (C) 2007 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License,  or
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
#include "../include/safetime.h"
#include "../include/mutex.h"

extern "C" {
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef NOT_WIN
#include <stdio.h>
#endif
}

#ifndef WIN32
#if !HAVE_LOCALTIME_R
static Mutex mutex;
#endif
#endif

/*!
 *Initialize the localtime function.
 */
void myserver_safetime_init()
{
#ifndef WIN32
#if !HAVE_LOCALTIME_R
	mutex.init();
#endif
#endif
}

/*!
 *Clean all the data used by the localtime function.
 */
void myserver_safetime_destroy()
{
#ifndef WIN32
#if !HAVE_LOCALTIME_R
	mutex.destroy();
#endif
#endif
}

/*!
 *Thread-safe wrap function for localtime.
 */
struct tm *myserver_localtime(const time_t *timep, tm* res)
{
#ifdef WIN32
	/* The localtime function under WIN32 is thread-safe.  */
	return localtime(timep);

#elif HAVE_LOCALTIME_R
	return localtime_r(timep, res);
#else

	mutex.lock();
	memcpy(res, localtime(timep), sizeof(tm));
	mutex.unlock();

	return res;
#endif
}

/*!
 *Thread-safe wrap function for gmtime.
 */
struct tm *myserver_gmtime(const time_t *timep, tm* res)
{
#ifdef WIN32
	/* The gmtime function under WIN32 is thread-safe.  */
	return gmtime(timep);

#elif HAVE_LOCALTIME_R
	return gmtime_r(timep, res);
#else

	mutex.lock();
	memcpy(res, gmtime(timep), sizeof(tm));
	mutex.unlock();

	return res;
#endif
}
