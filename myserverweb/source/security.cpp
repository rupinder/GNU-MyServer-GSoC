/*
*myServer
*Copyright (C) 2002 Giuseppe Scrivano
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


#include "../include/security.h"
#include "../include/utility.h"
#include "../include/HTTPmsg.h"
#include "../include/connectionstruct.h"


/*
*Global values for useLogonOption flag and the guest handle.
*/
int  useLogonOption;

/*
*Do the logon of an user.
*/
int logonCurrentThread(char *name,char* password,LOGGEDUSERID *handle)
{
	int logon=false;
#ifdef WIN32
	#ifndef LOGON32_LOGON_NETWORK
	#define LOGON32_LOGON_NETWORK 3
	#endif

	#ifndef LOGON32_PROVIDER_DEFAULT
	#define LOGON32_PROVIDER_DEFAULT
	#endif
	logon=LogonUser(name,NULL,password, LOGON32_LOGON_NETWORK, LOGON32_PROVIDER_DEFAULT,(PHANDLE)(handle));
#endif
	return logon;
}
/*
*Change the owner of current thread.
*/
void impersonateLogonUser(LOGGEDUSERID* hImpersonation)
{
#ifdef WIN32
	ImpersonateLoggedOnUser((HANDLE)*hImpersonation);
#endif	
}

/*
*This function terminates the impersonation of a client application.
*/
void revertToSelf()
{
#ifdef WIN32
	RevertToSelf();
#endif
}

/*
*Close the handle of a logged user.
*/
void cleanLogonUser(LOGGEDUSERID* hImpersonation)
{
#ifdef WIN32
	CloseHandle((HANDLE)*hImpersonation);
#endif
}
/*
*Change the owner of the thread with the connection login and password informations.
*/
void logon(LPCONNECTION c,int *logonStatus,LOGGEDUSERID *hImpersonation)
{
	*hImpersonation=0;
	if(useLogonOption)
	{
		if(c->login[0])
		{
			*logonStatus=logonCurrentThread(c->login,c->password,hImpersonation);
		}
		else
		{
			*logonStatus=false;
			*hImpersonation=0;
		}
		impersonateLogonUser(hImpersonation);
	}
	else
	{
		*logonStatus=false;
	}
}
/*
*Logout the hImpersonation handle.
*/
void logout(int /*logon*/,LOGGEDUSERID *hImpersonation)
{
	if(useLogonOption)
	{
		revertToSelf();
		if(*hImpersonation)
		{
			cleanLogonUser(hImpersonation);
			hImpersonation=0;
		}
	}
}
/*
*Do the logon of the guest client.
*/
void logonGuest()
{
#ifdef WIN32
//	if(useLogonOption)
//		LogonUser("Guest",NULL,"guest",LOGON32_LOGON_NETWORK, LOGON32_PROVIDER_DEFAULT,(PHANDLE)&guestLoginHandle);
#endif
}
