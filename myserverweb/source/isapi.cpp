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
/*
*Initialize the ISAPI engine.
*/
void initISAPI()
{
	max_Connections=lserver->getNumThreads();
	connTable=new ConnTableRecord[max_Connections];
	ZeroMemory(connTable,sizeof(ConnTableRecord)*max_Connections);
}
void cleanupISAPI()
{
	if(connTable)
		delete []connTable;
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
	int ConnIndex;

	ConnIndex = ((int) hConn) - 1;
	ConnTableRecord *ConnInfo;
	if ((ConnIndex < 0) || (ConnIndex >= max_Connections)) 
	{
		return NULL;
	}
	ConnInfo = &(connTable[ConnIndex]);
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


	ConnInfo = HConnRecord(hConn);
	if (ConnInfo == NULL) 
	{
		warningsLogWrite("WriteClientExport: invalid hConn\r\n");
		return FALSE;
	}
	Ret=ms_send(ConnInfo->connection->socket,(char*)Buffer, *lpdwBytes,0);
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
	int NumRead;

	ConnInfo = HConnRecord(hConn);
	if (ConnInfo == NULL) 
	{
		warningsLogWrite("ReadClientExport: invalid hConn\r\n");
		return FALSE;
	}
	NumRead = ms_recv(ConnInfo->connection->socket,(char*)lpvBuffer,*lpdwSize,0);
	if (NumRead == -1) 
	{
		*lpdwSize = 0;
		return FALSE;
	}
	else 
	{
		*lpdwSize = NumRead;
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
	*TODO...
	*/

	return TRUE;
}