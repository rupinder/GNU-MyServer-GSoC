/*
MyServer
Copyright (C) 2002, 2003, 2004 The MyServer Team
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

#ifndef SECURITY_H
#define SECURITY_H

#include "../stdafx.h"
#include "../include/connection.h"
#include "../include/xml_parser.h"
#include "../include/http_headers.h"
#ifndef DO_NOT_USE_SSL
#include<openssl/ssl.h>
#include<openssl/crypto.h>
#include<openssl/lhash.h>
#include<openssl/err.h>
#include<openssl/bn.h>
#include<openssl/pem.h>
#include<openssl/x509.h>
#include<openssl/rand.h>
#include<openssl/pem.h>
#include<openssl/err.h>
#include<openssl/rsa.h>
#endif

#include <string>

using namespace std;

/*!
 *Various permissions flags.
 */

/*! Current user can read the file. */
const u_long MYSERVER_PERMISSION_READ     =	(1<<0);

/*! Current user can write to the file. */
const u_long MYSERVER_PERMISSION_WRITE	  = (1<<1);

/*! Current user can execute the file. */
const u_long MYSERVER_PERMISSION_EXECUTE  = (1<<2);

/*! Current user can remove the file. */
const u_long MYSERVER_PERMISSION_DELETE	  = (1<<3);

/*! Current user can browse the directory content. */
const u_long MYSERVER_PERMISSION_BROWSE	  = (1<<4);

struct SecurityToken
{
  /*! User to check for. */
  const char* user;
  /*! Password provided by the user. */
  const char* password;
  /*! Directory that the user is in. */
  const char* directory;
  /*! System directory for the host. */
  const char* sysdirectory;
  /*! File that the user tried to access. */
  const char* filename;
  /*! 
   *Password that the user should provide to have access. 
   *This is used in authorization schemes like the HTTP digest,
   *where the password is not sent in clear on the network.
   */
  char *password2;
  /*! Permission mask that the user will have if providing a [password2]. */
  int *permission2;

  struct HttpThreadContext* context;

  /*! Authorization scheme to use. */
  char* auth_type;
  /*! Length for the [auth_type] allocated string. */
  int len_auth;
  /*! Throttling rate to use with specified user. */
  int throttlingRate;

	HashMap<string,string*> *otherValues;

  HttpThreadContext* td;

  SecurityToken();
  void reset();
};

class SecurityManager
{
public:
  SecurityManager();
  ~SecurityManager();
  int getErrorFileName(const char *sysDir, int error, string& out, 
                       XmlParser* parser=0);
  int getPermissionMask(SecurityToken* st, XmlParser* parser=0);
};
#endif
