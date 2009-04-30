/* -*- mode: c++ -*- */
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

#ifndef UNIX_SOCKET_H
#define UNIX_SOCKET_H

#include "stdafx.h"
#include <include/base/socket/socket.h>


extern "C"
{
#ifdef WIN32
#undef AF_UNIX //This shouldn't happen, but better be sure.
#else
#include <sys/un.h>
#endif
}


class UnixSocket: public Socket
{
public:
  UnixSocket ();
  int bind(const char *path);
  int shutdown ();
  int close ();
  int connect (const char* path);
  Socket accept ();
  int socket ();
protected:

#ifdef AF_UNIX
  sockaddr_un addr;
#endif

  int readHandle (FileHandle* fd);
  int writeHandle (FileHandle fd);

private:
#ifdef AF_UNIX
  void makeAddrInfo (sockaddr_un *info, const char *path)
  {
    memset(info, 0, sizeof(struct sockaddr_un));
    info->sun_family = AF_UNIX;
    strcpy (info->sun_path, path);
  }
#endif
};

#endif
