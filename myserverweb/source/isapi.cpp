/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
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
#include "../include/filemanager.h"
#include "../include/HTTPmsg.h"
#include "../include/cgi.h"

#ifdef WIN32

u_long isapi::max_Connections=0;
static CRITICAL_SECTION GetTableEntryCritSec;
int isapi::initialized=0;
myserver_mutex *isapi::isapi_mutex=0;
#define ISAPI_TIMEOUT (10000)
ConnTableRecord *isapi::connTable=0;

BOOL WINAPI ISAPI_ServerSupportFunctionExport(HCONN hConn, DWORD dwHSERRequest,
                                              LPVOID lpvBuffer, LPDWORD lpdwSize, 
                                              LPDWORD lpdwDataType) 
{
	ConnTableRecord *ConnInfo;
	
	isapi::isapi_mutex->myserver_mutex_lock();
	ConnInfo = isapi::HConnRecord(hConn);
	isapi::isapi_mutex->myserver_mutex_unlock();
	if (ConnInfo == NULL) 
	{
		preparePrintError();
		printf("isapi::ServerSupportFunctionExport: invalid hConn\r\n");
		endPrintError();
		return 0;
	}
	char *buffer=0;	
	char URI[MAX_PATH];/*! Under windows use MAX_PATH. */
	switch (dwHSERRequest) 
	{
		case HSE_REQ_MAP_URL_TO_PATH_EX:
			HSE_URL_MAPEX_INFO  *mapInfo;
			mapInfo=(HSE_URL_MAPEX_INFO*)lpdwDataType;
      mapInfo->lpszPath = 0;
			((http*)ConnInfo->td->lhttp)->getPath(ConnInfo->td,ConnInfo->connection,
                                           &mapInfo->lpszPath,(char*)lpvBuffer,0);
			mapInfo->cchMatchingURL=(DWORD)strlen((char*)lpvBuffer);
			mapInfo->cchMatchingPath=(DWORD)strlen(mapInfo->lpszPath);
      delete [] mapInfo->lpszPath;
			mapInfo->dwFlags = HSE_URL_FLAGS_WRITE|HSE_URL_FLAGS_SCRIPT 
                                            | HSE_URL_FLAGS_EXECUTE;
			break;
		case HSE_REQ_MAP_URL_TO_PATH:
			if(((char*)lpvBuffer)[0])
				strcpy(URI,(char*)lpvBuffer);
			else
				lstrcpyn(URI,ConnInfo->td->request.URI,
                 (int)(strlen(ConnInfo->td->request.URI)-
				 (ConnInfo->td->pathInfo?strlen(ConnInfo->td->pathInfo):0) +1));

			((http*)ConnInfo->td->lhttp)->getPath(ConnInfo->td,ConnInfo->connection,
                                            (char**)&buffer,URI,0);
      if(buffer==0)
      {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;            
      }
      if(strlen(buffer) < *lpdwSize)
      {
        strcpy((char*)lpvBuffer, buffer);
        delete [] buffer;
      }
      else
      {
        delete [] buffer;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
      }
      if(MYSERVER_FILE::completePath((char**)&lpvBuffer,(int*)lpdwSize,  1))
      {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
      }
      *lpdwSize=(DWORD)strlen((char*)lpvBuffer);
      break;
  case HSE_REQ_SEND_URL_REDIRECT_RESP:
    return ((isapi*)ConnInfo->lisapi)->Redirect(ConnInfo->td,
                              ConnInfo->connection,(char *)lpvBuffer);
    break;
  case HSE_REQ_SEND_URL:
    return ((isapi*)ConnInfo->lisapi)->SendURI(ConnInfo->td,
                                           ConnInfo->connection,(char *)lpvBuffer);
    break;
  case HSE_REQ_SEND_RESPONSE_HEADER:
    return ((isapi*)ConnInfo->lisapi)->SendHeader(ConnInfo->td,
                                           ConnInfo->connection,(char *)lpvBuffer);
    break;
  case HSE_REQ_DONE_WITH_SESSION:
    ConnInfo->td->response.httpStatus=*(DWORD*)lpvBuffer;
    SetEvent(ConnInfo->ISAPIDoneEvent);
    break;
  case HSE_REQ_IS_KEEP_CONN:
    if(!lstrcmpi(ConnInfo->td->request.CONNECTION,"Keep-Alive"))
      *((BOOL*)lpvBuffer)=1;
    else
      *((BOOL*)lpvBuffer)=0;
    break;
  default:
    return 0;
	}
	return 1;
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
	if (ConnInfo->Allocated == 0) 
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
	return ((http*)td->lhttp)->sendHTTPRESOURCE(td,a,URL,0,0);
}

