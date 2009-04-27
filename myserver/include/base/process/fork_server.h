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


#include <unistd.h>
#include <include/base/socket_pair/socket_pair.h>
#include <include/base/socket/socket.h>
#include <include/base/sync/mutex.h>

#ifndef FORK_SERVER_H
#define FORK_SERVER_H

struct StartProcInfo;

class ForkServer
{
 public:
  const static int FLAG_USE_IN = 1;
  const static int FLAG_USE_OUT = 2;
  const static int FLAG_USE_ERR = 4;
  const static int FLAG_STDIN_SOCKET = 8;

  ForkServer () {initialized = false; serverLock.init ();}
  ~ForkServer () {serverLock.destroy ();}

  void killServer ();
  int startForkServer ();

  int writeInt (SocketPair *socket, int num);
  int writeString (SocketPair *socket, const char* str, int len);
  int readInt (SocketPair *sock, int *dest);
  int readString (SocketPair *sock, char **out);

  int handleRequest (SocketPair *serverSock);
  int forkServerLoop (SocketPair *socket);

  int executeProcess (StartProcInfo *spi, int flags,
                      int *pid, int *port, 
                      bool waitEnd = false);

  u_short getPort (){return port;}
  bool isInitialized (){return initialized;}
  int generateListenerSocket (Socket &socket, u_short *port);

 private:

  SocketPair socket;

  Mutex serverLock;
  u_short port;
  bool initialized;
};

#endif
