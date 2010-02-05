/* -*- mode: c++ -*- */
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

#ifndef SSL_SOCKET_H
# define SSL_SOCKET_H

# include "myserver.h"

extern "C"
{
#  include <sys/types.h>
#  include <sys/socket.h>
}

# include <include/base/socket/socket.h>
# include <include/base/ssl/ssl.h>

# include <string>
using namespace std;

# include <gnutls/openssl.h>


# ifndef WIN32
extern "C" {
#  include <sys/ioctl.h>
#  include <netinet/in.h>
#  include <netdb.h>
#  include <stdio.h>
#  include <unistd.h>
}
# endif

class SslSocket : public Socket
{
public:

  int setSSLContext (SSL_CTX*);
  int sslAccept ();

  int freeSSL ();
  SSL* getSSLConnection ();

  virtual int close ();
  virtual int shutdown (int how);
  virtual int connect (MYSERVER_SOCKADDR* sa, int na);
  virtual int recv (char* buffer,int len,int flags);
  virtual int rawSend (const char* buffer, int len, int flags);
  virtual u_long bytesToRead ();

# ifdef __HURD__
  virtual int dataOnRead (int sec = 1, int usec = 500);
# else
  virtual int dataOnRead (int sec = 0, int usec = 500);
# endif

  SslSocket (Socket*);
  virtual ~SslSocket ();

protected:
  bool externalContext;
  Socket* socket;
  SSL *sslConnection;
  SSL_CTX *sslContext;
  const X509 *clientCert;

  /*! This is used only by clients sockets.  */
  SSL_METHOD* sslMethod;
};


#endif