/*!
 *Send the ISAPI header.
 */
int isapi::SendHeader(httpThreadContext* td,LPCONNECTION a,char *URL)
{
	return ((http*)td->lhttp)->sendHTTPRESOURCE(td,a,URL,0,1);
}

/*!
 *Write directly to the output.
 */
BOOL WINAPI ISAPI_WriteClientExport(HCONN hConn, LPVOID Buffer, LPDWORD lpdwBytes, 
                                    DWORD /*!dwReserved*/)
{
	ConnTableRecord *ConnInfo;
	if(*lpdwBytes==0)
		return 1;
	isapi::isapi_mutex->myserver_mutex_lock();
	ConnInfo = isapi::HConnRecord(hConn);
	char* buffer=(char*)ConnInfo->td->buffer->GetBuffer();
	isapi::isapi_mutex->myserver_mutex_unlock();
	if (ConnInfo == NULL) 
	{
		((vhost*)(ConnInfo->td->connection->host))->warningslogRequestAccess(
                                                                ConnInfo->td->id);
		((vhost*)(ConnInfo->td->connection->host))->warningsLogWrite(
                                        "isapi::WriteClientExport: invalid hConn\r\n");
		((vhost*)(ConnInfo->td->connection->host))->warningslogTerminateAccess(
                                                                ConnInfo->td->id);
		return 0;
	}
	int keepalive =(lstrcmpi(ConnInfo->td->request.CONNECTION, "Keep-Alive")==0)?1:0 ;

	char chunk_size[15];
	u_long nbw=0;

  /*If the HTTP header was sent do not send it again. */
	if(!ConnInfo->headerSent)
	{
		strncat(buffer,(char*)Buffer,*lpdwBytes);
		ConnInfo->headerSize+=*lpdwBytes;
		int headerSize=0;
		for(u_long i=0;i<(u_long)strlen(buffer);i++)
		{
			if(buffer[i]=='\r')
				if(buffer[i+1]=='\n')
					if(buffer[i+2]=='\r')
						if(buffer[i+3]=='\n')
						{
							headerSize=i+4;
							buffer[i+2]='\0';
							break;
						}
			if(buffer[i]=='\n')
			{
				if(buffer[i+1]=='\n')
				{
					headerSize=i+2;
					buffer[i+1]='\0';
					break;
				}
			}
		}
		if(headerSize && (!ConnInfo->td->appendOutputs))
		{
			int len = ConnInfo->headerSize-headerSize;
			if(!ConnInfo->td->appendOutputs)
			{
				if(keepalive)
					strcpy(ConnInfo->td->response.TRANSFER_ENCODING,"chunked");
				http_headers::buildHTTPResponseHeaderStruct(&ConnInfo->td->response,
                              ConnInfo->td,(char*)ConnInfo->td->buffer->GetBuffer());
				if(keepalive)
					strcpy(ConnInfo->td->response.CONNECTION,"Keep-Alive");
				else
					strcpy(ConnInfo->td->response.CONNECTION,"Close");
				http_headers::buildHTTPResponseHeader(
                 (char*)ConnInfo->td->buffer2->GetBuffer(),&(ConnInfo->td->response));
	
				if(ConnInfo->connection->socket.send(
                     (char*)ConnInfo->td->buffer2->GetBuffer(),
                     (int)strlen((char*)ConnInfo->td->buffer2->GetBuffer()), 0)==-1)
					return 0;
			}
			ConnInfo->headerSent=1;
      
      if(ConnInfo->headerSent && ConnInfo->only_header)
        return 0;

			/*!Send the first chunk. */
			if(len)
			{
				if(keepalive && (!ConnInfo->td->appendOutputs))
				{
					sprintf(chunk_size,"%x\r\n",len);
					if(ConnInfo->connection->socket.send(chunk_size,(int)strlen(chunk_size), 
                                               0)==-1)
						return 0;
				}
				if(ConnInfo->td->appendOutputs)
				{
					if(ConnInfo->td->outputData.writeToFile((char*)(buffer+headerSize),len,
                                                   &nbw))
						return 0;
				}
				else
				{
					nbw=(u_long)ConnInfo->connection->socket.send((char*)(buffer+headerSize), 
                                                        len, 0);
					if((nbw == (u_long)-1) || (!nbw))
						return 0;
				}
				if(keepalive && (!ConnInfo->td->appendOutputs))
				{
					nbw = ConnInfo->connection->socket.send("\r\n",2, 0);
					if((nbw == (u_long)-1) || (!nbw))
						return 0;
				}
			}
		}
		else
    {
			nbw=*lpdwBytes;
    }
	}
	else/*!Continue to send data chunks*/
	{
		if(keepalive)
		{
			sprintf(chunk_size,"%x\r\n",*lpdwBytes);
			nbw = ConnInfo->connection->socket.send(chunk_size,(int)strlen(chunk_size), 0);
			if((nbw == (u_long)-1) || (!nbw))
				return 0;
		}
		nbw = ConnInfo->connection->socket.send((char*)Buffer,*lpdwBytes, 0);
		if(keepalive)
		{
			nbw = ConnInfo->connection->socket.send("\r\n",2, 0);
			if((nbw == (u_long)-1) || (!nbw))
				return 0;
		}
	}

	*lpdwBytes = nbw;

	if (nbw!=-1) 
	{
		return 1;
	}
	else 
	{
		return 0;
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
		return 0;
	}
	ConnInfo->td->inputData.readFromFile((char*)lpvBuffer,*lpdwSize,&NumRead);
	if (NumRead == -1) 
	{
		*lpdwSize = 0;
		return 0;
	}
	else 
	{
		*lpdwSize = (DWORD)NumRead;
		return 1;
	}
}

