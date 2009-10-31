/*
  MyServer
  Copyright (C) 2008, 2009 Free Software Foundation, Inc.
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

#ifndef FORK_SERVER_H
# define FORK_SERVER_H

# include "stdafx.h"

#ifndef WIN32
# include <unistd.h>
#endif

#include <include/base/unix_socket/unix_socket.h>


struct StartProcInfo;

class ForkServer
{
 public:
  const static int FLAG_USE_IN = 1;
  const static int FLAG_USE_OUT = 2;
  const static int FLAG_USE_ERR = 4;
  const static int FLAG_STDIN_SOCKET = 8;

  ForkServer () {initialized = false;}
  ~ForkServer () {}

  void killServer ();
  int startForkServer ();

  int writeInt (Socket *socket, int num);
  int writeString (Socket *socket, const char* str, int len);
  int readInt (Socket *sock, int *dest);
  int readString (Socket *sock, char **out);

  int handleRequest (Socket *serverSock);
  int forkServerLoop (UnixSocket *socket);

  int executeProcess (StartProcInfo *spi, int flags,
                      int *pid, int *port,
                      bool waitEnd = false);

  u_short getPort (){return port;}
  bool isInitialized (){return initialized;}
  int generateListenerSocket (Socket &socket, u_short *port);

 private:
  string socketPath;
  UnixSocket socket;


  u_short port;
  bool initialized;
};

#endif
