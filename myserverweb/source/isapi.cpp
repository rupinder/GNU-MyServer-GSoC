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

 
#include "../include/isapi.h"
#include "../include/http.h"
#include "../include/cserver.h"
#include "../include/HTTPmsg.h"
#include "../include/cgi.h"

#ifdef WIN32

u_long isapi::max_Connections=0;
static CRITICAL_SECTION GetTableEntryCritSec;
int isapi::initialized=0;
#define ISAPI_TIMEOUT (10000)
ConnTableRecord *isapi::connTable=0;

BOOL WINAPI ISAPI_ServerSupportFunctionExport(HCONN hConn, DWORD dwHSERRequest,LPVOID lpvBuffer, LPDWORD lpdwSize, LPDWORD lpdwDataType) 
{
	ConnTableRecord *ConnInfo;

	ConnInfo = isapi::HConnRecord(hConn);
	if (ConnInfo == NULL) 
	{
		preparePrintError();
		printf("isapi::ServerSupportFunctionExport: invalid hConn\r\n");
		endPrintError();
		return FALSE;
	}
	switch (dwHSERRequest) 
	{
		case HSE_REQ_MAP_URL_TO_PATH_EX:
			HSE_URL_MAPEX_INFO  *mapInfo;
			mapInfo=(HSE_URL_MAPEX_INFO*)lpdwDataType;
			((http*)ConnInfo->td->lhttp)->getPath(ConnInfo->td,ConnInfo->connection,mapInfo->lpszPath,(char*)lpvBuffer,FALSE);
			mapInfo->cchMatchingURL=(DWORD)strlen((char*)lpvBuffer);
			mapInfo->cchMatchingPath=(DWORD)strlen(mapInfo->lpszPath);
			mapInfo->dwFlags = HSE_URL_FLAGS_WRITE|HSE_URL_FLAGS_SCRIPT|HSE_URL_FLAGS_EXECUTE;
			break;
		case HSE_REQ_MAP_URL_TO_PATH:
			char URI[MAX_PATH];
			if(((char*)lpvBuffer)[0])
				strcpy(URI,(char*)lpvBuffer);
			else
				lstrcpyn(URI,ConnInfo->td->request.URI,(int)(strlen(ConnInfo->td->request.URI)-strlen(ConnInfo->td->pathInfo)+1));
			((http*)ConnInfo->td->lhttp)->getPath(ConnInfo->td,ConnInfo->connection,(char*)lpvBuffer,URI,FALSE);
			MYSERVER_FILE::completePath((char*)lpvBuffer);
			*lpdwSize=(DWORD)strlen((char*)lpvBuffer);
			break;
		case HSE_REQ_SEND_URL_REDIRECT_RESP:
			return ((isapi*)ConnInfo->lisapi)->Redirect(ConnInfo->td,ConnInfo->connection,(char *)lpvBuffer);
			break;
		case HSE_REQ_SEND_URL:
			return ((isapi*)ConnInfo->lisapi)->SendURI(ConnInfo->td,ConnInfo->connection,(char *)lpvBuffer);
			break;
		case HSE_REQ_SEND_RESPONSE_HEADER:
			return ((isapi*)ConnInfo->lisapi)->SendHeader(ConnInfo->td,ConnInfo->connection,(char *)lpvBuffer);
			break;
		case HSE_REQ_DONE_WITH_SESSION:
			ConnInfo->td->response.httpStatus=*(DWORD*)lpvBuffer;
			SetEvent(ConnInfo->ISAPIDoneEvent);
			break;
		case HSE_REQ_IS_KEEP_CONN:
			if(!lstrcmpi(ConnInfo->td->request.CONNECTION,"Keep-Alive"))
				*((BOOL*)lpvBuffer)=TRUE;
			else
				*((BOOL*)lpvBuffer)=FALSE;
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

/*!
*Add a connection to the table.
*/
ConnTableRecord *isapi::HConnRecord(HCONN hConn) 
{
	u_long connIndex;

	connIndex =((u_long) hConn) - 1;
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

/*!
*Send an HTTP redirect.
*/
int isapi::Redirect(httpThreadContext* td,LPCONNECTION a,char *URL) 
{
	return ((http*)td->lhttp)->sendHTTPRedirect(td,a,URL);
}

/*!
*Send an HTTP URI.
*/
int isapi::SendURI(httpThreadContext* td,LPCONNECTION a,char *URL)
{
	return ((http*)td->lhttp)->sendHTTPRESOURCE(td,a,URL,FALSE,FALSE);
}

/*!
*Send the ISAPI header.
*/
int isapi::SendHeader(httpThreadContext* td,LPCONNECTION a,char *URL)
{
	return ((http*)td->lhttp)->sendHTTPRESOURCE(td,a,URL,FALSE,TRUE);
}

/*!
*Write directly to the output.
*/
BOOL WINAPI ISAPI_WriteClientExport(HCONN hConn, LPVOID Buffer, LPDWORD lpdwBytes, DWORD /*!dwReserved*/)
{
	ConnTableRecord *ConnInfo;
	if(*lpdwBytes==0)
		return TRUE;
	ConnInfo = isapi::HConnRecord(hConn);
	if (ConnInfo == NULL) 
	{
		((vhost*)(ConnInfo->td->connection->host))->warningslogRequestAccess(ConnInfo->td->id);
		((vhost*)(ConnInfo->td->connection->host))->warningsLogWrite("isapi::WriteClientExport: invalid hConn\r\n");
		((vhost*)(ConnInfo->td->connection->host))->warningslogTerminateAccess(ConnInfo->td->id);
		return FALSE;
	}
	int keepalive = (lstrcmpi(ConnInfo->td->request.CONNECTION,"Keep-Alive")==0) ? 1 : 0 ;

	char chunk_size[15];
	u_long nbw=0;
	if(!ConnInfo->headerSent)/*If the HTTP header was sent do not send it again*/
	{
		strncat(ConnInfo->td->buffer,((char*)Buffer),*lpdwBytes);
		ConnInfo->headerSize+=*lpdwBytes;
		int headerSize=0;
		for(u_long i=0;i<(u_long)(strlen(ConnInfo->td->buffer));i++)
		{
			if((ConnInfo->td->buffer[i]=='\r'))
				if(ConnInfo->td->buffer[i+1]=='\n')
					if(ConnInfo->td->buffer[i+2]=='\r')
						if(ConnInfo->td->buffer[i+3]=='\n')
						{
							headerSize=i+4;
							ConnInfo->td->buffer[i+2]='\0';
							break;
						}
			if((ConnInfo->td->buffer[i]=='\n'))
			{
				if(ConnInfo->td->buffer[i+1]=='\n')
				{
					headerSize=i+2;
					ConnInfo->td->buffer[i+1]='\0';
					break;
				}
			}
		}
		if(headerSize && (!ConnInfo->td->appendOutputs))
		{
			int len=ConnInfo->headerSize-headerSize;
			if(!ConnInfo->td->appendOutputs)
			{
				if(keepalive)
					strcpy(ConnInfo->td->response.TRANSFER_ENCODING,"chunked");
				http_headers::buildHTTPResponseHeaderStruct(&ConnInfo->td->response,ConnInfo->td,ConnInfo->td->buffer);
				if(keepalive)
					strcpy(ConnInfo->td->response.CONNECTION,"Keep-Alive");
				else
					strcpy(ConnInfo->td->response.CONNECTION,"Close");
				http_headers::buildHTTPResponseHeader(ConnInfo->td->buffer2,&(ConnInfo->td->response));
	
				if(ConnInfo->connection->socket.send(ConnInfo->td->buffer2,(int)strlen(ConnInfo->td->buffer2), 0)==-1)
					return 0;
			}
			ConnInfo->headerSent=1;

			/*!Send the first chunk*/
			if(len)
			{
				if(keepalive && (!ConnInfo->td->appendOutputs))
				{
					sprintf(chunk_size,"%x\r\n",len);
					if(ConnInfo->connection->socket.send(chunk_size,(int)strlen(chunk_size), 0)==-1)
						return 0;
				}
				if(ConnInfo->td->appendOutputs)
					ConnInfo->td->outputData.writeToFile((char*)(ConnInfo->td->buffer+headerSize),len,&nbw);
				else
					nbw=ConnInfo->connection->socket.send((char*)(ConnInfo->td->buffer+headerSize),len, 0);
				if(nbw==-1)
					return 0;
				if(keepalive && (!ConnInfo->td->appendOutputs))
				{
					if(ConnInfo->connection->socket.send("\r\n",2, 0)==-1)
						return 0;
				}
			}


		}
		else
			nbw=*lpdwBytes;
	}
	else/*!Continue to send data chunks*/
	{
		if(keepalive)
		{
			sprintf(chunk_size,"%x\r\n",*lpdwBytes);
			if(ConnInfo->connection->socket.send(chunk_size,(int)strlen(chunk_size), 0)==-1)
				return 0;
		}
		nbw=ConnInfo->connection->socket.send((char*)Buffer,*lpdwBytes, 0);
		if(keepalive)
		{
			if(ConnInfo->connection->socket.send("\r\n",2, 0)==-1)
				return 0;
		}
	}

	*lpdwBytes = nbw;

	if (nbw!=-1) 
	{
		return TRUE;
	}
	else 
	{
		return FALSE;
	}
}
/*!
*Read directly from the client.
*/
BOOL WINAPI ISAPI_ReadClientExport(HCONN hConn, LPVOID lpvBuffer, LPDWORD lpdwSize ) 
{
	ConnTableRecord *ConnInfo;
	u_long NumRead;

	ConnInfo = isapi::HConnRecord(hConn);
	if (ConnInfo == NULL) 
	{
		((vhost*)(ConnInfo->td->connection->host))->warningslogRequestAccess(ConnInfo->td->id);
		((vhost*)(ConnInfo->td->connection->host))->warningsLogWrite("isapi::ReadClientExport: invalid hConn\r\n");
		((vhost*)(ConnInfo->td->connection->host))->warningslogTerminateAccess(ConnInfo->td->id);
		return FALSE;
	}
	ConnInfo->td->inputData.readFromFile((char*)lpvBuffer,*lpdwSize,&NumRead);
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


/*!
*Get server environment variable.
*/
BOOL WINAPI ISAPI_GetServerVariableExport(HCONN hConn, LPSTR lpszVariableName, LPVOID lpv