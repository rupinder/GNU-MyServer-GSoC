/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008 The MyServer Team
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


#include "../stdafx.h"
#include "../include/utility.h"
#include "../include/ssl_socket.h"

extern "C" {
#include <string.h>
#include <stdio.h>
#ifdef WIN32
#include <Ws2tcpip.h>
#endif
#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
}

#include <sstream>

using namespace std;

#ifdef WIN32

#pragma comment(lib,"wsock32.lib")
#pragma comment(lib,"ws2_32.lib")

#ifndef DO_NOT_USE_SSL
 #pragma comment(lib,"libssl.lib")/*! Import the OpenSSL library.  */
 #pragma comment(lib,"libcrypto.lib")/*! Import the OpenSSL library.  */
#endif

#endif


/*!
 *Constructor of the class.
 */
SslSocket::SslSocket(Socket* socket) : Socket(socket)
{
#ifndef DO_NOT_USE_SSL
	this->socket = socket;
	sslConnection = 0;
	sslContext = 0;
	clientCert = 0;
  sslMethod = 0;
	externalContext = false;
#endif
}

SslSocket::~SslSocket()
{
#ifndef DO_NOT_USE_SSL
	freeSSL();
#endif
}

/*!
 *Close the socket.
 */
int SslSocket::closesocket()
{
#ifndef DO_NOT_USE_SSL
	freeSSL();
#endif
	return Socket::closesocket();
}

/*!
 *Shutdown the socket.
 */
int SslSocket::shutdown(int how)
{
#ifndef DO_NOT_USE_SSL
	if(sslConnection)
	{
		SSL_shutdown(sslConnection);
	}
#endif

#ifdef WIN32
	return ::shutdown(socketHandle, how);
#endif

#ifdef NOT_WIN
	return ::shutdown((int)socketHandle, how);
#endif
}

/*!
 *Send data over the socket.
 *Return -1 on error.
 *This routine is accessible only from the Socket class.
 */
int SslSocket::rawSend(const char* buffer, int len, int flags)
{
#ifndef DO_NOT_USE_SSL
	int err;
	do
	{
		err = SSL_write(sslConnection,buffer,len);
	}while((err <= 0) &&
				 (SSL_get_error(sslConnection,err) == SSL_ERROR_WANT_WRITE 
					|| SSL_get_error(sslConnection,err) == SSL_ERROR_WANT_READ));
	if(err <= 0)
		return -1;
	else
		return err;
#endif
}

/*!
 *Connect the socket.
 */
int SslSocket::connect(MYSERVER_SOCKADDR* sa, int na)
{
 	if ( sa == NULL || (sa->ss_family != AF_INET && sa->ss_family != AF_INET6) )
 	   return 1;//Andu: TODO our error code or what?
  if ( (sa->ss_family == AF_INET && na != sizeof(sockaddr_in)) || 
  (sa->ss_family == AF_INET6 && na != sizeof(sockaddr_in6)) )
     return 1;//Andu: TODO our error code or what?
#ifndef DO_NOT_USE_SSL
	sslMethod = SSLv23_client_method();
	/*! Create the local context. */
	sslContext = SSL_CTX_new(sslMethod);
	if(sslContext == 0)
		return -1;
	
	/*! Do the TCP connection. */
	if(::connect((int)socketHandle,(const sockaddr *)sa, na))
  {
		SSL_CTX_free(sslContext);
		sslContext = 0;
		return -1;
	}
	sslConnection = SSL_new(sslContext);
	if(sslConnection == 0)
  {
		SSL_CTX_free(sslContext);
		sslContext = 0;
		return -1;
	}
	SSL_set_fd(sslConnection, (int)socketHandle);
	if(SSL_connect(sslConnection) < 0)
  {
		SSL_CTX_free(sslContext);
		closesocket();
		sslContext = 0;
		return -1;
	}
	externalContext = false;
	return 0;
#endif
}


/*!
 *Set the SSL context.
 */
int SslSocket::setSSLContext(SSL_CTX* context)
{
#ifndef DO_NOT_USE_SSL
	sslContext = context;
	externalContext = true;
#endif
	return 1;
}

#ifndef DO_NOT_USE_SSL
/*!
 *Free the SSL connection.
 */
int SslSocket::freeSSL()
{
  /*! free up the SSL context. */
	if(sslConnection)
	{
		SSL_free(sslConnection);
		sslConnection = 0;
	}

  if(sslContext && !externalContext)
  {
		SSL_CTX_free(sslContext);
    sslContext = 0;
  }
	return 1;
}


/*!
 *Returns the SSL connection.
 */
SSL* SslSocket::getSSLConnection()
{
	return sslConnection;
}

#endif /*Endif for routines used only with the SSL library*/

/*!
 *SSL handshake procedure.
 *Return nonzero on errors.
 */
int SslSocket::sslAccept()
{
#ifndef DO_NOT_USE_SSL
	int ssl_accept;
	if(sslContext == 0)
		return -1;
	if(sslConnection)
		freeSSL();
	sslConnection = SSL_new(sslContext);
	if(sslConnection == 0)
  {
		freeSSL();
    return -1;
  }
	SSL_set_accept_state(sslConnection);
	if(SSL_set_fd(sslConnection,socketHandle) == 0)
	{
		shutdown(2);
		freeSSL();
		closesocket();
		return -1;
	}
	
	do
	{
		ssl_accept = SSL_accept(sslConnection);
	}while(SSL_get_error(sslConnection,ssl_accept) == SSL_ERROR_WANT_X509_LOOKUP
         || SSL_get_error(sslConnection,ssl_accept) == SSL_ERROR_WANT_READ);

	if(ssl_accept != 1 )
	{
		shutdown(2);
		freeSSL();
		closesocket();
		return -1;
	}
	SSL_set_read_ahead(sslConnection, 0);

	clientCert = SSL_get_peer_certificate(sslConnection);

	if(SSL_get_verify_result(sslConnection) != X509_V_OK)
	{
		shutdown(2);
		freeSSL();
		closesocket();
		return -1;
	}
#endif
	return 0;

}


/*!
 *Receive data from the socket.
 *Returns -1 on errors.
 */
int SslSocket::recv(char* buffer, int len, int flags)
{
	int err = 0;
#ifndef DO_NOT_USE_SSL
	if(sslConnection)
	{
    do
    {
        err = SSL_read(sslConnection, buffer, len);
    }while((err <= 0) &&
           (SSL_get_error(sslConnection,err) == SSL_ERROR_WANT_X509_LOOKUP)
           || (SSL_get_error(sslConnection,err) == SSL_ERROR_WANT_READ)
            || (SSL_get_error(sslConnection,err) == SSL_ERROR_WANT_WRITE));

		if(err <= 0)
			return -1;
		else
			return err;
	}
#endif
	return 0;
}

/*!
 *Returns the number of bytes waiting to be read.
 */
u_long SslSocket::bytesToRead()
{
	u_long nBytesToRead = 0;
#ifndef DO_NOT_USE_SSL
	nBytesToRead = SSL_pending(sslConnection);

	if(nBytesToRead)
		return nBytesToRead;

	return Socket::bytesToRead();
#endif
	return nBytesToRead;
}



/*!
 *Returns the number of bytes waiting to be read.
 */
int SslSocket::dataOnRead(int, int)
{
	return Socket::dataOnRead(0,0);
	if(bytesToRead())
		return 1;

	return 0;
}

