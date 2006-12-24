/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006 The MyServer Team
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

#ifndef SOCKETS_H
#define SOCKETS_H

#include "../stdafx.h"
#include "../include/stream.h"

#include <string>
using namespace std;

#ifndef DO_NOT_USE_SSL
#include<openssl/ssl.h>
#include<openssl/crypto.h>
#include<openssl/lhash.h>
#include<openssl/err.h>
#include<openssl/bn.h>
#include<openssl/pem.h>
#include<openssl/x509.h>
#include<openssl/rand.h>
#endif

#ifdef WIN32
#ifndef SOCKETLIBINCLUDED
extern "C"
{
#include <Ws2tcpip.h>
#include <winsock2.h>
}
#define SOCKETLIBINCLUDED
#endif
#endif

#ifdef NOT_WIN
extern "C" {
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>  
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
}

#define SOCKET int
#define INVALID_SOCKET -1
#define SD_BOTH SHUT_RDWR
#endif

#ifdef INET6_ADDRSTRLEN
#define MAX_IP_STRING_LEN	INET6_ADDRSTRLEN
#else
#define MAX_IP_STRING_LEN	32
#endif

typedef unsigned int SocketHandle;
typedef struct sockaddr_storage MYSERVER_SOCKADDR_STORAGE;
typedef struct sockaddr_storage MYSERVER_SOCKADDRIN;
//typedef struct sockaddr_in MYSERVER_SOCKADDRIN;
//typedef struct sockaddr MYSERVER_SOCKADDR;
typedef struct sockaddr_storage MYSERVER_SOCKADDR;
typedef struct hostent MYSERVER_HOSTENT;
int startupSocketLib(u_short);

class Socket: public Stream
{
private:
	SocketHandle socketHandle;
	int sslSocket;
#ifndef DO_NOT_USE_SSL
  /*! This is defined if all SSL members are used only by this socket.  */
  int localSSL;
	SSL *sslConnection;
	SSL_CTX *sslContext;
	X509 *clientCert;

  /*! This is used only by clients sockets.  */
  SSL_METHOD* sslMethod;
#endif
	/*! Pointer to the socket that has accepted this connection.  */
	Socket *serverSocket;

  /*! Send throttling rate.  */
  u_long throttlingRate;

	/*! Stop the sockets system.  */
	static bool denyBlockingOperations;

  int rawSend(const char* buffer,int len,int flags);
public:
	void setServerSocket(Socket*);
	Socket* getServerSocket();
#ifndef DO_NOT_USE_SSL
	int freeSSL();
	int setSSLContext(SSL_CTX*);
	int initializeSSL(SSL* connection = 0);
	int setSSL(int,SSL* connection = 0);
	SSL* getSSLConnection();
	int sslAccept();
#endif
	static void stopBlockingOperations(bool);
	int getSSL();
	SocketHandle getHandle();
	int setHandle(SocketHandle);
	static MYSERVER_HOSTENT *gethostbyaddr(char* addr, int len, int type);
	static MYSERVER_HOSTENT *gethostbyname(const char*);
	static int gethostname(char*, int);
	int socket(int, int, int, bool=false);
	int bind(MYSERVER_SOCKADDR*, int);
	int listen(int);
	Socket();
	Socket(SocketHandle);
	Socket accept(MYSERVER_SOCKADDR*, int*, int sslHandShake = 0);
	int closesocket();
	int setsockopt(int,int, const char*,int);
	int shutdown(int how);
	int ioctlsocket(long, unsigned long*);
	int send(const char*, int, int);
	int connect(MYSERVER_SOCKADDR*, int);
	int connect(const char* host, u_short port);
	int recv(char*, int, int);
	int recv(char*, int, int, u_long);
	u_long bytesToRead();
	int operator==(Socket);
	int operator=(Socket);
	int getsockname(MYSERVER_SOCKADDR*,int*);
  int setNonBlocking(int);
#ifdef __HURD__
	int dataOnRead(int sec = 1, int usec = 500);
#else
	int dataOnRead(int sec = 0, int usec = 500);
#endif
  u_long getThrottling();
  void setThrottling(u_long);
  static int getLocalIPsList(string&);
  /*! Inherithed from Stream.  */
  virtual int read(char* buffer, u_long len, u_long *nbr);
  virtual int write(const char* buffer, u_long len, u_long *nbw);
};
#endif
