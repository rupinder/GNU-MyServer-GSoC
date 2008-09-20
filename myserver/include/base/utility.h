/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006, 2008 Free Software Foundation, Inc.
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

#ifndef UTILITY_H
#define UTILITY_H

#include "stdafx.h"
#include <include/base/file/file.h>
#include <include/base/string/stringutils.h>
#include <include/base/process/process.h>
#include <include/base/thread/thread.h>
#include <include/base/sync/mutex.h>
#include <string.h>

using namespace std;

/*!
*Macros to do simple transformations.
*/
#define MYSERVER_KB(x) (x << 10)       //x * 1024
#define MYSERVER_MB(x) (x << 20)       //x * 1024 * 1024
#define MYSERVER_SEC(x) (x * 1000)
#define OS_WINDOWS_9X      1
#define OS_WINDOWS_2000    2
#define OS_WINDOWS_NT3     3
#define OS_WINDOWS_XP      4
#define OS_GNU_LINUX       100
#define OS_FREEBSD         200

int preparePrintError();
int endPrintError();

int getOSVersion();
u_long getCPUCount();
u_long getTicks();
int setcwdBuffer();
int getdefaultwdlen();
char *getdefaultwd(char* dwd,int maxlen);
int getdefaultwd(string&);
int setcwd(const char * cwd);
int freecwdBuffer();


#endif
