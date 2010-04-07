/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008, 2009, 2010 Free
Software Foundation, Inc.
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


#include "myserver.h"

#include <include/base/socket/socket.h>
#include <include/base/utility.h>

#include <sys/ioctl.h>
#include <errno.h>

#ifndef WIN32
# include <unistd.h>
# include <netdb.h>
# include <netinet/in.h>
# include <arpa/inet.h>
#endif


#include <sstream>

using namespace std;

/*!
 *Source code to wrap the socket library to MyServer project.
 */
int Socket::startupSocketLib ()
{
#ifdef WIN32
  static bool done = false;
  if (!done)
    {
      WSADATA wsaData;
      done = true;
      return WSAStartup (MAKEWORD (1, 1), &wsaData);
    }
#endif
  return 0;
}

/*!
 *Returns the socket handle
 */
Handle Socket::getHandle ()
{
  return (Handle) fd;
}

/*!
 *Set the handle for the socket
 */
void Socket::setHandle (SocketHandle h)
{
  fd = h;
}

/*!
 *Check if the two sockets have the same handle descriptor
 */
int Socket::operator==(Socket* s)
{
  return fd == s->fd;
}

/*!
 *Set the socket using the = operator
 */
int Socket::operator=(Socket* s)
{
  fd = s->fd;
  serverSocket = s->serverSocket;
  throttlingRate = s->throttlingRate;
  isNonBlocking = s->isNonBlocking;
  return 0;
}
/*!
 *Create the socket.
 */
int Socket::socket (int af, int type, int protocol)
{
  fd = gnulib::socket (af, type, protocol);
  return fd;
}

/*!
 * C'tor.
 */
Socket::Socket (SocketHandle handle)
{
  throttlingRate = 0;
  setHandle (handle);
  isNonBlocking = false;
}

/*!
 * C'tor.
 */
Socket::Socket (Socket* socket)
{
  fd = socket->fd;
  serverSocket = socket->serverSocket;
  throttlingRate = socket->throttlingRate;
  isNonBlocking = socket->isNonBlocking;
}

Socket::~Socket ()
{
  close ();
}

/*!
 *Return the throttling rate (bytes/second) used by the socket. A return value
 *of zero means that no throttling is used.
 */
u_long Socket::getThrottling ()
{
  return throttlingRate;
}

/*!
 *Set the throttling rate (bytes/second) for the socket.
 *Use a zero rate to disable throttling.
 */
void Socket::setThrottling (u_long tr)
{
  throttlingRate = tr;
}

/*!
 *Constructor of the class.
 */
Socket::Socket ()
{
  /*! Reset everything.  */
  throttlingRate = 0;
  serverSocket = NULL;
  setHandle (-1);
}

/*!
 *Bind the port to the socket.
 */
int Socket::bind (MYSERVER_SOCKADDR* sa, int namelen)
{
  if ( sa == NULL )
    return -1;

  if ((sa->ss_family == AF_INET && namelen != sizeof (sockaddr_in))
#if HAVE_IPV6
      || (sa->ss_family == AF_INET6 && namelen != sizeof (sockaddr_in6))
#endif
      )
    return -1;

  return gnulib::bind (fd, (struct sockaddr*) sa, namelen);
}

/*!
 *Listen for other connections.
 */
int Socket::listen (int max)
{
  return gnulib::listen (fd, max);
}

/*!
 *Accept a new connection.
 */
Socket* Socket::accept (MYSERVER_SOCKADDR* sa, socklen_t* sockaddrlen)
{
  int acceptedHandle = gnulib::accept (fd, (struct sockaddr *)sa,
                                       sockaddrlen);

  if (acceptedHandle >= 0)
    return new Socket ((SocketHandle) acceptedHandle);
  else
    return NULL;
}

/*!
 *Close the socket.
 */
int Socket::close ()
{
  int ret = -1;
  if (fd >= 0)
    ret = gnulib::close (fd);

  fd = -1;
  return ret;
}

/*!
 *Returns an host by its address.
 */
MYSERVER_HOSTENT *Socket::gethostbyaddr (char* addr, int len, int type)
{
  return (MYSERVER_HOSTENT *) ::gethostbyaddr (addr, len, type);
}

/*!
*Returns an host by its name
*/
MYSERVER_HOSTENT *Socket::gethostbyname (const char *hostname)
{
  return (MYSERVER_HOSTENT *) ::gethostbyname (hostname);
}

/*!
 *Shutdown the socket.
 */
int Socket::shutdown (int how)
{
  return gnulib::shutdown (fd, how);
}

