/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "../stdafx.h"
#include "../include/utility.h"
#include "../include/sockets.h"
extern "C" {
#include <string.h>
#include <stdio.h>
#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#endif
}

#ifdef WIN32

#pragma comment(lib,"wsock32.lib")
#pragma comment(lib,"ws2_32.lib")

#ifndef DO_NOT_USE_SSL
 #pragma comment(lib,"libssl.lib")/*!Import the OpenSSL library*/ 
 #pragma comment(lib,"libcrypto.lib")/*!Import the OpenSSL library*/
#endif

#endif
/*!
 *Source code to wrap the socket library to MyServer project.
 */
int startupSocketLib(u_short ver)
{
#ifdef WIN32	
	/*!
   *Under windows we need to initialize the socket library before use it.
   */
	WSADATA wsaData;
	return WSAStartup(ver, &wsaData);
#else
	return 0;
#endif
}
/*!
 *Returns the socket handle
 */
MYSERVER_SOCKET_HANDLE MYSERVER_SOCKET::getHandle()
{
	return socketHandle;
}
/*!
 *Set the handle for the socket
 */
int MYSERVER_SOCKET::setHandle(MYSERVER_SOCKET_HANDLE h)
{
	socketHandle=h;
	return 1;
}
/*!
 *Check if the two sockets have the same handle descriptor
 */
int MYSERVER_SOCKET::operator==(MYSERVER_SOCKET s)
{
	return socketHandle==s.socketHandle;
}
/*!
 *Set the socket using the = operator
 */
int MYSERVER_SOCKET::operator=(MYSERVER_SOCKET s)
{
  /*! Do a raw memory copy.*/
	memcpy(this,&s,sizeof(s));
	return 1;
}
/*!
 *Create the socket.
 */
int MYSERVER_SOCKET::socket(int af,int type,int protocol,int useSSL)
{
	sslSocket=useSSL;
	socketHandle=(MYSERVER_SOCKET_HANDLE)::socket(af,type,protocol);
#ifndef DO_NOT_USE_SSL
	if(sslSocket)
	{
		initializeSSL();
	}
#endif
	return	(int)socketHandle;
}
/*!
 *Set the socket handle.
 */
MYSERVER_SOCKET::MYSERVER_SOCKET(MYSERVER_SOCKET_HANDLE handle)
{
	setHandle(handle);
}
/*!
 *Constructor of the class.
 */
MYSERVER_SOCKET::MYSERVER_SOCKET()
{
  /*! Reset everything. */
#ifndef DO_NOT_USE_SSL
  localSSL = 0;
	sslSocket=0;
	sslConnection=0;
	sslContext=0;
  sslMethod = 0;
#endif
	serverSocket=0;
	setHandle(0);
}

/*!
 *Bind the port to the socket.
 */
int MYSERVER_SOCKET::bind(MYSERVER_SOCKADDR* sa,int namelen)
{
#ifdef WIN32	
	return ::bind((SOCKET)socketHandle,sa,namelen);
#endif
#ifdef NOT_WIN
	return ::bind((int)socketHandle,sa,namelen);
#endif
}

/*!
 *Listen for other connections.
 */
int MYSERVER_SOCKET::listen(int max)
{
#ifdef WIN32
	return ::listen(socketHandle,max);
#endif
#ifdef NOT_WIN
	return ::listen((int)socketHandle,max);
#endif
}

/*!
 *Accept a new connection.
 */
MYSERVER_SOCKET MYSERVER_SOCKET::accept(MYSERVER_SOCKADDR* sa,int* sockaddrlen,
                                        int /*!sslHandShake*/)
{

	MYSERVER_SOCKET s;
#ifndef DO_NOT_USE_SSL
	s.sslConnection=0;
	s.sslContext=0;
	s.sslSocket=0;
#endif
#ifdef WIN32
	MYSERVER_SOCKET_HANDLE h=(MYSERVER_SOCKET_HANDLE)::accept(socketHandle,sa,
                                                            sockaddrlen);
	s.setHandle(h);
#endif
#ifdef NOT_WIN
	socklen_t Connect_Size = *sockaddrlen;
	int as = ::accept((int)socketHandle,sa,&Connect_Size);
	s.setHandle(as);
#endif

	return s;
}

/*!
 *Close the socket.
 */
int MYSERVER_SOCKET::closesocket()
{
#ifndef DO_NOT_USE_SSL
	freeSSL();
#endif
#ifdef WIN32
  if(socketHandle)
    return ::closesocket(socketHandle);
  else
    return 0;
#endif

#ifdef NOT_WIN
  if(socketHandle)
    return ::close((int)socketHandle);
  else
    return 0;
#endif
}

/*!
 *Returns an host by its address.
 */
MYSERVER_HOSTENT *MYSERVER_SOCKET::gethostbyaddr(char* addr,int len,int type)
{
#ifdef WIN32
	HOSTENT *he=::gethostbyaddr(addr,len,type);
#endif
#ifdef NOT_WIN
	struct hostent * he=::gethostbyaddr(addr,len,type);
#endif
	return he;
}

