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
#include "../include/isapi.h"

static  max_Connections;
static ConnTableRecord *connTable;
static CRITICAL_SECTION GetTableEntryCritSec;
#define ISAPI_TIMEOUT (3600 * 1000)
/*
*Initialize the ISAPI engine.
*/
void initISAPI()
{
	InitializeCriticalSection(&GetTableEntryCritSec);	
	max_Connections=lserver->getNumThreads();
	connTable=new ConnTableRecord[max_Connections];
	ZeroMemory(connTable,sizeof(ConnTableRecord)*max_Connections);
}
void cleanupISAPI()
{
	DeleteCriticalSection(&GetTableEntryCritSec);
	if(connTable)
		delete []connTable;
}

/*
*Main procedure to call an ISAPI module.
*/
int sendISAPI(httpThreadContext* td,LPCONNECTION connection,char* scriptpath,char* /*ext*/,char *cgipath)
{
	DWORD FileAttr, Ret;
	EXTENSION_CONTROL_BLOCK ExtCtrlBlk;
	HMODULE AppHnd;
	HSE_VERSION_INFO Ver;
	int connIndex;
	PFN_GETEXTENSIONVERSION GetExtensionVersion;
	PFN_HTTPEXTENSIONPROC HttpExtensionProc;

	EnterCriticalSection(&GetTableEntryCritSec);
	connIndex = 0;
	while ((connTable[connIndex].Allocated != FALSE) && (connIndex < max_Connections)) 
	{
		connIndex++;
	}
	LeaveCriticalSection(&GetTableEntryCritSec);

	if (connIndex == max_Connections) 
	{
		return raiseHTTPError(td,connection,e_501);
	}
	connTable[connIndex].connection = connection;
	connTable[connIndex].td = td;
	connTable[connIndex].ISAPIDoneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	AppHnd = LoadLibrary(cgipath);
	if (AppHnd == NULL) 
	{
		warningsLogWrite("Failure to load ISAPI application module: ");
		warningsLogWrite(cgipath);
		warningsLogWrite("\r\n");
		return raiseHTTPError(td,connection,e_501);
	}

	GetExtensionVersion = (PFN_GETEXTENSIONVERSION) GetProcAddress(AppHnd, "GetExtensionVersion");
	if (GetExtensionVersion == NULL) 
	{
		warningsLogWrite("Failure to get pointer to GetExtensionVersion() in ISAPI application\r\n");
		return raiseHTTPError(td,connection,e_501);
	}
	if(!GetExtensionVersion(&Ver)) 
	{
		warningsLogWrite("ISAPI GetExtensionVersion() returned FALSE\r\n");
		return raiseHTTPError(td,connection,e_501);
	}
	if (Ver.dwExtensionVersion > MAKELONG(HSE_VERSION_MINOR, HSE_VERSION_MAJOR)) 
	{
		warningsLogWrite("ISAPI version not supported\r\n");
		return raiseHTTPError(td,connection,e_501);
	}
	/*
	*Store the environment string in the buffer2.
	*/
	connTable[connIndex].envString=td->buffer2;
	buildCGIEnvironmentString(td,connTable[connIndex].envString);

	memset(&ExtCtrlBlk, 0, sizeof(ExtCtrlBlk));
	ExtCtrlBlk.cbSize = sizeof(ExtCtrlBlk);
	ExtCtrlBlk.dwVersion = MAKELONG(HSE_VERSION_MINOR, HSE_VERSION_MAJOR);
	ExtCtrlBlk.GetServerVariable = GetServerVariableExport;
	ExtCtrlBlk.ReadClient  = ReadClientExport;
	ExtCtrlBlk.WriteClient = WriteClientExport;
	ExtCtrlBlk.ServerSupportFunction = ServerSupportFunctionExport;
	ExtCtrlBlk.ConnID = (HCONN) (connIndex + 1);
	ExtCtrlBlk.dwHttpStatusCode = 200;
	ExtCtrlBlk.lpszLogData[0] = '\0';
	ExtCtrlBlk.lpszMethod = td->request.CMD;
	ExtCtrlBlk.lpszQueryString = td->request.URIOPTS;
	ExtCtrlBlk.lpszPathInfo = td->pathInfo;
	ExtCtrlBlk.lpszPathTranslated = td->pathTranslated;
	ExtCtrlBlk.cbTotalBytes = atoi(td->request.CONTENTS_DIM);
	ExtCtrlBlk.cbAvailable = 0;
	ExtCtrlBlk.lpbData = 0;
	ExtCtrlBlk.lpszContentType = (LPSTR)(&(td->request.CONTENTS_TYPE[0]));

	HttpExtensionProc = (PFN_HTTPEXTENSIONPROC)GetProcAddress(AppHnd, "HttpExtensionProc");
	if (HttpExtensionProc == NULL) 
	{
		warningsLogWrite("Failure to get pointer to HttpExtensionProc() in ISAPI application module\r\n");
		return raiseHTTPError(td,connection,e_501);
	}
	Ret = HttpExtensionProc(&ExtCtrlBlk);
	if (Ret == HSE_STATUS_PENDING) 
	{
		WaitForSingleObject(connTable[connIndex].ISAPIDoneEvent, ISAPI_TIMEOUT);
	}
	int retvalue=0;
	switch(Ret) 
	{
		case HSE_STATUS_SUCCESS_AND_KEEP_CONN:
			retvalue=1;
			break;
		case HSE_STATUS_SUCCESS:
		case HSE_STATUS_ERROR:
			break;
			default:
			retvalue=0;
			break;
	}
	FreeLibrary(AppHnd);
	connTable[connIndex].Allocated = FALSE;
	return retvalue;
}


