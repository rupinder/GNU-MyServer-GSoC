/*
MyServer
* strlcpy and strlcat by codingmaster
Copyright (C) 2002, 2003, 2004 The MyServer Team
Copyright (C) 2004 by codingmaster
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
#include "../include/securestr.h"
#include <string.h>

/*!
 *Secure string concatenate routine.
 */
unsigned int myserver_strlcat(char *destination, const char *source, unsigned int size)
{
	char *dstptr=destination;
	size_t dstlen,tocopy=size;
	const char *srcptr=source;
	
	while(tocopy-- && *dstptr)
	dstptr++;
    
	dstlen=dstptr-destination;
	
	tocopy=size-dstlen;
	if(!tocopy)
		return((int)(dstlen+strlen(source)));
    
	while(*srcptr)
	{
		if(tocopy!=1)
		{
			*dstptr++=*srcptr;
			tocopy--;
		}
		srcptr++;
	}
	
	*dstptr=0;
	
	return((int)(dstlen+(srcptr-source)));
}

/*!
 *Secure string copy routine.
 */   
unsigned int myserver_strlcpy(char *destination, const char *source, unsigned int size)
{
	char *dstptr=destination;
	size_t tocopy=size;
	const char *srcptr=source;
	
	if(tocopy && --tocopy)
	{
		do
		{
			if(!(*dstptr++=*srcptr++))
				break;
		}
	
		while(--tocopy);
	}
	
	if(!tocopy)
	{
		if(size)
			*dstptr=0;
		
		while(*srcptr++);
	}
	
	return((int)(srcptr-source-1));
}
