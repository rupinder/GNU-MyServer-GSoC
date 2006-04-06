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


#include "../stdafx.h"
#include "../include/utility.h"
#include "../include/pipe.h"

#ifdef NOT_WIN
extern "C" {
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
}
#else
#include <windows.h>
#endif

#include <string>
#include <sstream>

using namespace std;

/*!
 *Read from the pipe.
 *\param buffer Where write.
 *\param len Buffer size.
 *\param nbr Get how many bytes were read.
 */
int Pipe::read(char* buffer, u_long len, u_long *nbr)
{
#ifdef NOT_WIN
	int ret = ::read(handles[0], buffer, len);
	if(ret == -1)
		return 1;
	else
		*nbr = (u_long)ret;
#else
 return !ReadFile(readHandle, buffer, len, nbr, NULL);
#endif
	return 0;
}


/*!
 *Create the pipe descriptors.  Return 0 on success.
 */
int Pipe::create()
{
#ifdef NOT_WIN
	return pipe(handles);
#else
  return !CreatePipe(&readHandle, &writeHandle, NULL, 0);
#endif
}

/*!
 *Write from the pipe.
 *\param buffer What write.
 *\param len Buffer size.
 *\param nbr Get how many bytes were really written.
 */
int Pipe::write(const char* buffer, u_long len, u_long *nbw)
{
#ifdef NOT_WIN
	int ret = ::write(handles[1], buffer, len);
	if(ret == -1)
		return 1;
	else
		*nbw = (u_long) ret;
#else
  return !WriteFile(writeHandle, buffer, len, nbw, NULL);
#endif
	return 0;
}

/*!
 *Get the handle to use to read from the pipe.
 */
long Pipe::getReadHandle()
{
#ifdef NOT_WIN
	return handles[0];
#else
  return (long)readHandle;
#endif
}

/*!
 *Get the handle used to write to the pipe.
 */
long Pipe::getWriteHandle()
{
#ifdef NOT_WIN
	return handles[1];
#else
  return (long)writeHandle;
#endif
}


/*!
 *Close the pipe.
 */
void Pipe::close()
{
#ifdef NOT_WIN
	if(handles[0])
		::close(handles[0]);
	if(handles[1])
		::close(handles[1]);
  handles[0] = handles[1] = 0;
#else
  if(readHandle)
    CloseHandle(readHandle);
  if(writeHandle)
    CloseHandle(writeHandle);
  readHandle = writeHandle = 0;
#endif
}

/*!
 *Invert the current pipe on another instance of the class.
 *The input will be used as output and viceversa.
 *\param pipe The pipe where write.
 */
void Pipe::inverted(Pipe& pipe)
{
#ifdef NOT_WIN
	pipe.handles[0] = handles[1];
	pipe.handles[1] = handles[0];
#else
  pipe.readHandle = writeHandle;
  pipe.writeHandle = readHandle;
#endif
}

Pipe::Pipe()
{
#ifdef NOT_WIN
	handles[0] = handles[1] = 0;
#else
  readHandle = writeHandle = 0;
#endif
}

/*!
 *Close the read stream of the pipe.
 */
void Pipe::closeRead()
{
#ifdef NOT_WIN
	if(handles[0])
		::close(handles[0]);
  handles[0] = 0;
#else
  if(readHandle)
    CloseHandle(readHandle);
  readHandle = 0;
#endif

}

/*!
 *Close the write stream of the pipe.
 */
void Pipe::closeWrite()
{
#ifdef NOT_WIN
	if(handles[1])
		::close(handles[1]);
	handles[1] = 0;
#else
  if(writeHandle)
    CloseHandle(writeHandle);
    writeHandle = 0;
#endif
}
