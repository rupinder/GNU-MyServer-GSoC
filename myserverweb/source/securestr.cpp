/*
*MyServer
*Copyright (C) 2002 The MyServer Team
*
*   strlcpy and strlcat by codingmaster
*
*   (c)2004 by codingmaster
*
*
*
*
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
#include "../include/securestr.h"
#include <string.h>

unsigned int myserver_strlcat(char *destination, const char *source, size_t size)
{
    char *dstptr=destination;
    size_t dstlen,tocopy=size;
    const char *srcptr=source;
    
    while(tocopy-- && *dstptr)
    	dstptr++;
    
    dstlen=dstptr-destination;
    
    if(!(tocopy=size-dstlen))
    	return(dstlen+strlen(source));
    
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
    
    return(dstlen+(srcptr-source));
}

   
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
    
    return(srcptr-source-1);
}
