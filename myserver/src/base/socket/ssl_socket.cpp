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
#include <include/base/utility.h>
#include "include/base/socket/ssl_socket.h"

extern "C"
{
#include <string.h>
#include <stdio.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/ioctl.h>
#ifndef WIN32
# include <netdb.h>
# include <unistd.h>
# include <netinet/in.h>
# include <arpa/inet.h>
#endif
}

#include <sstream>

using namespace std;

/*!
 *Constructor of the class.
 */
SslSocket::SslSocket (Socket* sock) : Socket (sock)
{
  this->sock = sock;
  sslConnection = 0;
  sslContext = 0;
  clientCert = 0;
  sslMethod = 0;
  externalContext = false;
}

SslSocket::SslSocket ()
{
  this->sock = NULL;
  sslConnection = 0;
  sslContext = 0;
  clientCert = 0;
  sslMethod = 0;
  externalContext = false;
}

SslSocket::~SslSocket ()
{
  freeSSL ();
}

/*!
 *Close the socket.
 */
int SslSocket::close ()
{
  freeSSL ();
  return Socket::close ();
}

/*!
 *Shutdown the socket.
 */
int SslSocket::shutdown (int how)
{
  if (sslConnection)
    SSL_shutdown (sslConnection);

  return ::shutdown (fd, how);
}

/*!
 *Send data over the socket.
 *Return -1 on error.
 *This routine is accessible only from the Socket class.
 */
int SslSocket::rawSend (const char* buffer, int len, int flags)
{
  int err;
  do
    {
      err = SSL_write (sslConnection, buffer, len);
    }while ((err <= 0) &&
            (SSL_get_error (sslConnection,err) == SSL_ERROR_WANT_WRITE
             || SSL_get_error (sslConnection,err) == SSL_ERROR_WANT_READ));
  if (err <= 0)
    return -1;
  else
    return err;
}

/*!
 *Connect the socket.
 */
int SslSocket::connect (MYSERVER_SOCKADDR* sa, int na)
{
  if ( sa == NULL || (sa->ss_family != AF_INET && sa->ss_family != AF_INET6) )
    return 1;
  if ( (sa->ss_family == AF_INET && na != sizeof (sockaddr_in))
#if HAVE_IPV6
       || (sa->ss_family == AF_INET6 && na != sizeof (sockaddr_in6))
#endif
       )
    return 1;

  sslMethod = SSLv23_client_method ();
  /*! Create the local context. */
  sslContext = SSL_CTX_new (sslMethod);
  if (sslContext == 0)
    return -1;

  /*! Do the TCP connection.  */
  if (::connect (fd, (sockaddr *) sa, na))
    {
      SSL_CTX_free (sslContext);
      sslContext = 0;
      return -1;
    }
  sslConnection = SSL_new (sslContext);
  if (sslConnection == 0)
    {
      SSL_CTX_free (sslContext);
      sslContext = 0;
      return -1;
    }
  SSL_set_fd (sslConnection, fd);
  if (SSL_connect (sslConnection) < 0)
    {
      SSL_CTX_free (sslContext);
      sslContext = 0;
      return -1;
    }

  externalContext = false;
  return 0;
}

/*!
 *Set the SSL context.
 */
int SslSocket::setSSLContext (SSL_CTX* context)
{
  sslContext = context;
  externalContext = true;
  return 1;
}

/*!
 *Free the SSL connection.
 */
int SslSocket::freeSSL ()
{
  /*! free up the SSL context. */
  if (sslConnection)
    {
      SSL_free (sslConnection);
      sslConnection = 0;
    }

  if (sslContext && !externalContext)
    {
      SSL_CTX_free (sslContext);
      sslContext = 0;
    }
  return 1;
}


/*!
 *Returns the SSL connection.
 */
SSL* SslSocket::getSSLConnection ()
{
  return sslConnection;
}

/*!
 *SSL handshake procedure.
 *Return nonzero on errors.
 */
int SslSocket::sslAccept ()
{
  int sslAccept;
  if (sslContext == 0)
    return -1;
  if (sslConnection)
    freeSSL ();
  sslConnection = SSL_new (sslContext);
  if (sslConnection == 0)
    {
      freeSSL ();
      return -1;
    }

  if (SSL_set_fd (sslConnection, fd) == 0)
    {
      shutdown (2);
      freeSSL ();
      return -1;
    }

  do
    {
      sslAccept = SSL_accept (sslConnection);
    }while (sslAccept != 1
            && SSL_get_error (sslConnection, sslAccept) == SSL_ERROR_WANT_READ);

  if (sslAccept != 1 )
    {
      shutdown (2);
      freeSSL ();
      return -1;
    }

  clientCert = SSL_get_peer_certificate (sslConnection);

  return 0;
}


/*!
 *Receive data from the socket.
 *Returns -1 on errors.
 */
int SslSocket::recv (char* buffer, int len, int flags)
{
  int err = 0;

  if (sslConnection)
    {
      for (;;)
        {
          int sslError;
          err = SSL_read (sslConnection, buffer, len);

          if (err > 0)
            break;

          sslError = SSL_get_error (sslConnection, err);

          if ((sslError != SSL_ERROR_WANT_READ) &&
              (sslError != SSL_ERROR_WANT_WRITE))
            break;
        }

      if (err <= 0)
        return -1;
      else
        return err;
    }

  return 0;
}

/*!
 *Returns the number of bytes waiting to be read.
 */
u_long SslSocket::bytesToRead ()
{
  u_long nBytesToRead = 0;

  nBytesToRead = SSL_pending (sslConnection);

  if (nBytesToRead)
    return nBytesToRead;

  return Socket::bytesToRead ();
}

/*!
 *Check if there is data ready to be read.
 */
int SslSocket::dataOnRead (int sec, int usec)
{
  if (bytesToRead ())
    return 1;

  return Socket::dataOnRead (sec, usec);
}