/*!
 *Set socket options.
 */
int  Socket::setsockopt (int level, int optname,
                       const char *optval, int optlen)
{
  return gnulib::setsockopt (fd, level, optname, optval, optlen);
}

/*!
 * Fill the out string with a list of the local IPs. Returns 0 on success.
 */
int Socket::getLocalIPsList (string &out)
{
  char serverName[HOST_NAME_MAX + 1];
  memset (serverName, 0, HOST_NAME_MAX + 1);

  Socket::gethostname (serverName, HOST_NAME_MAX);
#if HAVE_IPV6
  addrinfo aiHints = { 0 }, *pHostInfo = NULL, *pCrtHostInfo = NULL;
  /* only interested in socket types the that server will listen to.  */
  aiHints.ai_socktype = SOCK_STREAM;
  if ( getaddrinfo (serverName, NULL, &aiHints, &pHostInfo) == 0 &&
       pHostInfo != NULL )
    {
      sockaddr_storage *pCurrentSockAddr = NULL;
      char straddr[NI_MAXHOST] = "";
      memset (straddr, 0, NI_MAXHOST);
      ostringstream stream;
      for ( pCrtHostInfo = pHostInfo; pCrtHostInfo != NULL;
            pCrtHostInfo = pCrtHostInfo->ai_next )
        {
          pCurrentSockAddr =
            reinterpret_cast<sockaddr_storage *>(pCrtHostInfo->ai_addr);
          if ( pCurrentSockAddr == NULL )
            continue;

          if ( !getnameinfo (reinterpret_cast<sockaddr *>(pCurrentSockAddr),
                             sizeof (sockaddr_storage), straddr, NI_MAXHOST,
                             NULL, 0, NI_NUMERICHOST) )
            {
              stream << ( !stream.str ().empty () ? ", " : "" ) << straddr;
            }
          else
            return -1;
        }
      out.assign (stream.str ());
      freeaddrinfo (pHostInfo);
    }
  return 0;
#else// !HAVE_IPV6

  MYSERVER_HOSTENT *localhe;
  in_addr ia;
  localhe = Socket::gethostbyname (serverName);

  if (localhe)
    {
      ostringstream stream;
      int i;
      /*! Print all the interfaces IPs. */
      for (i = 0; (localhe->h_addr_list[i]); i++)
        {
# ifdef WIN32
          ia.S_un.S_addr = *((u_long FAR *) (localhe->h_addr_list[i]));
# else
          ia.s_addr = *((u_long *) (localhe->h_addr_list[i]));
# endif
          stream << ((i != 0) ? ", " : "") << inet_ntoa (ia);
        }

      out.assign (stream.str ());
      return 0;
    }
  else
    {
      out.assign (LOCALHOST_ADDRESS);
      return 0;
    }
#endif//HAVE_IPV6
  return -1;
}

/*!
 *Send data over the socket.
 *Return -1 on error.
 *This routine is accessible only from the Socket class.
 */
int Socket::rawSend (const char* buffer, int len, int flags)
{
  return gnulib::send (fd, buffer, len, flags);
}

/*!
 *Send data over the socket.
 *Returns -1 on error.
 *Returns the number of bytes sent on success.
 *If a throttling rate is specified, send will use it.
 */
int Socket::send (const char* buffer, int len, int flags)
{
  u_long toSend = (u_long) len;
  int ret;
  /*! If no throttling is specified, send only one big data chunk. */
  if (throttlingRate == 0)
  {
    ret = rawSend (buffer, len, flags);
    return ret;
  }
  else
  {
    while (1)
      {
        /*! When we can send data again?  */
        u_long time = getTicks () + (1000 * 1024 / throttlingRate) ;
        /*! If a throttling rate is specified, send chunks of 1024 bytes.  */
        ret = rawSend (buffer + (len - toSend), toSend < 1024 ?
                       toSend : 1024, flags);
        /*! On errors returns directly -1.  */
        if (ret < 0)
          return -1;
        toSend -= (u_long) ret;
        /*!
         *If there are other bytes to send wait before cycle again.
         */
        if (toSend)
          {
            Thread::wait (getTicks () - time);
          }
        else
          break;
      }
    /*! Return the number of sent bytes. */
    return len - toSend;
  }
  return 0;
}

/*!
 *Function used to control the socket.
 */
int Socket::ioctlsocket (long cmd, unsigned long* argp)
{
  return gnulib::ioctl (fd, cmd, argp);
}

/*!
 *Connect to the specified host:port.
 *Returns zero on success.
 */
