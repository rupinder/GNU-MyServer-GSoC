/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2005, 2007, 2008, 2009, 2010 Free Software Foundation, Inc.
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

#ifndef STDAFX_H
# define STDAFX_H

# include "config.h"

# ifdef HAVE_PTHREAD
#  include <pthread.h>
# endif

# include <stdlib.h>
# include <stdio.h>
# include <fcntl.h>
# include <math.h>
# include "time.h"
# include <ctype.h>
# include <stdint.h>

# ifdef WIN32
#  include <tchar.h>
#  include <process.h>
# else
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <limits.h>
# endif


typedef void* HANDLE;

#endif

#ifdef HAVE_GETTEXT

/*XXX: Fix me.  */
# undef sprintf
# undef snprintf

# include <libintl.h>

# define _(X) gettext (X)

#else
# define _(X) X
#endif

/*
  Automatically append an exception to the formatting string.
  Don't forget to pass as last parameter a pointer to an exception.
 */
#define _E(X) _(X " : %e")
