/*
MyServer
Copyright (C) 2009 Free Software Foundation, Inc.
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
#include <include/base/unix_socket/unix_socket.h>

UnixSocket::UnixSocket ()
{

}

/*!
 *Bind the socket to a file.  If the file exists it is removed.
 *\param path Path to the file bound by the socket.
 *\return 0 on success.
 */
int UnixSocket::bind(const char* path)
{
  if (path == NULL)
    return -1;

#ifdef AF_UNIX
  makeAddrInfo (&addr, path);
  unlink (path);

  return Socket::bind ((MYSERVER_SOCKADDR*)&addr, sizeof (sockaddr_un));
#else
  return -1;
#endif
}

/*!
 *Shutdown the socket.
 *\return 0 on success.
 */
int UnixSocket::shutdown ()
{
#ifdef AF_UNIX
  Socket::shutdown (2);
  unlink (addr.sun_path);
#else
  return -1;
#endif
}


/*!
 *Free the socket.
 *\return 0 on success.
 */
int UnixSocket::close ()
{
#ifdef AF_UNIX
  Socket::close ();
#else
  return -1;
#endif
}

/*!
 *Open the socket.
 *\return the socket handle on success.
 */
int UnixSocket::socket ()
{
#ifdef AF_UNIX
  return Socket::socket (AF_UNIX, SOCK_STREAM, 0);
#else
  return 0;
#endif
}

/*!
 *Connect to an Unix socket by its path.
 *\param path Path to the Unix socket.
 *\return 0 on success.
 */
int UnixSocket::connect (const char* path)
{
  if (path == NULL)
    return -1;

#ifdef AF_UNIX
  makeAddrInfo (&addr, path);

  return Socket::connect ((MYSERVER_SOCKADDR*)&addr, sizeof (addr));
#else
  return -1;
#endif
}

/*!
 *Accept a new connection.
 */
Socket UnixSocket::accept ()
{

#ifdef AF_UNIX
  sockaddr_un addr;
  int len = sizeof (addr);
  return Socket::accept ((MYSERVER_SOCKADDR*)&addr, &len);
#else
  Socket s;
  s.setHandle (0);
  return s;
#endif
}
