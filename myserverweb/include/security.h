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

#ifndef SECURITY_H
#define SECURITY_H

#include "../stdafx.h"
#include "../include/connectionstruct.h"
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
extern int useLogonOption;
/*!
*Various allowable permissions
*/
#define MYSERVER_PERMISSION_READ	(1)
#define MYSERVER_PERMISSION_WRITE	(2)
#define MYSERVER_PERMISSION_EXECUTE	(4)
#define MYSERVER_PERMISSION_DELETE	(8)
#define MYSERVER_PERMISSION_BROWSE	(16)

/*!
*Change the ownner of the caller thread.
*/
int logonCurrentThread(char*,char*,LOGGEDUSERID*);
/*!
*Change the ownner of the caller thread to the runner of the process.
*/
void revertToSelf();
/*!
*Impersonate the logon user.
*/
void impersonateLogonUser(LOGGEDUSERID* hImpersonation);
/*!
*Close the handle.
*/
void cleanLogonUser(LOGGEDUSERID* hImpersonation);

void logon(LPCONNECTION c,int *logonStatus,LOGGEDUSERID *hImpersonation);
void logout(int logon,LOGGEDUSERID *hImpersonation);

int getErrorFileName(char *root,int error,char* out);
int getPermissionMask(char* user, char* password,char* folder,char* filename,char *sysfolder=0,char *password2=0,char* auth_type=0,int len_auth=0,int *permission2=0);

#endif
