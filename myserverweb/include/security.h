#include "..\stdafx.h"
/*
*Change the ownner of the caller thread
*/
BOOL logonCurrentThread(char*,char*,PHANDLE);
/*
*Change the ownner of the caller thread to the runner of the process
*/
VOID revertToSelf();
/*
*Impersonate the logon user
*/
VOID impersonateLogonUser(HANDLE hImpersonation);
/*
*Close the handle
*/
VOID cleanLogonUser(HANDLE hImpersonation);