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
#define STDAFX_H

#ifndef WIN32
# include "config.h"
#endif

#ifdef HAVE_CONFIG_H
# include <cstdlib>
# include "time.h"
# include <config.h>
#endif

#ifdef HAVE_PTHREAD
# include <pthread.h>
#endif

#ifdef WIN32
extern "C"
{
# include <winsock2.h>
# include <tchar.h>
# include <process.h>
}
#endif

extern "C"
{
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <math.h>
#include "time.h"
#include <ctype.h>
#include <stdint.h>

#ifndef WIN32
# include <sys/types.h>
# include <sys/stat.h>
# include <limits.h>
#endif
}

typedef void* HANDLE;

#ifndef SOCKET_ERROR
# define SOCKET_ERROR -1
#endif

#ifndef INVALID_SOCKET
# define INVALID_SOCKET -1
#endif

#endif

#ifdef HAVE_GETTEXT
# include <libintl.h>
# define _(X) gettext (X)
#else
# define _(X) X
#endif