/*!
*Get server environment variable.
*/
BOOL WINAPI ISAPI_GetServerVariableExport(HCONN hConn, LPSTR lpszVariableName, LPVOID lpvBuffer, LPDWORD lpdwSize) 
{
	ConnTableRecord *ConnInfo;
	BOOL ret =1;
	isapi::isapi_mutex->myserver_mutex_lock();
	ConnInfo = isapi::HConnRecord(hConn);
	isapi::isapi_mutex->myserver_mutex_unlock();
	if (ConnInfo == NULL) 
	{
		preparePrintError();
		printf("isapi::GetServerVariableExport: invalid hConn\r\n");
		endPrintError();
		return 0;
	}

	if (!strcmp(lpszVariableName, "ALL_HTTP")) 
	{
		if(isapi::buildAllHttpHeaders(ConnInfo->td,ConnInfo->connection, 
                                  lpvBuffer, lpdwSize))
			ret=1;
		else
		{
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
			ret=0;
		}
			
	}else if(!strcmp(lpszVariableName, "ALL_RAW")) 
	{
		if(isapi::buildAllRawHeaders(ConnInfo->td,ConnInfo->connection,lpvBuffer,lpdwSize))
			ret=1;
		else
		{
            		SetLastError(ERROR_INSUFFICIENT_BUFFER);
			ret=0;
		}
			
	}
	else
	{
		/*!
     *Find in ConnInfo->envString the value lpszVariableName 
     *and copy next string in lpvBuffer.
     */
		((char*)lpvBuffer)[0]='\0';
		char *localEnv=ConnInfo->envString;
		int variableNameLen=(int)strlen(lpszVariableName);
		for(u_long i=0;;i+=(u_long)strlen(&localEnv[i])+1)
		{
			if(((localEnv[i+variableNameLen])=='=')&&
         (!strncmp(&localEnv[i],lpszVariableName,variableNameLen)))
			{
				strncpy((char*)lpvBuffer,&localEnv[i+variableNameLen+1],*lpdwSize);
				break;
			}
			else if((localEnv[i]=='\0') && (localEnv[i+1]=='\0'))
			{
				break;
			}
		}
	}
	*lpdwSize=(DWORD)strlen((char*)lpvBuffer);
	return ret;
}
/*!
 *Build the string that contains all the HTTP headers.
 */
