/*
MyServer
Copyright (C) 2009, 2010 Free Software Foundation, Inc.
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
#include <include/base/socket_pair/socket_pair.h>

#ifndef WIN32
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

# include <sys/socket.h>
# include <sys/uio.h>

#else
# include <windows.h>
#endif

#include <string>
#include <sstream>

#include <include/base/exceptions/checked.h>

using namespace std;



SocketPair::SocketPair ()
{

}

/*!
  Create a Socket pair.
  \return 0 on success.
 */
int SocketPair::create ()
{
#ifndef WIN32
  int ret = socketpair (AF_UNIX, SOCK_STREAM, 0, (int*) handles);
  if (ret == 0)
    fd = handles[0];

  return ret;
#else
  struct sockaddr_in addr;
  SOCKET listener;
  int e;
  int addrlen = sizeof (addr);
  DWORD flags = 0;

  if (handles == 0)
    return -1;

  handles[0] = handles[1] = -1;
  listener = checked::socket (AF_INET, SOCK_STREAM, 0);

  if (listener < 0)
    return -1;

  memset (&addr, 0, sizeof (addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl (0x7f000001);
  addr.sin_port = 0;

  if (checked::bind (listener, (struct sockaddr*) &addr, sizeof (addr)) < 0)
    {
      checked::close (listener);
      return -1;
    }

  if (checked::getsockname (listener, (struct sockaddr*) &addr, &addrlen) < 0)
    {
      checked::close (listener);
      return -1;
    }

  do
    {
      if (checked::listen (listener, 1) < 0)
        break;

      if ((handles[0] = checked::socket (AF_INET, SOCK_STREAM, 0)) < 0)
        break;

      if (checked::connect (handles[0], (struct sockaddr*) &addr,
                           sizeof (addr)) < 0)
        break;

      if ((handles[1] = checked::accept (listener, NULL, NULL)) < 0)
        break;

      fd = handles[0];

      checked::close (listener);
      return 0;
    } while (0);

  checked::close (listener);

  if (handles[0] != -1)
    checked::close (handles[0]);


  if (handles[1] != -1)
    checked::close (handles[1]);

  return -1;
#endif

  return 0;
}

/*!
  Get the first handle used by the socket pair.
 */
SocketHandle SocketPair::getFirstHandle ()
{
  return handles[0];
}

/*!
  Get the second handle used by the socket pair.
 */
SocketHandle SocketPair::getSecondHandle ()
{
  return handles[1];
}


/*!
  Invert the current socket pair on the passed argument.
  The first handle will be the second handle on the inverted SocketPair
  and the second handle will be the first one.
  \param inverted It will be the inverted SocketPair.
 */
void SocketPair::inverted (SocketPair& inverted)
{
  inverted.handles[0] = handles[1];
  inverted.handles[1] = handles[0];

  inverted.fd = handles[1];
}

/*!
  Close both handles.
 */
int SocketPair::close ()
{
  closeFirstHandle ();
  closeSecondHandle ();

  return 0;
}

/*!
  Close the first handle.
 */
void SocketPair::closeFirstHandle ()
{
  if (handles[0] < 0)
    return;

  checked::close (handles[0]);
  fd = handles[0] = -1;
}

/*!
  Close the second handle.
 */
void SocketPair::closeSecondHandle ()
{
  if (handles[1] < 0)
    return;

  checked::close (handles[1]);
  handles[1] = -1;
}

/*!
  Configure the server socket blocking or not blocking.
  \param notBlocking The new non-blocking status.
 */
void SocketPair::setNonBlocking (bool notBlocking)
{
  /* FIXME: avoid this trick to set the handle to -1 to avoid
   a close on the real descriptor by the d'tor.  */
  Socket sock0 (handles[0]);
  sock0.setNonBlocking (notBlocking);
  sock0.setHandle (-1);

  Socket sock1 (handles[1]);
  sock1.setNonBlocking (notBlocking);
  sock1.setHandle (-1);
}

/*!
  Read a file handle on the socket pair.
  \param fd The file descriptor to read.
  \return 0 on success.
 */
int SocketPair::readHandle (Handle* fd)
{
  return readFileHandle (handles[0], fd);
}

/*!
  Transmit a file descriptor on the socket.
  \param fd The file descriptor to transmit.
  \return 0 on success.
 */
int SocketPair::writeHandle (Handle fd)
{
  return writeFileHandle (handles[0], fd);
}
