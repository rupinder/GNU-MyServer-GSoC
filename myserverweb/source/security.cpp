#include "..\include\security.h"
BOOL logonCurrentThread(char *name,char* password,PHANDLE handle)
{
	BOOL logon;
#ifdef WIN32
	logon=LogonUser(name,NULL,password, LOGON32_LOGON_NETWORK, LOGON32_PROVIDER_DEFAULT,handle);
#endif
	return logon;
}
VOID impersonateLogonUser(HANDLE hImpersonation)
{
#ifdef WIN32
	ImpersonateLoggedOnUser(hImpersonation);
#endif	
}
VOID revertToSelf()
{
#ifdef WIN32
	RevertToSelf();
#endif
}

VOID cleanLogonUser(HANDLE hImpersonation)
{
#ifdef WIN32
	CloseHandle(hImpersonation);
#endif
}