BOOL isapi::buildAllHttpHeaders(httpThreadContext* td,LPCONNECTION /*!a*/,LPVOID output,LPDWORD dwMaxLen)
{
	DWORD valLen=0;
	DWORD maxLen=*dwMaxLen;
	char *ValStr=(char*)output;

	if(td->request.ACCEPT[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_ACCEPT:%s\n",td->request.ACCEPT);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.ACCEPTLAN[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_ACCEPT_LANGUAGE:%s\n",
                    td->request.ACCEPTLAN);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.ACCEPTENC[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_ACCEPT_ENCODING:%s\n",
                    td->request.ACCEPTENC);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.CONNECTION[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_CONNECTION:%s\n",td->request.CONNECTION);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.COOKIE[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_COOKIE:%s\n",td->request.COOKIE);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.HOST[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_HOST:%s\n",td->request.HOST);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.DATE[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_DATE:%s\n",td->request.DATE);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.MODIFIED_SINCE[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_IF_MODIFIED_SINCE:%s\n",
                    td->request.MODIFIED_SINCE);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.REFERER[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_REFERER:%s\n",td->request.REFERER);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.PRAGMA[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_PRAGMA:%s\n",td->request.PRAGMA);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.USER_AGENT[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_USER_AGENT:%s\n",td->request.USER_AGENT);
	else if(valLen+30<maxLen)
		return 0;

	if(td->request.VER[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_MIME_VERSION:%s\n",td->request.VER);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.FROM[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_FROM:%s\n",td->request.FROM);
	else if(valLen+30<maxLen) 
		return 0;
	return 1;
}

/*!
 *Build the string that contains all the headers.
 */
BOOL isapi::buildAllRawHeaders(httpThreadContext* td,LPCONNECTION a,
                               LPVOID output,LPDWORD dwMaxLen)
{
	DWORD valLen=0;
	DWORD maxLen=*dwMaxLen;
	char *ValStr=(char*)output;
	if(buildAllHttpHeaders(td,a,output,dwMaxLen)==0)
		return 0;
	valLen=(DWORD)strlen(ValStr);

	if(td->pathInfo && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"PATH_INFO:%s\n",td->pathInfo);
	else if(valLen+30<maxLen) 
		return 0;
	
	if(td->pathTranslated && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"PATH_INFO:%s\n",td->pathTranslated);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.URIOPTS[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"QUERY_STRING:%s\n",td->request.URIOPTS[0]);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.CMD[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"REQUEST_METHOD:%s\n",td->request.CMD[0]);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->filenamePath && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"SCRIPT_FILENAME:%s\n",td->filenamePath[0]);
	else if(valLen+30<maxLen) 
		return 0;

	if(valLen+30<maxLen)
		valLen+=sprintf(&ValStr[valLen],"SERVER_PORT:%u\n",td->connection->localPort);
	else if(valLen+30<maxLen) 
		return 0;

	if(valLen+30<maxLen)
		valLen+=sprintf(&ValStr[valLen],"SERVER_SIGNATURE:<address>%s</address>\n",
                    versionOfSoftware);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->connection->ipAddr[0] && valLen+30<maxLen)
		valLen+=sprintf(&ValStr[valLen],"REMOTE_ADDR:\n",td->connection->ipAddr);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->connection->port && valLen+30<maxLen)
		valLen+=sprintf(&ValStr[valLen],"REMOTE_PORT:%u\n",td->connection->port);
	else if(valLen+30<maxLen) 
		return 0;

	if(valLen+30<maxLen)
		valLen+=sprintf(&ValStr[valLen],"SERVER_ADMIN:%s\n",lserver->getServerAdmin());
	else if(valLen+30<maxLen) 
		return 0;

	if(valLen+MAX_PATH<maxLen)
	{
		valLen+=sprintf(&ValStr[valLen],"SCRIPT_NAME:");
		lstrcpyn(&ValStr[valLen],td->request.URI,(int)(strlen(td->request.URI)-
                                                   strlen(td->pathInfo)+1));
		valLen+=(DWORD)strlen(td->request.URI)-strlen(td->pathInfo)+1;
		valLen+=(DWORD)sprintf(&ValStr[valLen],"\n");
	}
	else if(valLen+30<maxLen) 
		return 0;
	return 1;
}