/*!
*Returns an host by its name
*/
MYSERVER_HOSTENT *MYSERVER_SOCKET::gethostbyname(const char *hostname)
{	
	return (MYSERVER_HOSTENT *)::gethostbyname(hostname);
}

/*!
 *Shutdown the socket.
 */
int MYSERVER_SOCKET::shutdown(int how)
{
#ifndef DO_NOT_USE_SSL
	if(sslSocket && sslConnection)
	{
		SSL_shutdown(sslConnection);
	}
#endif
#ifdef WIN32
	return ::shutdown(socketHandle,how);
#endif
#ifdef NOT_WIN
	return ::shutdown((int)socketHandle,how);
#endif
}
/*!
 *Set socket options.
 */
int	MYSERVER_SOCKET::setsockopt(int level,int optname,const char *optval,int optlen)
{
	return ::setsockopt(socketHandle,level, optname,optval,optlen);
}

/*!
 *Send data over the socket.
 *Return -1 on error.
 */
int MYSERVER_SOCKET::send(const char* buffer,int len,int flags)
{
#ifndef DO_NOT_USE_SSL
	if(sslSocket)
	{
		int err;
		do
		{
			err=SSL_write(sslConnection,buffer,len);
		}while((err<=0) &&(SSL_get_error(sslConnection,err) ==SSL_ERROR_WANT_WRITE 
                       || SSL_get_error(sslConnection,err) == SSL_ERROR_WANT_READ));
    if(err<=0)
      return -1;
    else
      return err;
	}
#endif
#ifdef WIN32
	int ret;
	do
  {
    ret=::send(socketHandle,buffer,len,flags);
  }while((ret == SOCKET_ERROR) && (GetLastError() == WSAEWOULDBLOCK));
#endif
#ifdef NOT_WIN
	return	::send((int)socketHandle,buffer,len,flags);
#endif
}

/*!
 *Function used to control the socket.
 */
int MYSERVER_SOCKET::ioctlsocket(long cmd,unsigned long* argp)
{
#ifdef WIN32
	return ::ioctlsocket(socketHandle,cmd,argp);
#endif
#ifdef NOT_WIN
	int int_argp = 0;
	int ret = ::ioctl((int)socketHandle,cmd,&int_argp);
	*argp = int_argp;
	return ret;

#endif
}

/*!
 *Connect the socket.
 */
int MYSERVER_SOCKET::connect(MYSERVER_SOCKADDR* sa,int na)
{
#ifndef DO_NOT_USE_SSL
	if(sslSocket)
	{
    sslMethod = SSLv23_method();
    /*! Create the local context. */
    sslContext = SSL_CTX_new(sslMethod);
    if(sslContext == 0)
      return -1;

    /*! Do the TCP connection. */
    if(::connect((int)socketHandle,sa,na))
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
    localSSL = 1;
    return 0;
  }
#endif

#ifdef WIN32
  return ::connect((SOCKET)socketHandle,sa,na);
#endif
#ifdef NOT_WIN
  return ::connect((int)socketHandle,sa,na);
#endif
}

/*!
 *Receive data from the socket.
 */
int MYSERVER_SOCKET::recv(char* buffer,int len,int flags,u_long timeout)
{
	int time=get_ticks();
	while(get_ticks()-time<timeout)
	{
    /*! Check if there is data to read before do any read. */
		if(bytesToRead())
			return recv(buffer,len,flags);
	}
	return -1;

}
#ifndef DO_NOT_USE_SSL
/*!
 *Free the SSL connection.
 */
int MYSERVER_SOCKET::freeSSL()
{
  /*! Free up the SSL context. */
	if(sslConnection)
	{
		SSL_free(sslConnection);
		sslConnection=0;
	}
  if(localSSL && sslContext)
  {
    SSL_CTX_free(sslContext);
    sslContext = 0;
  }
	return 1;
}

/*!
 *Set the SSL context.
 */
int MYSERVER_SOCKET::setSSLContext(SSL_CTX* context)
{
	sslContext=context;
	return 1;
}

/*!
 *Initialize the SSL connection.
 *Returns nonzero on errors.
 */
int MYSERVER_SOCKET::initializeSSL(SSL* connection)
{
	freeSSL();
	if(connection)
		sslConnection = connection;
	else
	{
		if(sslContext==0)
			return 1;
		sslConnection =(SSL *)SSL_new(sslContext);
    if(sslConnection == 0)
      return 1;
		SSL_set_read_ahead(sslConnection,0);
	}
	return 0;
}
/*!
 *Set SSL for the socket.
 *Return nonzero on errors.
 */
int MYSERVER_SOCKET::setSSL(int nSSL,SSL* connection)
{
  int ret=0;
	if(sslSocket && (nSSL==0))
		freeSSL();
	if(nSSL && (sslSocket==0))
		ret = initializeSSL(connection);

	sslSocket=nSSL;
  return ret;
}

