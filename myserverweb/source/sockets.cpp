/*
*MyServer
*Copyright (C) 2002 The MyServer Team
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
#pragma comment(lib,"libeay32.lib")/*Import the OpenSSL library*/
#pragma comment(lib,"ssleay32.lib")/*Import the OpenSSL library*/
#endif

#define CERT_FILE "server.pem"		
#define KEY_FILE  "server.key"
#define CIPHER_LIST  "aRSA"

/*
*Source code to wrap the socket library to myServer project.
*/
int startupSocketLib(u_short ver)
{
#ifdef WIN32	
	/*
	*Under windows we need to initialize the socket library before use it.
	*/
	WSADATA wsaData;
	return WSAStartup(ver, &wsaData);
#else
	return 0;
#endif
}
MYSERVER_SOCKET_HANDLE MYSERVER_SOCKET::getHandle()
{
	return socketHandle;
}
int MYSERVER_SOCKET::setHandle(MYSERVER_SOCKET_HANDLE h)
{
	socketHandle=h;
	return 1;
}
int MYSERVER_SOCKET::operator==(MYSERVER_SOCKET s)
{
	return socketHandle==s.socketHandle;
}
int MYSERVER_SOCKET::operator=(MYSERVER_SOCKET s)
{
	memcpy(this,&s,sizeof(s));
	return 1;
}
int MYSERVER_SOCKET::socket(int af,int type,int protocol,int useSSL)
{
	sslSocket=useSSL;
	socketHandle=::socket(af,type,protocol);
	if(sslSocket)
	{
		initializeSSL();
	}
	return	(int)socketHandle;
}
MYSERVER_SOCKET::MYSERVER_SOCKET(MYSERVER_SOCKET_HANDLE handle)
{
	setHandle(handle);
}
MYSERVER_SOCKET::MYSERVER_SOCKET()
{
	sslSocket=0;
	sslConnection=0;
	sslContext=0;
	setHandle(0);
}
int MYSERVER_SOCKET::bind(MYSERVER_SOCKADDR* sa,int namelen)
{
#ifdef WIN32	
	return ::bind((SOCKET)socketHandle,sa,namelen);
#endif
#ifdef __linux__
	return ::bind((int)socketHandle,sa,namelen);
#endif
}

int MYSERVER_SOCKET::listen(int max)
{
#ifdef WIN32
	return ::listen(socketHandle,max);
#endif
#ifdef __linux__
	return ::listen((int)socketHandle,max);
#endif
}
SSL* MYSERVER_SOCKET::getSSLConnection()
{
	return sslConnection;
}
MYSERVER_SOCKET MYSERVER_SOCKET::accept(MYSERVER_SOCKADDR* sa,int* sockaddrlen,int sslHandShake)
{

	MYSERVER_SOCKET s;
	s.sslConnection=0;
	s.sslContext=0;
	s.sslSocket=0;
#ifdef WIN32
	MYSERVER_SOCKET_HANDLE h=(MYSERVER_SOCKET_HANDLE)::accept(socketHandle,sa,sockaddrlen);
	s.setHandle(h);
#endif
#ifdef __linux__
	unsigned int Connect_Size = *sockaddrlen;
	int as = ::accept((int)socketHandle,sa,&Connect_Size);
	s.setHandle(as);
#endif

	return s;
}

int MYSERVER_SOCKET::closesocket()
{
#ifdef WIN32
	return ::closesocket(socketHandle);
#endif
#ifdef __linux__
	return ::close((int)socketHandle);
#endif
	freeSSL();
}
MYSERVER_HOSTENT *MYSERVER_SOCKET::gethostbyaddr(char* addr,int len,int type)
{
#ifdef WIN32
	HOSTENT *he=::gethostbyaddr(addr,len,type);
	return he;
#endif
#ifdef __linux__
	struct hostent * he=::gethostbyaddr(addr,len,type);
	return he;
#endif
}
MYSERVER_HOSTENT *MYSERVER_SOCKET::gethostbyname(const char *hostname)
{	
	return (MYSERVER_HOSTENT *)::gethostbyname(hostname);
}


