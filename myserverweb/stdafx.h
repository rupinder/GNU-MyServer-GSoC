/*
MyServer
Copyright (C) 2002, 2003, 2004, 2005 The MyServer Team
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

#include "config.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

/* If we are not windows, assume *nix */
#ifndef WIN32
#define NOT_WIN
#endif
extern "C" {
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#ifdef NOT_WIN
#include <limits.h>
#endif
}

#ifdef WIN32
extern "C"
{
#include <winsock2.h>
#include <tchar.h>
#include <process.h>
#include <io.h>
}
#endif


#ifndef MAX_PATH
#ifdef PATH_MAX
#define MAX_PATH PATH_MAX
#else
#define MAX_PATH 255
#endif
#endif

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

#ifndef MAXIMUM_PROCESSORS
#define MAXIMUM_PROCESSORS 255
#endif

#define USE_NEW

typedef unsigned long DWORD;
typedef unsigned long u_long;
typedef unsigned short u_short;

typedef void* HANDLE;
extern class Server *lserver;
extern const char *versionOfSoftware;
extern int mustEndServer;

#endif