/*!
 *SSL handshake procedure.
 *Return nonzero on errors.
 */
int MYSERVER_SOCKET::sslAccept()
{
	if(sslContext==0)
		return -1;
	sslSocket = 1;
	if(sslConnection)
		freeSSL();
	sslConnection=SSL_new(sslContext);
	if(sslConnection==0)
  {
		freeSSL();
    return   -1;
  }
	int ssl_accept;
	SSL_set_accept_state(sslConnection);
	if(SSL_set_fd(sslConnection,socketHandle)==0)
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
         || SSL_get_error(sslConnection,ssl_accept) ==SSL_ERROR_WANT_READ);

	if(ssl_accept != 1 )
	{
		shutdown(2);
		freeSSL();
		closesocket();
		return -1;
	}
	SSL_set_read_ahead(sslConnection,1);

	clientCert = SSL_get_peer_certificate(sslConnection);

	if(SSL_get_verify_result(sslConnection)!=X509_V_OK)
	{
		shutdown(2);
		freeSSL();
		closesocket();
		return -1;
	}
	return 0;

}
/*!
 *Returns the SSL connection.
 */
SSL* MYSERVER_SOCKET::getSSLConnection()
{
	return sslConnection;
}

#endif /*Endif for routines used only with the SSL library*/


/*!
 *Returns if the connection is using SSL.
 */
int MYSERVER_SOCKET::getSSL()
{
	return sslSocket;
}

/*!
 *Receive data from the socket.
 *Returns -1 on errors.
 */
int MYSERVER_SOCKET::recv(char* buffer,int len,int flags)
{
	int err=0;
#ifndef DO_NOT_USE_SSL
	if(sslSocket && sslConnection)
	{
    do
    {	
        err=SSL_read(sslConnection,buffer,len);
    }while((err <= 0) && 
           (SSL_get_error(sslConnection,err) == SSL_ERROR_WANT_X509_LOOKUP)
           || (SSL_get_error(sslConnection,err) == SSL_ERROR_WANT_READ)
            || (SSL_get_error(sslConnection,err) == SSL_ERROR_WANT_WRITE));

		if(err<=0)
			return -1;
		else 
			return err;
	}
#endif

#ifdef WIN32
	do
  {
  	err=::recv(socketHandle,buffer,len,flags);
  }while((err == SOCKET_ERROR) && (GetLastError() == WSAEWOULDBLOCK));
	if(err==SOCKET_ERROR)
		return -1;
	else 
		return err;
#endif
#ifdef NOT_WIN
	err=::recv((int)socketHandle,buffer,len,flags);
	if(err == 0)
		err = -1;
	return err;
#endif

}

/*!
 *Returns the number of bytes waiting to be read.
 */
u_long MYSERVER_SOCKET::bytesToRead()
{
  u_long nBytesToRead=0;
#ifndef DO_NOT_USE_SSL
	if(sslSocket)
	{
		char b;
		SSL_peek(sslConnection,&b,1);
		return  SSL_pending(sslConnection);
	}
  else
#endif
  {
    ioctlsocket(FIONREAD,&nBytesToRead);
  }
	return nBytesToRead;
}

/*!
 *Returns the hostname.
 */
int MYSERVER_SOCKET::gethostname(char *name,int namelen)
{
	return ::gethostname(name,namelen);
}

/*!
 *Returns the sockname.
 */
int MYSERVER_SOCKET::getsockname(MYSERVER_SOCKADDR *ad,int *namelen)
{
#ifdef WIN32
	return ::getsockname(socketHandle,ad,namelen);
#endif
#ifdef NOT_WIN
	socklen_t len = *namelen;
	int ret = ::getsockname((int)socketHandle,ad,&len);
	*namelen = len;
	return ret;
#endif
}

/*!
 *Set the socket used by the server.
 */
void MYSERVER_SOCKET::setServerSocket(MYSERVER_SOCKET* sock)
{
	serverSocket=sock;
}
/*!
 *Returns the server socket.
 */
MYSERVER_SOCKET* MYSERVER_SOCKET::getServerSocket()
{
	return serverSocket;
}

/*!
 *Check for data to be read on the socket
 *Returns 1 if there is data to read, 0 if not.
 */
int MYSERVER_SOCKET::dataOnRead(int sec, int usec)
{
	struct timeval tv;
	fd_set readfds;

	tv.tv_sec = sec;
	tv.tv_usec = usec;

	FD_ZERO(&readfds);
#ifdef WIN32
	FD_SET((SOCKET)socketHandle, &readfds);
#else
	FD_SET(socketHandle, &readfds);
#endif
	::select(socketHandle+1, &readfds, NULL, NULL, &tv);

	if (FD_ISSET(socketHandle, &readfds))
		return 1;
	else
		return 0;
}