int MYSERVER_SOCKET::shutdown(int how)
{
#ifdef WIN32
	return ::shutdown(socketHandle,how);
#endif
#ifdef __linux__
	return ::shutdown((int)socketHandle,how);
#endif
}
int	MYSERVER_SOCKET::setsockopt(int level,int optname,const char *optval,int optlen)
{
	return ::setsockopt(socketHandle,level, optname,optval,optlen);
}

int MYSERVER_SOCKET::send(const char* buffer,int len,int flags)
{
	if(sslSocket)
	{
		int err;
		do
		{	
			err=SSL_write(sslConnection,buffer,len);
		}while(SSL_get_error(sslConnection,err) ==SSL_ERROR_WANT_WRITE || SSL_get_error(sslConnection,err) == SSL_ERROR_WANT_READ);
		return err;
	}
#ifdef WIN32
	return	::send(socketHandle,buffer,len,flags);
#endif
#ifdef __linux__
	return	::send((int)socketHandle,buffer,len,flags);
#endif
}

int MYSERVER_SOCKET::ioctlsocket(long cmd,unsigned long* argp)
{
#ifdef WIN32
	return ::ioctlsocket(socketHandle,cmd,argp);
#endif
#ifdef __linux__
	int int_argp = 0;
	int ret = ::ioctl((int)socketHandle,cmd,&int_argp);
	*argp = int_argp;
	return ret;

#endif
}

int MYSERVER_SOCKET::connect(MYSERVER_SOCKADDR* sa,int na)
{
#ifdef WIN32
	return ::connect((SOCKET)socketHandle,sa,na);
#endif
#ifdef __linux__
	return ::connect((int)socketHandle,sa,na);
#endif
}
int MYSERVER_SOCKET::recv(char* buffer,int len,int flags,int timeout)
{
	int time=clock();
	while(clock()-time<timeout)
	{
		if(bytesToRead())
			return recv(buffer,len,flags);
	}
	return -1;

}
int MYSERVER_SOCKET::freeSSL()
{
	if(sslConnection)
	{
		SSL_free(sslConnection);
		sslConnection=0;
	}
	return 1;
}
int MYSERVER_SOCKET::setSSLContext(SSL_CTX* context)
{
	sslContext=context;
	return 1;
}
int MYSERVER_SOCKET::initializeSSL(SSL* connection)
{
	freeSSL();
	if(connection)
		sslConnection = connection;
	else
	{
		if(sslContext==0)
			return 0;
		sslConnection =(SSL *)SSL_new(sslContext);
	}
	return 1;
}
void MYSERVER_SOCKET::setSSL(int nSSL,SSL* connection)
{
	if(sslSocket && (nSSL==0))
		freeSSL();
	if(nSSL && (sslSocket==0))
		initializeSSL(connection);
	sslSocket=nSSL;
}
int MYSERVER_SOCKET::getSSL()
{
	return sslSocket;
}
int MYSERVER_SOCKET::recv(char* buffer,int len,int flags)
{
	int err;
	if(sslSocket)
	{
		do
		{	
			SSL_read(sslConnection,buffer,len);
		}while(SSL_get_error(sslConnection,err) ==SSL_ERROR_WANT_X509_LOOKUP || SSL_get_error(sslConnection,err) == SSL_ERROR_WANT_READ);
		if(SSL_get_error(sslConnection,err)!=SSL_ERROR_ZERO_RETURN)
			return err;
		else 
			return -1;
	}
#ifdef WIN32
	err=::recv(socketHandle,buffer,len,flags);
	if(err==SOCKET_ERROR)
		return -1;
	else 
		return err;
#endif
#ifdef __linux__
	err=::recv((int)socketHandle,buffer,len,flags);
	if(err == 0)
		err = -1;
	return err;
#endif
}

u_long MYSERVER_SOCKET::bytesToRead()
{
	u_long nBytesToRead=0;
	ioctlsocket(FIONREAD,&nBytesToRead);
	return nBytesToRead;
}
int MYSERVER_SOCKET::gethostname(char *name,int namelen)
{
	return ::gethostname(name,namelen);
}
int MYSERVER_SOCKET::getsockname(MYSERVER_SOCKADDR *ad,int *namelen)
{
#ifdef WIN32
	return ::getsockname(socketHandle,ad,namelen);
#endif
#ifdef __linux__
	unsigned int len = *namelen;
	int ret = ::getsockname((int)socketHandle,ad,&len);
	*namelen = len;
	return ret;
#endif
}
