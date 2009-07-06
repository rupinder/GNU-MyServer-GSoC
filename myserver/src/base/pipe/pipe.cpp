/*
MyServer
Copyright (C) 2002, 2003, 2004, 2005, 2008, 2009 Free Software Foundation, Inc.
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


#include "stdafx.h"
#include <include/base/utility.h>
#include <include/base/pipe/pipe.h>

#ifndef WIN32
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
  *nbr = 0;
#ifndef WIN32
  int ret = ::read(handles[0], buffer, len);
  if(ret == -1)
  {
    terminated = true;
    return 1;
  }
  else if(!ret)
  {
    terminated = true;
    return 0;
  }
  else
  {
    *nbr = (u_long)ret;
  }
  return 0;
#else
  if( !ReadFile(readHandle, buffer, len, nbr, NULL) || !nbr)
  {
    *nbr = 0;
    if(GetLastError() != ERROR_BROKEN_PIPE)
      return 1;
    else
    {
      terminated = true;
      return 0;
    }
  }
  return 0;
#endif

  return 0;
}


/*!
 *Create the pipe descriptors.  Return 0 on success.
 *\param readPipe Specify if the current process uses it for read.  A false
 *value means the process uses it to write.
 */
int Pipe::create(bool readPipe)
{
#ifndef WIN32

#ifdef HAVE_PIPE
  return pipe (handles);
#else
  return 1;
#endif

#else
  HANDLE tmp;
  SECURITY_ATTRIBUTES sa;
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = NULL;
  if(!CreatePipe(&readHandle, &writeHandle, &sa, 0))
    return 1;
    
  if(readPipe)
  {
    if(!DuplicateHandle(GetCurrentProcess(), readHandle, 
                        GetCurrentProcess(), &tmp, 0, 
                        FALSE, DUPLICATE_SAME_ACCESS))
    {
      close();
      return 1;                    
    }
    CloseHandle(readHandle);
    readHandle = tmp;
  }
  else
  {
    if(!DuplicateHandle(GetCurrentProcess(), writeHandle, 
                        GetCurrentProcess(), &tmp, 0, 
                        FALSE, DUPLICATE_SAME_ACCESS))
    {
      close();
      return 1;                    
    }
    CloseHandle(writeHandle);
    writeHandle = tmp;
  }
  return 0;
#endif
}

/*!
 *Write from the pipe.
 *\param buffer What write.
 *\param len Buffer size.
 *\param nbw Get how many bytes were written.
 */
int Pipe::write(const char* buffer, u_long len, u_long *nbw)
{
  *nbw = 0;
#ifndef WIN32
  int ret = ::write(handles[1], buffer, len);
  if(ret == -1)
  {
    terminated = true;
    return 1;
  }
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
#ifndef WIN32
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
#ifndef WIN32
  return handles[1];
#else
  return (long)writeHandle;
#endif
}


/*!
 *Close the pipe.
 */
int Pipe::close()
{
  terminated = true;
#ifndef WIN32
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
  return 0;
}

/*!
 *Invert the current pipe on another instance of the class.
 *The input will be used as output and viceversa.
 *\param pipe The pipe where write.
 */
void Pipe::inverted(Pipe& pipe)
{
#ifndef WIN32
  handles[0] = pipe.handles[1];
  handles[1] = pipe.handles[0];
#else
  pipe.readHandle = writeHandle;
  pipe.writeHandle = readHandle;
#endif
}

Pipe::Pipe()
{
  terminated = false;
#ifndef WIN32
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
  terminated = true;
#ifndef WIN32
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
  terminated = true;
#ifndef WIN32
  if(handles[1])
    ::close(handles[1]);
  handles[1] = 0;
#else
  if(writeHandle)
    CloseHandle(writeHandle);
    writeHandle = 0;
#endif
}

/*!
 *Wait until new data is ready.  Do not wait more 
 *than the specified timeout.
 *\param sec Seconds part of the timeout.
 *\param usec Micro seconds part of the timeout.
 */
int Pipe::waitForData (int sec, int usec)
{
#ifdef WIN32
  /* FIXME: it seems to don't work properly.  */
  return WaitForSingleObject (readHandle, sec * 1000 + usec / 1000)
    == WAIT_OBJECT_0;
#elif HAVE_PIPE
  struct timeval tv;
  fd_set readfds;
  int ret;
  tv.tv_sec = sec;
  tv.tv_usec = usec;

  FD_ZERO(&readfds);

  FD_SET(handles[0], &readfds);

  ret = ::select(handles[0] + 1, &readfds, NULL, NULL, &tv);

  if(ret == -1 || ret == 0)
  {
    return 0;
  }

  if (FD_ISSET(handles[0], &readfds))
    return 1;

  return 0;
#endif

  return 0;
}
