/*
*myServer
*Copyright (C) Rocky_10_Balboa
*This library is free software; you can redistribute it and/or
*modify it under the terms of the GNU Library General Public
*License as published by the Free Software Foundation; either
*version 2 of the License, or (at your option) any later version.

*This library is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*Library General Public License for more details.

*You should have received a copy of the GNU Library General Public
*License along with this library; if not, write to the
*Free Software Foundation, Inc., 59 Temple Place - Suite 330,
*Boston, MA  02111-1307, USA.
*/
#pragma once

#ifdef WIN32
#	undef getTime
#	undef max
#	undef min
#endif

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <tchar.h>

#include <math.h>
#include <process.h>

#include <winsock2.h>
#include <time.h>
#include <io.h>

#include "resource.h"

#include "include\utility.h"
#include "include\HTTPmsg.h"
extern class cserver *lserver;

#include "include\Response_RequestStructs.h"
#include "include\ConnectionStruct.h"


/*
*Set MAX_MIME_TYPES to define the maximum
*number of MIME TYPES records to alloc
*/
#include "include\MIME_manager.h"
#include "include\AMMimeUtils.h"