int Socket::connect (const char* host, u_short port)
{
  if ( host == NULL )
    return -1;

#if HAVE_IPV6
  MYSERVER_SOCKADDRIN thisSock = { 0 };
  int nLength = sizeof (MYSERVER_SOCKADDRIN);
  int nSockLen = 0;
  bool bSocketConnected = false;
  int nGetaddrinfoRet = 0;
  addrinfo *pHostInfo = NULL, *pCrtHostInfo = NULL;
  MYSERVER_SOCKADDRIN *pCurrentSockAddr = NULL, connectionSockAddrIn = { 0 };
  char szPort[10];

  addrinfo aiHints = { 0 };

  /*
   *If the socket is not created, wait to see what address families
   *have this host, than create socket.
   */

  if ( getsockname (&thisSock, &nLength) == 0 )
    {
      aiHints.ai_family = thisSock.ss_family;
      aiHints.ai_socktype = SOCK_STREAM;
    }

  memset (szPort, 0, sizeof (char)*10);
  gnulib::snprintf (szPort, 10, "%d", port);

  if (aiHints.ai_family != 0)
    nGetaddrinfoRet = getaddrinfo (host, NULL, &aiHints, &pHostInfo);
  else
    nGetaddrinfoRet = getaddrinfo (host, NULL, NULL, &pHostInfo);
  if (nGetaddrinfoRet != 0 || pHostInfo == NULL )
    return -1;

  for ( pCrtHostInfo = pHostInfo; pCrtHostInfo != NULL;
        pCrtHostInfo = pCrtHostInfo->ai_next )
    {
      pCurrentSockAddr = reinterpret_cast<sockaddr_storage *>
        (pCrtHostInfo->ai_addr);
      if ( pCurrentSockAddr == NULL ||
           (thisSock.ss_family != 0 &&
            pCurrentSockAddr->ss_family != thisSock.ss_family) )
        continue;

      if ( thisSock.ss_family == 0 )
        {
          if ( pCurrentSockAddr->ss_family == AF_INET )
            {
              if (Socket::socket (AF_INET, SOCK_STREAM, 0) == -1)
                continue;
            }
          else if ( pCurrentSockAddr->ss_family == AF_INET6 )
            {
              if (Socket::socket (AF_INET6, SOCK_STREAM, 0) == -1)
                continue;
            }
          if ( getsockname (&thisSock, &nLength) != 0 )
            return -1;
        }

      memset (&connectionSockAddrIn, 0, sizeof (connectionSockAddrIn));
      connectionSockAddrIn.ss_family = pCurrentSockAddr->ss_family;

      if ( connectionSockAddrIn.ss_family != AF_INET &&
           connectionSockAddrIn.ss_family != AF_INET6 )
        continue;

      if ( connectionSockAddrIn.ss_family == AF_INET )
        {
          nSockLen = sizeof (sockaddr_in);
          memcpy ((sockaddr_in *)&connectionSockAddrIn,
                  (sockaddr_in *)pCurrentSockAddr, nSockLen);
          ((sockaddr_in *)(&connectionSockAddrIn))->sin_port = htons (port);

        }
      else// if ( connectionSockAddrIn.ss_family == AF_INET6 )
        {
          nSockLen = sizeof (sockaddr_in6);
          memcpy ((sockaddr_in6 *)&connectionSockAddrIn,
                  (sockaddr_in6 *)pCurrentSockAddr, nSockLen);
          ((sockaddr_in6 *)&connectionSockAddrIn)->sin6_port = htons (port);
        }

      if (Socket::connect ((MYSERVER_SOCKADDR*)(&connectionSockAddrIn),
                           nSockLen) == -1)
        Socket::close ();
      else
        {
          bSocketConnected = true;
          break;
        }
    }
  freeaddrinfo (pHostInfo);
  if ( !bSocketConnected )
    return -1;
#else
  MYSERVER_HOSTENT *hp = Socket::gethostbyname (host);
  struct sockaddr_in sockAddr;
  int sockLen;
  if (hp == 0)
    return -1;

  /*! If the socket is not created, create it before use. */
  if (fd == -1 &&
      Socket::socket (AF_INET, SOCK_STREAM, 0) == -1)
    return -1;

  sockLen = sizeof (sockAddr);
  memset (&sockAddr, 0, sizeof (sockAddr));
  sockAddr.sin_family = AF_INET;
  memcpy (&sockAddr.sin_addr, hp->h_addr, hp->h_length);
  sockAddr.sin_port = htons (port);
  if (Socket::connect ((MYSERVER_SOCKADDR*)&sockAddr, sockLen) == -1)
    {
      Socket::close ();
      return -1;
    }
#endif

  return 0;
}