#endif

/*!
 *Main procedure to call an ISAPI module.
 */
int isapi::sendISAPI(httpThreadContext* td,LPCONNECTION connection, 
                     char* scriptpath,char* /*!ext*/,char *cgipath,int execute,
                     int only_header)
{
  /*!
   *ISAPI works only on the windows architecture.
   */
#ifdef NOT_WIN
		return ((http*)td->lhttp)->raiseHTTPError(td, connection, e_501);
#endif

#ifdef WIN32
	DWORD Ret;
	EXTENSION_CONTROL_BLOCK ExtCtrlBlk;
	HMODULE AppHnd;
	HSE_VERSION_INFO Ver;
	u_long connIndex;
	PFN_GETEXTENSIONVERSION GetExtensionVersion;
	PFN_HTTPEXTENSIONPROC HttpExtensionProc;

	char fullpath[MAX_PATH*2];/*! Under windows there is MAX_PATH so use it. */
	if(execute)
	{
		if(cgipath[0])
			sprintf(fullpath,"%s \"%s\"",cgipath,td->filenamePath);
		else
			sprintf(fullpath,"%s",td->filenamePath);
	}
	else
	{
		sprintf(fullpath,"%s",cgipath);
	}
	EnterCriticalSection(&GetTableEntryCritSec);
	connIndex = 0;
	isapi::isapi_mutex->myserver_mutex_lock();
	while ((connTable[connIndex].Allocated != 0) && (connIndex < max_Connections)) 
	{
		connIndex++;
	}
	isapi::isapi_mutex->myserver_mutex_unlock();
	LeaveCriticalSection(&GetTableEntryCritSec);

	if (connIndex == max_Connections) 
	{
		((vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((vhost*)td->connection->host)->warningsLogWrite(
                                                    "Error ISAPI max connections\r\n");
		((vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		return ((http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
	}
	AppHnd = LoadLibrary(fullpath);

	connTable[connIndex].connection = connection;
	connTable[connIndex].td = td;
  connTable[connIndex].only_header = only_header;
	connTable[connIndex].headerSent=0;
	connTable[connIndex].headerSize=0;
	connTable[connIndex].Allocated = 1;
	connTable[connIndex].lisapi = this;
	connTable[connIndex].ISAPIDoneEvent = CreateEvent(NULL, 1, 0, NULL);
	if (AppHnd == NULL) 
	{
		((vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((vhost*)(td->connection->host))->warningsLogWrite(
                                         "Failure to load ISAPI application module: ");
		((vhost*)(td->connection->host))->warningsLogWrite(cgipath);
		((vhost*)(td->connection->host))->warningsLogWrite("\r\n");
		((vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		if(!FreeLibrary((HMODULE)AppHnd))
		{
			((vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
			((vhost*)(td->connection->host))->warningsLogWrite(
                                          "Failure to FreeLibrary in ISAPI module");
			((vhost*)(td->connection->host))->warningsLogWrite(cgipath);
			((vhost*)(td->connection->host))->warningsLogWrite("\r\n");
			((vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		}
    return	((http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
	}

	GetExtensionVersion = (PFN_GETEXTENSIONVERSION) 
    GetProcAddress(AppHnd, "GetExtensionVersion");
	if (GetExtensionVersion == NULL) 
	{
		((vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((vhost*)(td->connection->host))->warningsLogWrite(
           "Failure to get pointer to GetExtensionVersion() in ISAPI application\r\n");
		((vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		if(!FreeLibrary(AppHnd))
		{
			((vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
			((vhost*)(td->connection->host))->warningsLogWrite(
                                            "Failure to FreeLibrary in ISAPI module");
			((vhost*)(td->connection->host))->warningsLogWrite(cgipath);
			((vhost*)(td->connection->host))->warningsLogWrite("\r\n");
			((vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		}
		return ((http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
	}
	if(!GetExtensionVersion(&Ver)) 
	{
		((vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((vhost*)(td->connection->host))->warningsLogWrite(
                                     "ISAPI GetExtensionVersion() returned FALSE\r\n");
		((vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		if(!FreeLibrary(AppHnd))
		{
			((vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
			((vhost*)(td->connection->host))->warningsLogWrite(
                                            "Failure to FreeLibrary in ISAPI module");
			((vhost*)(td->connection->host))->warningsLogWrite(cgipath);
			((vhost*)(td->connection->host))->warningsLogWrite("\r\n");
			((vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		}
		return ((http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
	}
	if (Ver.dwExtensionVersion > MAKELONG(HSE_VERSION_MINOR, HSE_VERSION_MAJOR)) 
	{
		((vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((vhost*)(td->connection->host))->warningsLogWrite(
                                       "ISAPI version not supported\r\n");
		((vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		if(!FreeLibrary((HMODULE)AppHnd))
		{
			((vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
			((vhost*)(td->connection->host))->warningsLogWrite(
                                    "Failure to FreeLibrary in ISAPI module");
			((vhost*)(td->connection->host))->warningsLogWrite(cgipath);
			((vhost*)(td->connection->host))->warningsLogWrite("\r\n");
			((vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		}
		return ((http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
	}
	/*!
   *Store the environment string in the buffer2.
	*/
	connTable[connIndex].envString=(char*)td->buffer2->GetBuffer();
	
	/*!
   *Build the environment string.
   */
  int scriptDirLen=0;
  int scriptFileLen= 0;
  int cgiRootLen= 0;
  int  cgiFileLen =0  ;
  int  scriptpathLen = strlen(scriptpath) + 1;

  if(td->scriptPath)
    delete [] td->scriptPath;
  
  td->scriptPath = new char[scriptpathLen];
  if(td->scriptPath == 0)
  {
    return ((http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
  }
	lstrcpy(td->scriptPath, scriptpath);

  MYSERVER_FILE::splitPathLength(scriptpath, &scriptDirLen, &scriptFileLen);
  
  if(td->scriptDir)
    delete [] td->scriptDir;
  td->scriptDir = new char[scriptDirLen];
  if(td->scriptDir == 0)
  {
		((vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((vhost*)(td->connection->host))->warningsLogWrite("Error allocating memory\r\n");
		((vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
    return ((http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
  }

  if(td->scriptFile)
    delete [] td->scriptFile;
  td->scriptFile = new char[scriptFileLen];
  if(td->scriptFile == 0)
  {
		((vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((vhost*)(td->connection->host))->warningsLogWrite("Error allocating memory\r\n");
		((vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
    return ((http*)td->lhttp)->raiseHTTPError(td,connection,e_500); 
  }


  MYSERVER_FILE::splitPathLength(cgipath, &cgiRootLen, &cgiFileLen);

  if(td->scriptDir)
    delete [] td->cgiRoot;
  td->cgiRoot = new char[cgiRootLen];
  if(td->cgiRoot == 0)
  {
		((vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((vhost*)(td->connection->host))->warningsLogWrite("Error allocating memory\r\n");
		((vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
    return ((http*)td->lhttp)->raiseHTTPError(td,connection,e_500); 
  }

  if(td->cgiFile)
    delete [] td->cgiFile;
  td->cgiFile = new char[cgiFileLen];
  if(td->cgiFile == 0)
  {
		((vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((vhost*)(td->connection->host))->warningsLogWrite("Error allocating memory\r\n");
		((vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
    return ((http*)td->lhttp)->raiseHTTPError(td,connection,e_500); 
  }


	MYSERVER_FILE::splitPath(scriptpath, td->scriptDir, td->scriptFile);
	MYSERVER_FILE::splitPath(cgipath, td->cgiRoot, td->cgiFile);


	connTable[connIndex].envString[0]='\0';
	cgi::buildCGIEnvironmentString(td,connTable[connIndex].envString);

	ZeroMemory(&ExtCtrlBlk, sizeof(ExtCtrlBlk));
	ExtCtrlBlk.cbSize = sizeof(ExtCtrlBlk);
	ExtCtrlBlk.dwVersion = MAKELONG(HSE_VERSION_MINOR, HSE_VERSION_MAJOR);
	ExtCtrlBlk.GetServerVariable = ISAPI_GetServerVariableExport;
	ExtCtrlBlk.ReadClient  = ISAPI_ReadClientExport;
	ExtCtrlBlk.WriteClient = ISAPI_WriteClientExport;
	ExtCtrlBlk.ServerSupportFunction = ISAPI_ServerSupportFunctionExport;
	ExtCtrlBlk.ConnID = (HCONN) (connIndex + 1);
	ExtCtrlBlk.dwHttpStatusCode = 200;
	ExtCtrlBlk.lpszLogData[0] = '0';
	ExtCtrlBlk.lpszMethod = td->request.CMD;
	ExtCtrlBlk.lpszQueryString = td->request.URIOPTS;
	ExtCtrlBlk.lpszPathInfo = td->pathInfo ? td->pathInfo : (CHAR*)"" ;
	if(td->pathInfo)
		ExtCtrlBlk.lpszPathTranslated = td->pathTranslated;
	else
		ExtCtrlBlk.lpszPathTranslated = td->filenamePath;
	ExtCtrlBlk.cbTotalBytes = td->inputData.getFileSize();
	ExtCtrlBlk.cbAvailable = 0;
	ExtCtrlBlk.lpbData = 0;
	ExtCtrlBlk.lpszContentType = (LPSTR)(&(td->request.CONTENT_TYPE[0]));

	connTable[connIndex].td->buffer->SetLength(0);
	connTable[connIndex].td->buffer->GetAt(0)='\0';
	HttpExtensionProc = (PFN_HTTPEXTENSIONPROC)GetProcAddress(AppHnd,
                                                            "HttpExtensionProc");
	if (HttpExtensionProc == NULL) 
	{
		((vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((vhost*)(td->connection->host))->warningsLogWrite(
                             "Failure to get pointer to HttpExtensionProc() in ISAPI \
                              application module\r\n");

		((vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		FreeLibrary((HMODULE)AppHnd);
    return ((http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
	}

	Ret = HttpExtensionProc(&ExtCtrlBlk);
	if (Ret == HSE_STATUS_PENDING) 
	{
		WaitForSingleObject(connTable[connIndex].ISAPIDoneEvent, ISAPI_TIMEOUT);
	}
	connTable[connIndex].connection->socket.send("\r\n\r\n",4, 0);

	int retvalue=0;

	switch(Ret) 
	{
		case HSE_STATUS_SUCCESS_AND_KEEP_CONN:
			retvalue=1;
			break;
		case 0:
		case HSE_STATUS_SUCCESS:
		case HSE_STATUS_ERROR:
		default:
			retvalue=0;
			break;
	}
	if(!FreeLibrary((HMODULE)AppHnd))
	{
		((vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((vhost*)(td->connection->host))->warningsLogWrite(
                                      "Failure to FreeLibrary in ISAPI module");
		((vhost*)(td->connection->host))->warningsLogWrite(cgipath);
		((vhost*)(td->connection->host))->warningsLogWrite("\r\n");
		((vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
	}

	connTable[connIndex].Allocated = 0;
	return retvalue;
#else
  /*!
   *On other archs returns a non implemented error. 
   */
	((vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
	((vhost*)td->connection->host)->warningsLogWrite(
                                   "Error ISAPI is not implemented\r\n");
	((vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);	
	return ((http*)td->lhttp)->raiseHTTPError(td,connection,e_501);
#endif	
}

/*!
*Constructor for the ISAPI class.
*/
isapi::isapi()
{

}

/*!
*Initialize the ISAPI engine under WIN32.
*/
void isapi::initISAPI()
{
#ifdef WIN32
	if(initialized)
		return;
	isapi_mutex = new myserver_mutex;
	max_Connections=lserver->getNumThreads();
	
	if(connTable)
		free(connTable);
		
	connTable = new ConnTableRecord[max_Connections];
	ZeroMemory(connTable,sizeof(ConnTableRecord)*max_Connections);
	InitializeCriticalSection(&GetTableEntryCritSec);	
	initialized=1;
#endif
}

/*!
 *Cleanup the memory used by ISAPI
 */
void isapi::cleanupISAPI()
{
#ifdef WIN32
	delete isapi_mutex;
	DeleteCriticalSection(&GetTableEntryCritSec);
	if(connTable)
		delete [] connTable;
	connTable=0;
	initialized=0;
#endif
}
