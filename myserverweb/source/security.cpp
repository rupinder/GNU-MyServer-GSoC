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

#include "../include/security.h"
#include "../include/utility.h"
#include "../include/HTTPmsg.h"
#include "../include/connectionstruct.h"


/*
*Global values for useLogonOption flag and the guest handle.
*/
int  useLogonOption;
LOGGEDUSERID guestLoginHandle;
char guestLogin[20];
char guestPassword[32];

/*
*Do the ms_logon of an user.
*/
int ms_logonCurrentThread(char *name,char* password,LOGGEDUSERID *handle)
{
	int ms_logon=false;
#ifdef WIN32
	#ifndef LOGON32_LOGON_NETWORK
	#define LOGON32_LOGON_NETWORK 3
	#endif

	#ifndef LOGON32_PROVIDER_DEFAULT
	#define LOGON32_PROVIDER_DEFAULT
	#endif
	ms_logon=LogonUser(name,NULL,password, LOGON32_LOGON_NETWORK, LOGON32_PROVIDER_DEFAULT,(PHANDLE)(handle));
#endif
	return ms_logon;
}
/*
*Change the owner of current thread.
*/
void ms_impersonateLogonUser(LOGGEDUSERID* hImpersonation)
{
#ifdef WIN32
	ImpersonateLoggedOnUser((HANDLE)*hImpersonation);
#endif	
}

/*
*This function terminates the impersonation of a client application.
*/
void ms_revertToSelf()
{
#ifdef WIN32
	RevertToSelf();
#endif
}

/*
*Close the handle of a logged user.
*/
void ms_cleanLogonUser(LOGGEDUSERID* hImpersonation)
{
#ifdef WIN32
	CloseHandle((HANDLE)*hImpersonation);
#endif
}
/*
*Change the owner of the thread with the connection login and password informations.
*/
void ms_logon(LPCONNECTION c,int *logonStatus,LOGGEDUSERID *hImpersonation)
{
	*hImpersonation=0;
	if(useLogonOption)
	{
		if(c->login[0])
		{
			*logonStatus=ms_logonCurrentThread(c->login,c->password,hImpersonation);
		}
		else
		{
			*logonStatus=false;
			*hImpersonation=guestLoginHandle;
		}
		ms_impersonateLogonUser(hImpersonation);
	}
	else
	{
		*logonStatus=false;
	}
}
/*
*Logout the hImpersonation handle.
*/
void ms_logout(int /*ms_logon*/,LOGGEDUSERID *hImpersonation)
{
	if(useLogonOption)
	{
		ms_revertToSelf();
		if(*hImpersonation)
		{
			ms_cleanLogonUser(hImpersonation);
			hImpersonation=0;
		}
	}
}
/*
*Do the ms_logon of the guest client.
*/
void ms_logonGuest()
{
#ifdef WIN32
	if(useLogonOption)
		LogonUser(guestLogin,NULL,guestPassword,LOGON32_LOGON_NETWORK, LOGON32_PROVIDER_DEFAULT,(PHANDLE)&guestLoginHandle);
#endif
}