/*!
 *Connect the socket.
 */
int Socket::connect (MYSERVER_SOCKADDR* sa, int na)
{
  if ( sa == NULL )
    return -1;

  if ( (sa->ss_family == AF_INET && na != sizeof (sockaddr_in))
#if HAVE_IPV6
    || (sa->ss_family == AF_INET6 && na != sizeof (sockaddr_in6))
#endif
 )
    return -1;

  return gnulib::connect (fd, (sockaddr *) sa, na);
}

/*!
 *Receive data from the socket.
 */
int Socket::recv (char* buffer, int len, int flags, u_long timeout)
{
  int ret = dataAvailable (timeout / 1000, timeout % 1000);

  if (ret < 0)
    return ret;

  if (ret)
    return recv (buffer, len, flags);

  return 0;
}

/*!
 *Receive data from the socket.
 *Returns -1 on errors.
 */
int Socket::recv (char* buffer,int len,int flags)
{
  int err = 0;

  err = gnulib::recv (fd, buffer, len, flags);

  if ( err < 0 && errno == EAGAIN && isNonBlocking)
    return 0;

  if (err == 0)
    err = -1;

  return err;
}

/*!
 *Returns the number of bytes waiting to be read.
 */
u_long Socket::bytesToRead ()
{
  u_long nBytesToRead = 0;

#ifdef FIONREAD
  ioctlsocket (FIONREAD,&nBytesToRead);
#else
# ifdef I_NREAD
  ::ioctlsocket ( I_NREAD, &nBytesToRead ) ;
# endif
#endif
  return nBytesToRead;
}

/*!
 * Change the socket behaviour when an operation can't be completed
 * immediately.
 * If the socket is configured to be non blocking then it will return
 * immediately the control to the caller function.
 * A blocking socket will wait until the operation can be performed.
 * \param nonBlocking Nonzero to configure the socket non blocking.
 */
int Socket::setNonBlocking (int nonBlocking)
{
  int ret = -1;
  int flags;

#ifdef WIN32
  u_long nonblock = nonBlocking ? 1 : 0;
  ret = ioctlsocket (FIONBIO, &nonblock);
#else
  flags = gnulib::fcntl (fd, F_GETFL, 0);
  if (flags < 0)
    return -1;

  if (nonBlocking)
    flags |= O_NONBLOCK;
  else
    flags &= ~O_NONBLOCK;

  ret = gnulib::fcntl (fd, F_SETFL, flags);

  isNonBlocking = nonBlocking ? true : false;
#endif

  return ret;
}

/*!
 *Returns the hostname.
 */
int Socket::gethostname (char *name, int namelen)
{
  return gnulib::gethostname (name,namelen);
}

/*!
 *Returns the sockname.
 */
int Socket::getsockname (MYSERVER_SOCKADDR *ad, int *namelen)
{
  socklen_t len =(socklen_t) *namelen;
  int ret = gnulib::getsockname (fd, (struct sockaddr *)ad, &len);
  *namelen = (int)len;
  return ret;
}

/*!
 *Set the socket used by the server.
 */
void Socket::setServerSocket (Socket* sock)
{
  serverSocket = sock;
}
/*!
 *Returns the server socket.
 */
Socket* Socket::getServerSocket ()
{
  return serverSocket;
}

/*!
 *Check if there is data ready to be read.
 *Returns 1 if there is data to read, 0 if not.
 */
int Socket::dataAvailable (int sec, int usec)
{
  struct timeval tv;
  fd_set readfds;
  int ret;
  tv.tv_sec = sec;
  tv.tv_usec = usec;

  FD_ZERO (&readfds);
  FD_SET (fd, &readfds);

  ret = gnulib::select (fd + 1, &readfds, NULL, NULL, &tv);
  if (ret <= 0)
    return 0;

  if (FD_ISSET (fd, &readfds))
    return 1;

  return 0;
}

/*!
 *Inherited from Stream.
 *Return zero on success, or a negative number on error.
 */
int Socket::read (char* buffer, u_long len, u_long *nbr)
{
  int ret = recv (buffer, len, 0);
  if (ret < 0)
    return ret;

  *nbr = static_cast<u_long> (ret);
  return 0;
}

/*!
 *Inherited from Stream.
 *Return zero on success, or a negative number on error.
 */
int Socket::write (const char* buffer, u_long len, u_long *nbw)
{
  int ret = send (buffer, len, 0);
  if (ret < 0)
    return ret;

  *nbw = static_cast<u_long> (ret);
  return 0;
}