BOOL WINAPI ServerSupportFunctionExport(HCONN hConn, DWORD dwHSERRequest,LPVOID lpvBuffer, LPDWORD lpdwSize, LPDWORD lpdwDataType) 
{
	ConnTableRecord *ConnInfo;

	ConnInfo = HConnRecord(hConn);
	if (ConnInfo == NULL) 
	{
		warningsLogWrite("ServerSupportFunctionExport: invalid hConn\r\n");
		return FALSE;
	}

	switch (dwHSERRequest) 
	{
		case HSE_REQ_SEND_URL_REDIRECT_RESP:
			return ISAPIRedirect(ConnInfo->td,ConnInfo->connection,(char *)lpvBuffer);
			break;
		case HSE_REQ_SEND_URL:
			return ISAPISendURI(ConnInfo->td,ConnInfo->connection,(char *)lpvBuffer);
			break;
		case HSE_REQ_SEND_RESPONSE_HEADER:
			return ISAPISendHeader(ConnInfo->td,ConnInfo->connection,(char *)lpvBuffer);
			break;
		case HSE_REQ_DONE_WITH_SESSION:
			SetEvent(ConnInfo->ISAPIDoneEvent);
			break;
		default:
			return FALSE;
	}
	return TRUE;
}




/*
*Add a connection to the table.
*/
ConnTableRecord *HConnRecord(HCONN hConn) 
{
	int connIndex;

	connIndex = ((int) hConn) - 1;
	ConnTableRecord *ConnInfo;
	if ((connIndex < 0) || (connIndex >= max_Connections)) 
	{
		return NULL;
	}
	ConnInfo = &(connTable[connIndex]);
	if (ConnInfo->Allocated == FALSE) 
	{
		return NULL;
	}
	return ConnInfo;
}

/*
*Send an HTTP redirect.
*/
int ISAPIRedirect(httpThreadContext* td,LPCONNECTION a,char *URL) 
{
    return sendHTTPRedirect(td,a,URL);
}
/*
*Send an HTTP location.
*/
int ISAPISendURI(httpThreadContext* td,LPCONNECTION a,char *URL)
{
	return sendHTTPRESOURCE(td,a,URL,FALSE,FALSE);
}
/*
*Send the ISAPI header.
*/
int ISAPISendHeader(httpThreadContext* td,LPCONNECTION a,char *URL)
{
	return sendHTTPRESOURCE(td,a,URL,FALSE,TRUE);
}
/*
*Write directly to the output.
*/
BOOL WINAPI WriteClientExport(HCONN hConn, LPVOID Buffer, LPDWORD lpdwBytes, DWORD dwReserved)
{
	ConnTableRecord *ConnInfo;
	int Ret;

	if(*lpdwBytes==0)
		return TRUE;
	ConnInfo = HConnRecord(hConn);
	if (ConnInfo == NULL) 
	{
		warningsLogWrite("WriteClientExport: invalid hConn\r\n");
		return FALSE;
	}
	/*strncat(ConnInfo->td->buffer,(char*)Buffer,*lpdwBytes);*/
	ms_send(ConnInfo->connection->socket,(char*)Buffer,*lpdwBytes,0);
	if (Ret == -1) 
	{
		*lpdwBytes = 0;
		return FALSE;
	}
	else 
	{
		return TRUE;
	}
}
/*
*Read directly from the client.
*/
BOOL WINAPI ReadClientExport(HCONN hConn, LPVOID lpvBuffer, LPDWORD lpdwSize ) 
{
	ConnTableRecord *ConnInfo;
	u_long NumRead;

	ConnInfo = HConnRecord(hConn);
	if (ConnInfo == NULL) 
	{
		warningsLogWrite("ReadClientExport: invalid hConn\r\n");
		return FALSE;
	}
	ms_ReadFromFile(ConnInfo->td->inputData ,(char*)lpvBuffer,*lpdwSize,&NumRead);
	if (NumRead == -1) 
	{
		*lpdwSize = 0;
		return FALSE;
	}
	else 
	{
		*lpdwSize = (DWORD)NumRead;
		return TRUE;
	}
}


/*
*Get server environment variable.
*/
BOOL WINAPI GetServerVariableExport(HCONN hConn, LPSTR lpszVariableName, LPVOID lpvBuffer, LPDWORD lpdwSize) 
{
	ConnTableRecord *ConnInfo;

	ConnInfo = HConnRecord(hConn);
	if (ConnInfo == NULL) 
	{
		warningsLogWrite("GetServerVariableExport: invalid hConn\r\n");
		return FALSE;
	}
	/*
	*TODO
	*Find in ConnInfo->envString the value lpszVariableName and copy next string in lpvBuffer.
	*/
	*lpdwSize=lstrlen((char*)lpvBuffer);
	return TRUE;
}