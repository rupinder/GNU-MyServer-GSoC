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
SocketHandle Socket::getHandle()
{
	return socketHandle;
}
/*!
 *Set the handle for the socket
 */
int Socket::setHandle(SocketHandle h)
{
	socketHandle=h;
	return 1;
}
/*!
 *Check if the two sockets have the same handle descriptor
 */
int Socket::operator==(Socket s)
{
	return socketHandle==s.socketHandle;
}
/*!
 *Set the socket using the = operator
 */
int Socket::operator=(Socket s)
{
  /*! Do a raw memory copy.*/
	memcpy(this,&s,sizeof(s));
	return 1;
}
/*!
 *Create the socket.
 */
int Socket::socket(int af,int type,int protocol,int useSSL)
{
	sslSocket=useSSL;
	socketHandle=(SocketHandle)::socket(af,type,protocol);
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
Socket::Socket(SocketHandle handle)
{
  throttlingRate = 0;
	setHandle(handle);
}

/*!
 *Return the throttling rate(bytes/second) used by the socket. A return value
 *of zero means that no throttling is used.
 */
u_long Socket::getThrottling()
{
  return throttlingRate;
}

/*!
 *Set the throttling rate(bytes/second) for the socket. 
 *Use a zero rate to disable throttling.
 */
void Socket::setThrottling(u_long tr)
{
  throttlingRate = tr;
}

/*!
 *Constructor of the class.
 */
Socket::Socket()
{
  /*! Reset everything. */
#ifndef DO_NOT_USE_SSL
  localSSL = 0;
	sslSocket=0;
	sslConnection=0;
	sslContext=0;
  sslMethod = 0;
#endif
  throttlingRate = 0;
	serverSocket=0;
	setHandle(0);
}

/*!
 *Bind the port to the socket.
 */
int Socket::bind(MYSERVER_SOCKADDR* sa,int namelen)
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
int Socket::listen(int max)
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
Socket Socket::accept(MYSERVER_SOCKADDR* sa,
                                int* sockaddrlen, int /*!sslHandShake*/)
{

#ifdef NOT_WIN
	socklen_t Connect_Size;
	int accept_handle;
#endif

	Socket s;
#ifndef DO_NOT_USE_SSL
	s.sslConnection=0;
	s.sslContext=0;
	s.sslSocket=0;
#endif

#ifdef WIN32
	SocketHandle h=(SocketHandle)::accept(socketHandle,sa,
                                        sockaddrlen);
	s.setHandle(h);
#endif

#ifdef NOT_WIN
	Connect_Size = (socklen_t) *sockaddrlen;
	
  accept_handle = ::accept((int)socketHandle, sa, 
                           (socklen_t*)&Connect_Size);
	s.setHandle(accept_handle);
#endif

	return s;
}

/*!
 *Close the socket.
 */
int Socket::closesocket()
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
MYSERVER_HOSTENT *Socket::gethostbyaddr(char* addr,int len,int type)
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
MYSERVER_HOSTENT *Socket::gethostbyname(const char *hostname)
{	
	return (MYSERVER_HOSTENT *)::gethostbyname(hostname);
}

/*!
 *Shutdown the socket.
 */
int Socket::shutdown(int how)
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
int	Socket::setsockopt(int level,int optname, 
                                const char *optval,int optlen)
{
	return ::setsockopt(socketHandle,level, optname,optval,optlen);
}


/*!
 *Send data over the socket.
 *Return -1 on error.
 *This routine is acccessible only from the Socket class.
 */
int Socket::rawSend(const char* buffer,int len,int flags)
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
 *Send data over the socket.
 *Return -1 on error.
 *If a throttling rate is specified, send will use it.
 */
int Socket::send(const char* buffer, int len, int flags)
{
  u_long toSend =(u_long) len;
	int ret;
  /*! If no throttling is specified, send only one big data chunk. */
  if(throttlingRate == 0)
  {
    ret = rawSend(buffer, len, flags);
    return ret;
  }
  else
  {
    for(;;)
    {
      /*! When we can send data again? */
      u_long time = get_ticks() + (1000*1024/throttlingRate) ;
      /*! If a throttling rate is specified, send chunks of 1024 bytes. */
      ret = rawSend( buffer+(len-toSend), toSend < 1024 ? toSend : 1024, flags);  
      /*! On errors returns directly -1. */
      if(ret < 0)
        return -1;
      toSend -= (u_long)ret;
      /*! If there are other bytes to send wait before cycle again. */
      if(toSend)
      {
        
        while(get_ticks() <= time)
          Thread::wait(1);
      }    
      else
        break;
    }
    /*! Return the number of sent bytes. */
    return len-toSend;
  }
  return 0;
}

/*!
 *Function used to control the socket.
 */
int Socket::ioctlsocket(long cmd,unsigned long* argp)
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
int Socket::connect(MYSERVER_SOCKADDR* sa, int na)
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
int Socket::recv(char* buffer, int len, int flags, u_long timeout)
{
	u_long time=get_ticks();
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
int Socket::freeSSL()
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
int Socket::setSSLContext(SSL_CTX* context)
{
	sslContext=context;
	return 1;
}

/*!
 *Initialize the SSL connection.
 *Returns nonzero on errors.
 */
int Socket::initializeSSL(SSL* connection)
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
int Socket::setSSL(int nSSL,SSL* connection)
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
int Socket::sslAccept()
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
SSL* Socket::getSSLConnection()
{
	return sslConnection;
}

#endif /*Endif for routines used only with the SSL library*/


/*!
 *Returns if the connection is using SSL.
 */
int Socket::getSSL()
{
	return sslSocket;
}

/*!
 *Receive data from the socket.
 *Returns -1 on errors.
 */
int Socket::recv(char* buffer,int len,int flags)
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
u_long Socket::bytesToRead()
{
  u_long nBytesToRead=0;
#ifndef DO_NOT_USE_SSL
	if(sslSocket)
	{
		char b;
		int ret = SSL_peek(sslConnection,&b,1);
    if(ret < 0)
      return 0;
    if((ret == 0) &&(sslConnection->shutdown))
      return 0;
		return  SSL_pending(sslConnection);
	}
  else
#endif
  {
#ifdef FIONREAD
    ioctlsocket(FIONREAD,&nBytesToRead);
#else 
#ifdef I_NREAD
    ::ioctlsocket( I_NREAD, &nBytesToRead ) ;
#endif
#endif
  }
	return nBytesToRead;
}

/*!
 *Pass a nonzero value to set the socket to be nonblocking.
 */
int Socket::setNonBlocking(int non_blocking)
{
  int ret = -1;
#ifdef FIONBIO
  u_long nonblock = non_blocking ? 1 : 0;
  ret = ioctlsocket( FIONBIO, &nonblock);

#else

#ifdef NOT_WIN
  int flags;
  flags = fcntl((int)socketHandle, F_GETFL, 0);
  if (flags < 0) 
    return -1;
  
  if(non_blocking)
    flags |= O_NONBLOCK;
  else
    flags &= ~O_NONBLOCK;
 
  ret = fcntl((int)socketHandle, F_SETFL, flags);
#endif

#endif
  return ret;
}

/*!
 *Returns the hostname.
 */
int Socket::gethostname(char *name,int namelen)
{
	return ::gethostname(name,namelen);
}

/*!
 *Returns the sockname.
 */
int Socket::getsockname(MYSERVER_SOCKADDR *ad,int *namelen)
{
#ifdef WIN32
	return ::getsockname(socketHandle,ad,namelen);
#endif
#ifdef NOT_WIN
	socklen_t len =(socklen_t) *namelen;
	int ret = ::getsockname((int)socketHandle, ad, &len);
	*namelen = (int)len;
	return ret;
#endif
}

/*!
 *Set the socket used by the server.
 */
void Socket::setServerSocket(Socket* sock)
{
	serverSocket=sock;
}
/*!
 *Returns the server socket.
 */
Socket* Socket::getServerSocket()
{
	return serverSocket;
}

/*!
 *Check for data to be read on the socket
 *Returns 1 if there is data to read, 0 if not.
 */
int Socket::dataOnRead(int sec, int usec)
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
