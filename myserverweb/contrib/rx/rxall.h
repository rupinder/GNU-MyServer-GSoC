/* classes: h_files */

#ifndef RXALLH
#define RXALLH
/*	Copyright (C) 1995, 1996 Tom Lord
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, 59 Temple Place - Suite 330, 
 * Boston, MA 02110-1301, USA. 
 */
#ifndef __STDC__
#define __STDC__
#endif


#ifdef WIN32

#include <stdlib.h>
#include <string.h>
#include "rx.h"


#include <ctype.h>
extern int sscanf(const char *buffer, const char *format,...);

#endif

#include <stdlib.h>

#ifdef __STDC__

#else /* STDC */

#endif /* STDC */



#endif  /* RXALLH */

