/*
*myServer
*Copyright (C) Rocky_10_Balboa
*This library is free software; you can redistribute it and/or
*modify it under the terms of the GNU Library General Public
*License as published by the Free Software Foundation; either
*version 2 of the License, or (at your option) any later version.

*This library is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*Library General Public License for more details.

*You should have received a copy of the GNU Library General Public
*License along with this library; if not, write to the
*Free Software Foundation, Inc., 59 Temple Place - Suite 330,
*Boston, MA  02111-1307, USA.
*/
#pragma once

#include "..\stdafx.h"
#include "..\include\connectionstruct.h"

extern int useLogonOption;
typedef  void* LOGGEDUSERID;
extern LOGGEDUSERID guestLoginHandle;
extern char guestLogin[20];
extern char  guestPassword[32];


/*
*Change the ownner of the caller thread.
*/
int ms_logonCurrentThread(char*,char*,LOGGEDUSERID*);
/*
*Change the ownner of the caller thread to the runner of the process.
*/
VOID ms_revertToSelf();
/*
*Impersonate the ms_logon user.
*/
VOID ms_impersonateLogonUser(LOGGEDUSERID* hImpersonation);
/*
*Close the handle.
*/
VOID ms_cleanLogonUser(LOGGEDUSERID* hImpersonation);

VOID ms_logon(LPCONNECTION c,int *logonStatus,LOGGEDUSERID *hImpersonation);
VOID ms_logout(int ms_logon,LOGGEDUSERID *hImpersonation);

VOID ms_logonGuest();