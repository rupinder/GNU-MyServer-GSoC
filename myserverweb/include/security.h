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

#ifndef SECURITY_H
#define SECURITY_H

#include "../stdafx.h"
#include "../include/connectionstruct.h"
#include "../include/cXMLParser.h"
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


class SecurityManager
{
public:
  SecurityManager();
  ~SecurityManager();
  int getErrorFileName(char *sysDir,int error,char** out,XmlParser* parser=0);
  int getPermissionMask(char* user, char* password,char* directory,
                        char* filename,char *sysdirectory=0,char *password2=0,
                        char* auth_type=0,int len_auth=0,int *permission2=0, 
                        XmlParser* parser=0);
};
#endif
