/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006, 2007 The MyServer Team
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

#ifndef SSL_SOCKETS_H
#define SSL_SOCKETS_H

#include "../stdafx.h"
#include "../include/sockets.h"

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
#endif

class SslSocket : public Socket
{
public:

	int setSSLContext(SSL_CTX*);
	int sslAccept();

#ifndef DO_NOT_USE_SSL
	int freeSSL();
	int initializeSSL(SSL* connection = 0);
	SSL* getSSLConnection();
#endif

	virtual int closesocket();
	virtual int shutdown(int how);
	virtual int connect(MYSERVER_SOCKADDR* sa, int na);
	virtual int recv(char* buffer,int len,int flags);
	virtual int rawSend(const char* buffer, int len, int flags);
	virtual u_long bytesToRead();

	SslSocket(Socket*);
	~SslSocket();

protected:
	Socket* socket;
#ifndef DO_NOT_USE_SSL
	SSL *sslConnection;
	SSL_CTX *sslContext;
	X509 *clientCert;

  /*! This is used only by clients sockets.  */
  SSL_METHOD* sslMethod;
#endif
};


#endif
