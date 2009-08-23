/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008, 2009 Free Software Foundation, Inc.
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

#ifndef SOCKET_H
#define SOCKET_H

#include "stdafx.h"
#include <include/filter/stream.h>

#include <string>
using namespace std;

#ifdef WIN32
#ifndef SOCKETLIBINCLUDED
extern "C"
{
#include <winsock2.h>
}
#define SOCKETLIBINCLUDED
#endif
#endif

#ifndef WIN32
extern "C" {
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
}

#define INVALID_SOCKET -1
#define SD_BOTH SHUT_RDWR
#endif

#ifdef WIN32
typedef SOCKET SocketHandle;
#else
typedef int SocketHandle;
#endif


#ifdef INET6_ADDRSTRLEN
#define MAX_IP_STRING_LEN  INET6_ADDRSTRLEN
#else
#define MAX_IP_STRING_LEN  32
#endif

typedef struct sockaddr_storage MYSERVER_SOCKADDR_STORAGE;
typedef struct sockaddr_storage MYSERVER_SOCKADDRIN;
typedef struct sockaddr_storage MYSERVER_SOCKADDR;
typedef struct hostent MYSERVER_HOSTENT;

class Socket: public Stream
{
public:
  static int startupSocketLib ();

  void setServerSocket(Socket*);
  Socket* getServerSocket();

  static void stopBlockingOperations(bool);
  virtual Handle getHandle();
  void setHandle(SocketHandle);
  static MYSERVER_HOSTENT *gethostbyaddr(char* addr, int len, int type);
  static MYSERVER_HOSTENT *gethostbyname(const char*);
  static int gethostname(char*, int);
  int socket(int, int, int);
  int bind(MYSERVER_SOCKADDR*, int);
  int listen(int);
  Socket();
  Socket(Socket*);
  Socket(SocketHandle);

  Socket accept(MYSERVER_SOCKADDR*, socklen_t*);
  int setsockopt(int,int, const char*,int);

  virtual int connect(MYSERVER_SOCKADDR*, int);
  virtual int close();
  virtual int shutdown(int how);
  virtual int recv(char*, int, int, u_long);
  virtual int recv(char*, int, int);
  virtual u_long bytesToRead();

  int ioctlsocket(long, unsigned long*);
  int send(const char*, int, int);
  int connect(const char* host, u_short port);
  int operator==(Socket);
  int operator=(Socket);
  int getsockname(MYSERVER_SOCKADDR*,int*);
  int setNonBlocking(int);
  bool getNonBLocking () {return isNonBlocking;}
  virtual int dataOnRead(int sec = 0, int usec = 500);

  u_long getThrottling();
  void setThrottling(u_long);
  static int getLocalIPsList(string&);
  /*! Inherithed from Stream.  */
  virtual int read(char* buffer, u_long len, u_long *nbr);
  virtual int write(const char* buffer, u_long len, u_long *nbw);

protected:
  SocketHandle socketHandle;

  /*! Pointer to the socket that has accepted this connection.  */
  Socket *serverSocket;

  /*! Send throttling rate.  */
  u_long throttlingRate;

  /*! Is the socket non blocking?  */
  bool isNonBlocking;

  /*! Stop the sockets system.  */
  static bool denyBlockingOperations;

  virtual int rawSend(const char* buffer, int len, int flags);
};
#endif
