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

static  u_long max_Connections;
static ConnTableRecord *connTable;
static CRITICAL_SECTION GetTableEntryCritSec;
#define ISAPI_TIMEOUT (INFINITE)
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
	DWORD Ret;
	EXTENSION_CONTROL_BLOCK ExtCtrlBlk;
	HMODULE AppHnd;
	HSE_VERSION_INFO Ver;
	u_long connIndex;
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
	connTable[connIndex].Allocated = TRUE;
	connTable[connIndex].ISAPIDoneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	AppHnd = LoadLibrary(cgipath);
	if (AppHnd == NULL) 
	{
		warningsLogWrite("Failure to load ISAPI application module: ");
		warningsLogWrite(cgipath);
		warningsLogWrite("\r\n");
		FreeLibrary(AppHnd);
		return raiseHTTPError(td,connection,e_501);
	}

	GetExtensionVersion = (PFN_GETEXTENSIONVERSION) GetProcAddress(AppHnd, "GetExtensionVersion");
	if (GetExtensionVersion == NULL) 
	{
		warningsLogWrite("Failure to get pointer to GetExtensionVersion() in ISAPI application\r\n");
		FreeLibrary(AppHnd);
		return raiseHTTPError(td,connection,e_501);
	}
	if(!GetExtensionVersion(&Ver)) 
	{
		warningsLogWrite("ISAPI GetExtensionVersion() returned FALSE\r\n");
		FreeLibrary(AppHnd);
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
	/*
	*Build the environment string.
	*/
	lstrcpy(td->scriptPath,scriptpath);
	splitPath(scriptpath,td->scriptDir,td->scriptFile);
	splitPath(cgipath,td->cgiRoot,td->cgiFile);
	buildCGIEnvironmentString(td,connTable[connIndex].envString);

	memset(&ExtCtrlBlk, 0, sizeof(ExtCtrlBlk));
	ExtCtrlBlk.cbSize = sizeof(ExtCtrlBlk);
	ExtCtrlBlk.dwVersion = MAKELONG(HSE_VERSION_MINOR, HSE_VERSION_MAJOR);
	ExtCtrlBlk.GetServerVariable = GetServerVariableExport;
	ExtCtrlBlk.ReadClient  = ReadClientExport;
	ExtCtrlBlk.WriteClient = WriteClientExport;
	ExtCtrlBlk.ServerSupportFunction = ServerSupportFunctionExport;
	ExtCtrlBlk.ConnID = (HCONN) (connIndex + 1);
	ExtCtrlBlk.dwHttpStatusCode = 0;
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
		FreeLibrary(AppHnd);
		return raiseHTTPError(td,connection,e_501);
	}
	Ret = HttpExtensionProc(&ExtCtrlBlk);
	if (Ret == HSE_STATUS_PENDING) 
	{
		WaitForSingleObject(connTable[connIndex].ISAPIDoneEvent, ISAPI_TIMEOUT);
	}

	/*
	*Flush the output to the client.
	*/

	u_long headerSize=0;
	u_long len=lstrlen(connTable[connIndex].td->buffer);
	for(u_long i=0;i<len;i++)
	{
		if(connTable[connIndex].td->buffer[i]=='\n')
			if(connTable[connIndex].td->buffer[i+1]=='\n')
			{
				/*
				*The HTTP header ends with a \n\n sequence so 
				*determinate where it ends and set the header size
				*to i + 2.
				*/
				headerSize=i+2;
				break;
			}
	}
	sprintf(connTable[connIndex].td->response.CONTENTS_DIM,"%u",len-headerSize);
	buildHTTPResponseHeader(connTable[connIndex].td->buffer2,&(connTable[connIndex].td->response));
	if(headerSize)
		ms_send(connTable[connIndex].connection->socket,connTable[connIndex].td->buffer2,lstrlen(connTable[connIndex].td->buffer2)-2, 0);
	else
		ms_send(connTable[connIndex].connection->socket,connTable[connIndex].td->buffer2,lstrlen(connTable[connIndex].td->buffer2), 0);

	char lbuffer[100];
	ms_send(connTable[connIndex].connection->socket,connTable[connIndex].td->buffer,headerSize-2, 0);
	sprintf(lbuffer,"\r\nContent-Length:%i\r\n\r\n",len-headerSize);
	ms_send(connTable[connIndex].connection->socket,lbuffer,lstrlen(lbuffer), 0);
	ms_send(connTable[connIndex].connection->socket,(char*)(connTable[connIndex].td->buffer+headerSize),len-headerSize, 0);

	int retvalue=0;

	switch(Ret) 
	{
		case HSE_STATUS_SUCCESS_AND_KEEP_CONN:
			retvalue=1;
			break;
		case HSE_STATUS_SUCCESS:
		case HSE_STATUS_ERROR:
		default:
			retvalue=0;
			break;
	}
	if(!FreeLibrary(AppHnd))
	{
		warningsLogWrite("Failure to FreeLibrary in ISAPI module");
		warningsLogWrite(cgipath);
		warningsLogWrite("\r\n");
	}
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
		case HSE_REQ_MAP_URL_TO_PATH_EX:
		case HSE_REQ_MAP_URL_TO_PATH:
			char URL[MAX_PATH];
			HSE_URL_MAPEX_INFO  *mapInfo;
			mapInfo=(HSE_URL_MAPEX_INFO*)lpdwDataType;
			lstrcpy(URL,(char*)lpvBuffer+1);
			getPath((char*)lpvBuffer,URL,FALSE);
			*lpdwSize=lstrlen((char*)lpvBuffer);
			lstrcpy(mapInfo->lpszPath,(char*)lpvBuffer);
			break;
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
	u_long connIndex;

	connIndex =((int) hConn) - 1;
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
BOOL WINAPI WriteClientExport(HCONN hConn, LPVOID Buffer, LPDWORD lpdwBytes, DWORD /*dwReserved*/)
{
	ConnTableRecord *ConnInfo;

	if(*lpdwBytes==0)
		return TRUE;
	ConnInfo = HConnRecord(hConn);
	if (ConnInfo == NULL) 
	{
		warningsLogWrite("WriteClientExport: invalid hConn\r\n");
		return FALSE;
	}
	strncat(ConnInfo->td->buffer,(char*)Buffer,*lpdwBytes);
	if (*lpdwBytes == -1) 
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
	*Find in ConnInfo->envString the value lpszVariableName and copy next string in lpvBuffer.
	*/
	((char*)lpvBuffer)[0]='\0';
	char *localEnv=ConnInfo->envString;
	int variableNameLen=(int)strlen(lpszVariableName);
	for(u_long i=0;;i+=(u_long)strlen(&localEnv[i])+1)
	{
		if(((localEnv[i+variableNameLen])=='=')&&(!strncmp(&localEnv[i],lpszVariableName,variableNameLen)))
		{
			strncpy((char*)lpvBuffer,&localEnv[i+variableNameLen+1],*lpdwSize);
			break;
		}
		else if((localEnv[i]=='\0') && (localEnv[i+1]=='\0'))
		{
			break;
		}
	}
	*lpdwSize=lstrlen((char*)lpvBuffer);
	return TRUE;
}