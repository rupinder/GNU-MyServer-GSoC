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
int requestAccess(u_long* ac,u_long id)
{
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
			requestAccess(ac,id);
			return 0;
		}
	}
	/*!
	*Wait until another thread ends the access, then set our access.
	*/
	while(*ac);


	*ac=id;
	requestAccess(ac,id);
	return 0;
}
/*!
*Reset the owner of the access.
*/
int terminateAccess(u_long* ac,u_long/*! id*/)
{
	/*!
	*Only set to Zero the owner of the access.
	*/
	*ac=0;
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
