/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2005, 2008, 2009, 2010 Free Software
  Foundation, Inc.
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


#include "myserver.h"
#include <include/base/utility.h>
#include <include/base/pipe/pipe.h>

# include <fcntl.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <errno.h>
# include <stdio.h>
# include <fcntl.h>
# include <stdlib.h>
# include <string.h>
# include <math.h>
# include <time.h>

#include <string>
#include <sstream>


#include <include/base/exceptions/checked.h>

using namespace std;

/*!
  Read from the pipe.
  \param buffer Where write.
  \param len Buffer size.
  \param nbr Get how many bytes were read.
 */
int Pipe::read (char* buffer, u_long len, u_long *nbr)
{
  *nbr = 0;
  int ret = ::read (handles[0], buffer, len);
  if (ret == -1)
    {
      terminated = true;
      return 1;
    }
  else if (!ret)
    {
      terminated = true;
      return 0;
    }
  else
    {
      *nbr = (u_long)ret;
    }
  return 0;
}


/*!
  Create the pipe descriptors.  Return 0 on success.
  \param readPipe Specify if the current process uses it for read.  A false
  value means the process uses it to write.
 */
int Pipe::create (bool readPipe)
{
  return checked::pipe2 (handles, 0);
}

/*!
  Write from the pipe.
  \param buffer What write.
  \param len Buffer size.
  \param nbw Get how many bytes were written.
 */
int Pipe::write (const char* buffer, u_long len, u_long *nbw)
{
  *nbw = 0;
  int ret = checked::write (handles[1], buffer, len);
  if (ret == -1)
    {
      terminated = true;
      return 1;
    }
  else
    *nbw = (u_long) ret;

  return 0;
}

/*!
  Get the handle to use to read from the pipe.
 */
long Pipe::getReadHandle ()
{
  return handles[0];
}

/*!
  Get the handle used to write to the pipe.
 */
long Pipe::getWriteHandle ()
{
  return handles[1];
}


/*!
  Close the pipe.
 */
int Pipe::close ()
{
  terminated = true;
  if (handles[0] >= 0)
    checked::close (handles[0]);
  if (handles[1] >= 0)
    checked::close (handles[1]);

  handles[0] = handles[1] = -1;

  return 0;
}

class PipeExceptionInt : public exception
{
public:
  virtual const char *what () const throw ()
  {
    return message;
  }

  PipeExceptionInt (string & str)
  {
    this->message = str.c_str ();
  }

private:
  const char *message;
};

/*!
  Invert the current pipe on another instance of the class.
  The input stream will be used as output and viceversa.
  \param pipe The pipe where write.
 */
void Pipe::inverted (Pipe& pipe)
{
  pipe.handles[0] = checked::dup (handles[1]);
  pipe.handles[1] = checked::dup (handles[0]);
  if (pipe.handles[0] < 0 || pipe.handles[1] < 0)
    {
      string err (_("Internal error"));
      throw PipeExceptionInt (err);
    }
}

Pipe::Pipe ()
{
  terminated = false;
  handles[0] = handles[1] = -1;
}

Pipe::~Pipe ()
{
  close ();
}

/*!
  Close the read stream of the pipe.
 */
void Pipe::closeRead ()
{
  terminated = true;
  if (handles[0] >= 0)
    checked::close (handles[0]);
  handles[0] = -1;
}

/*!
  Close the write stream of the pipe.
 */
void Pipe::closeWrite ()
{
  terminated = true;
  if (handles[1] >= 0)
    checked::close (handles[1]);
  handles[1] = -1;
}

/*!
  Wait until new data is ready.  Do not wait more
  than the specified timeout.
  \param sec Seconds part of the timeout.
  \param usec Micro seconds part of the timeout.
 */
int Pipe::waitForData (int sec, int usec)
{
  struct timeval tv;
  fd_set readfds;
  int ret;
  tv.tv_sec = sec;
  tv.tv_usec = usec;

  FD_ZERO (&readfds);

  FD_SET (handles[0], &readfds);

  ret = checked::select (handles[0] + 1, &readfds, NULL, NULL, &tv);

  if (ret == -1 || ret == 0)
    return 0;

  if (FD_ISSET (handles[0], &readfds))
    return 1;

  return 0;
}
