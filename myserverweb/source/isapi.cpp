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
#include "../include/dynamiclib.h"

#include <string>
#include <sstream>
using namespace std;

/*!
 *Initialize the timeout value to 15 seconds.
 */
u_long Isapi::timeout=MYSERVER_SEC(15);


#ifdef WIN32

u_long Isapi::max_Connections=0;
static CRITICAL_SECTION GetTableEntryCritSec;
int Isapi::initialized=0;
Mutex *Isapi::isapi_mutex=0;
ConnTableRecord *Isapi::connTable=0;


BOOL WINAPI ISAPI_ServerSupportFunctionExport(HCONN hConn, DWORD dwHSERRequest,
                                              LPVOID lpvBuffer, LPDWORD lpdwSize, 
                                              LPDWORD lpdwDataType) 
{
  string tmp;
	ConnTableRecord *ConnInfo;
  int ret;
	char *buffer=0;	
	char URI[MAX_PATH];/*! Under windows use MAX_PATH. */	

	Isapi::isapi_mutex->lock();
	ConnInfo = Isapi::HConnRecord(hConn);
	Isapi::isapi_mutex->unlock();
	if (ConnInfo == NULL) 
	{
    lserver->logLockAccess();
		lserver->logPreparePrintError();
		lserver->logWriteln("isapi::ServerSupportFunctionExport: invalid hConn");
		lserver->logEndPrintError();
    lserver->logUnlockAccess();
		return 0;
	}

	switch (dwHSERRequest) 
	{
		case HSE_REQ_MAP_URL_TO_PATH_EX:
			HSE_URL_MAPEX_INFO  *mapInfo;
			mapInfo=(HSE_URL_MAPEX_INFO*)lpdwDataType;
      mapInfo->lpszPath = 0;
      tmp.assign(mapInfo->lpszPath);
			ret=((Http*)ConnInfo->td->lhttp)->getPath(ConnInfo->td,ConnInfo->connection,
                                                tmp,(char*)lpvBuffer,0);
      if(ret!=e_200)
        return 1;
			mapInfo->cchMatchingURL=(DWORD)strlen((char*)lpvBuffer);
			mapInfo->cchMatchingPath=(DWORD)strlen(mapInfo->lpszPath);
      delete [] mapInfo->lpszPath;
      mapInfo->lpszPath = new char[tmp.length()+1];
      if(mapInfo->lpszPath == 0)
     if(buffer==0)
      {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;            
      }
       strcpy(mapInfo->lpszPath, tmp.c_str());
			mapInfo->dwFlags = HSE_URL_FLAGS_WRITE|HSE_URL_FLAGS_SCRIPT 
                                            | HSE_URL_FLAGS_EXECUTE;
			break;
		case HSE_REQ_MAP_URL_TO_PATH:
			if(((char*)lpvBuffer)[0])
				strcpy(URI,(char*)lpvBuffer);
			else
        lstrcpyn(URI,ConnInfo->td->request.URI.c_str(),
                 (int)ConnInfo->td->request.URI.length()-ConnInfo->td->pathInfo.length() +1);
			ret=((Http*)ConnInfo->td->lhttp)->getPath(ConnInfo->td,ConnInfo->connection,
                                            tmp ,URI,0);
      if(ret!=e_200)
      {
        if(buffer)
          delete [] buffer;
        return 1;
      }
      
      if(tmp.length() >= *lpdwSize)
      {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
      }    
      strcpy((char*)lpvBuffer, tmp.c_str());
      if(File::completePath((char**)&lpvBuffer,(int*)lpdwSize,  1))
      {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
      }
      *lpdwSize=(DWORD)strlen((char*)lpvBuffer);
      break;
  case HSE_REQ_SEND_URL_REDIRECT_RESP:
    return ((Isapi*)ConnInfo->lisapi)->Redirect(ConnInfo->td,
                              ConnInfo->connection,(char *)lpvBuffer);
    break;
  case HSE_REQ_SEND_URL:
    return ((Isapi*)ConnInfo->lisapi)->SendURI(ConnInfo->td,
                                           ConnInfo->connection,(char *)lpvBuffer);
    break;
  case HSE_REQ_SEND_RESPONSE_HEADER:
    return ((Isapi*)ConnInfo->lisapi)->SendHeader(ConnInfo->td,
                                           ConnInfo->connection,(char *)lpvBuffer);
    break;
  case HSE_REQ_DONE_WITH_SESSION:
    ConnInfo->td->response.httpStatus=*(DWORD*)lpvBuffer;
    SetEvent(ConnInfo->ISAPIDoneEvent);
    break;
  case HSE_REQ_IS_KEEP_CONN:
    if(!stringcmpi(ConnInfo->td->request.CONNECTION, "Keep-Alive"))
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
ConnTableRecord *Isapi::HConnRecord(HCONN hConn) 
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
int Isapi::Redirect(HttpThreadContext* td,ConnectionPtr a,char *URL) 
{
	return ((Http*)td->lhttp)->sendHTTPRedirect(td,a,URL);
}

/*!
 *Send an HTTP URI.
 */
int Isapi::SendURI(HttpThreadContext* td,ConnectionPtr a,char *URL)
{
  string tmp;
  tmp.assign(URL);
	return ((Http*)td->lhttp)->sendHTTPResource(td, a, tmp, 0, 0);
}

/*!
 *Send the ISAPI header.
 */
int Isapi::SendHeader(HttpThreadContext* td,ConnectionPtr a,char *URL)
{
	string tmp;
  tmp.assign(URL);
  return ((Http*)td->lhttp)->sendHTTPResource(td, a, tmp, 0, 1);
}

/*!
 *Write directly to the output.
 */
BOOL WINAPI ISAPI_WriteClientExport(HCONN hConn, LPVOID Buffer, LPDWORD lpdwBytes, 
                                    DWORD /*!dwReserved*/)
{
	int keepalive;
	char* buffer;
	ConnTableRecord *ConnInfo;
	char chunk_size[15];
	u_long nbw=0;
	if(*lpdwBytes==0)
		return 1;
	
  Isapi::isapi_mutex->lock();
	ConnInfo = Isapi::HConnRecord(hConn);
	Isapi::isapi_mutex->unlock();
	
  if(ConnInfo == NULL)
    return 1;
	buffer=(char*)ConnInfo->td->buffer->GetBuffer();
	if (ConnInfo == NULL) 
	{
		((Vhost*)(ConnInfo->td->connection->host))->warningslogRequestAccess(
                                                                ConnInfo->td->id);
		((Vhost*)(ConnInfo->td->connection->host))->warningsLogWrite(
                                        "isapi::WriteClientExport: invalid hConn\r\n");
		((Vhost*)(ConnInfo->td->connection->host))->warningslogTerminateAccess(
                                                                ConnInfo->td->id);
		return 0;
	}
	keepalive =(!stringcmpi(ConnInfo->td->request.CONNECTION, "Keep-Alive")) ;

  /*If the HTTP header was sent do not send it again. */
	if(!ConnInfo->headerSent)
	{
	  int headerSize=0;
		strncat(buffer,(char*)Buffer,*lpdwBytes);
		ConnInfo->headerSize+=*lpdwBytes;
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
    /*!
     *Handle the HTTP header if exists.
     */
		if(headerSize)
		{
			int len = ConnInfo->headerSize-headerSize;
      HttpHeaders::buildHTTPResponseHeaderStruct(&ConnInfo->td->response,
                                  ConnInfo->td,(char*)ConnInfo->td->buffer->GetBuffer());
			if(!ConnInfo->td->appendOutputs)
			{
				if(keepalive)
					ConnInfo->td->response.TRANSFER_ENCODING.assign("chunked");

				if(keepalive)
					ConnInfo->td->response.CONNECTION.assign("Keep-Alive");
				else
					ConnInfo->td->response.CONNECTION.assign("Close");
				HttpHeaders::buildHTTPResponseHeader(
                 (char*)ConnInfo->td->buffer2->GetBuffer(),&(ConnInfo->td->response));
	
				if(ConnInfo->connection->socket.send(
                     (char*)ConnInfo->td->buffer2->GetBuffer(),
                     (int)strlen((char*)ConnInfo->td->buffer2->GetBuffer()), 0)==-1)
					return 0;
			}
      /*! Save the headerSent status. */
			ConnInfo->headerSent=1;
      
      /*! If only the header was requested return. */
      if(ConnInfo->headerSent && ConnInfo->only_header)
        return 0;

			/*!Send the first chunk. */
			if(len)
			{
        /*! With keep-alive connections use chunks.*/
				if(keepalive && (!ConnInfo->td->appendOutputs))
				{
					sprintf(chunk_size,"%x\r\n",len);
          nbw=ConnInfo->connection->socket.send(chunk_size,
                             (int)strlen(chunk_size),0);
					if((nbw == (u_long)-1))
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
					if((nbw == (u_long)-1))
						return 0;
          ConnInfo->dataSent += nbw;
				}

        /*! Send the chunk tailer.*/
				if(keepalive && (!ConnInfo->td->appendOutputs))
				{
					nbw = ConnInfo->connection->socket.send("\r\n",2, 0);
					if((nbw == (u_long)-1))
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
	
  Isapi::isapi_mutex->lock();
	ConnInfo = Isapi::HConnRecord(hConn);
  Isapi::isapi_mutex->unlock();
	if (ConnInfo == NULL) 
	{
		((Vhost*)(ConnInfo->td->connection->host))->warningslogRequestAccess(ConnInfo->td->id);
		((Vhost*)(ConnInfo->td->connection->host))->warningsLogWrite("isapi::ReadClientExport: invalid hConn\r\n");
		((Vhost*)(ConnInfo->td->connection->host))->warningslogTerminateAccess(ConnInfo->td->id);
		return 0;
	}
	ConnInfo->td->inputData.readFromFile((char*)lpvBuffer, *lpdwSize, &NumRead);
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
BOOL WINAPI ISAPI_GetServerVariableExport(HCONN hConn, LPSTR lpszVariableName, 
                                          LPVOID lpvBuffer, LPDWORD lpdwSize) 
{
	ConnTableRecord *ConnInfo;
	BOOL ret =1;
	Isapi::isapi_mutex->lock();
	ConnInfo = Isapi::HConnRecord(hConn);
	Isapi::isapi_mutex->unlock();
	if (ConnInfo == NULL) 
	{
    lserver->logLockAccess();
    lserver->logPreparePrintError();
		lserver->logWriteln("isapi::GetServerVariableExport: invalid hConn");
		lserver->logEndPrintError();
    lserver->logUnlockAccess();
		return 0;
	}

	if (!strcmp(lpszVariableName, "ALL_HTTP")) 
	{
		if(Isapi::buildAllHttpHeaders(ConnInfo->td,ConnInfo->connection, 
                                  lpvBuffer, lpdwSize))
			ret=1;
		else
		{
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
			ret=0;
		}
			
	}else if(!strcmp(lpszVariableName, "ALL_RAW")) 
	{
		if(Isapi::buildAllRawHeaders(ConnInfo->td,ConnInfo->connection,lpvBuffer,lpdwSize))
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
		char *localEnv;
		int variableNameLen;
		((char*)lpvBuffer)[0]='\0';
	  localEnv=ConnInfo->envString;
		variableNameLen=(int)strlen(lpszVariableName);
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
BOOL Isapi::buildAllHttpHeaders(HttpThreadContext* td,ConnectionPtr /*!a*/,
                                LPVOID output,LPDWORD dwMaxLen)
{
	DWORD valLen=0;
	DWORD maxLen=*dwMaxLen;
	char *ValStr=(char*)output;

	if(td->request.ACCEPT[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_ACCEPT:%s\n",td->request.ACCEPT.c_str());
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.ACCEPTLAN[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_ACCEPT_LANGUAGE:%s\n",
                    td->request.ACCEPTLAN.c_str());
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.CACHE_CONTROL[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_CACHE_CONTROL:%s\n",
                    td->request.CACHE_CONTROL.c_str());
	else if(valLen+30<maxLen) 
		return 0;

	if((td->request.RANGEBYTEBEGIN || td->request.RANGEBYTEEND) && (valLen+30<maxLen))
	{
    ostringstream rangeBuffer;
		rangeBuffer << "HTTP_RANGE:" << td->request.RANGETYPE << "=" ;
    if(td->request.RANGEBYTEBEGIN)
    {
      rangeBuffer << (int)td->request.RANGEBYTEBEGIN;
    }
    rangeBuffer << "-";
    if(td->request.RANGEBYTEEND)
    {
      rangeBuffer << td->request.RANGEBYTEEND;
     }   
		valLen+=sprintf(&ValStr[valLen],"%s\n",rangeBuffer.str().c_str());
	}
  		
	if(td->request.ACCEPTENC[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_ACCEPT_ENCODING:%s\n",
                    td->request.ACCEPTENC.c_str());
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.CONNECTION[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_CONNECTION:%s\n", 
                    td->request.CONNECTION.c_str());
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.COOKIE[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_COOKIE:%s\n",td->request.COOKIE.c_str());
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.HOST[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_HOST:%s\n",td->request.HOST.c_str());
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.DATE[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_DATE:%s\n",td->request.DATE.c_str());
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.MODIFIED_SINCE[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_IF_MODIFIED_SINCE:%s\n",
                    td->request.MODIFIED_SINCE.c_str());
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.REFERER[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_REFERER:%s\n", 
                    td->request.REFERER.c_str());
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.PRAGMA[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_PRAGMA:%s\n", 
                    td->request.PRAGMA.c_str());
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.USER_AGENT[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_USER_AGENT:%s\n", 
                    td->request.USER_AGENT.c_str());
	else if(valLen+30<maxLen)
		return 0;

	if(td->request.FROM[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_FROM:%s\n",td->request.FROM.c_str());
	else if(valLen+30<maxLen) 
		return 0;
	return 1;
}

/*!
 *Build the string that contains all the headers.
 */
BOOL Isapi::buildAllRawHeaders(HttpThreadContext* td,ConnectionPtr a,
                               LPVOID output,LPDWORD dwMaxLen)
{
	DWORD valLen=0;
	DWORD maxLen=*dwMaxLen;
	char *ValStr=(char*)output;
	if(buildAllHttpHeaders(td,a,output,dwMaxLen)==0)
		return 0;
	valLen=(DWORD)strlen(ValStr);

	if(td->pathInfo.length() && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"PATH_INFO:%s\n",td->pathInfo.c_str());
	else if(valLen+30<maxLen) 
		return 0;
	
	if(td->pathTranslated.length() && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"PATH_INFO:%s\n",td->pathTranslated.c_str());
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

	if(td->filenamePath.length() && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"SCRIPT_FILENAME:%s\n",td->filenamePath[0]);
	else if(valLen+30<maxLen) 
		return 0;

	if(valLen+30<maxLen)
		valLen+=sprintf(&ValStr[valLen],"SERVER_PORT:%u\n",td->connection->getLocalPort());
	else if(valLen+30<maxLen) 
		return 0;

	if(valLen+30<maxLen)
		valLen+=sprintf(&ValStr[valLen],"SERVER_SIGNATURE:<address>%s</address>\n",
                    versionOfSoftware);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->connection->getIpAddr()[0] && valLen+30<maxLen)
		valLen+=sprintf(&ValStr[valLen],"REMOTE_ADDR:\n",td->connection->getIpAddr());
	else if(valLen+30<maxLen) 
		return 0;

	if(td->connection->getPort() && valLen+30<maxLen)
		valLen+=sprintf(&ValStr[valLen],"REMOTE_PORT:%u\n",td->connection->getPort());
	else if(valLen+30<maxLen) 
		return 0;

	if(valLen+30<maxLen)
		valLen+=sprintf(&ValStr[valLen],"SERVER_ADMIN:%s\n",lserver->getServerAdmin());
	else if(valLen+30<maxLen) 
		return 0;

	if(valLen+MAX_PATH<maxLen)
	{
		valLen+=sprintf(&ValStr[valLen],"SCRIPT_NAME:");
		lstrcpyn(&ValStr[valLen],td->request.URI.c_str(),
             td->request.URI.length()- td->pathInfo.length()+1);
		valLen+=(DWORD)td->request.URI.length()-td->pathInfo.length()+1;
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
int Isapi::send(HttpThreadContext* td,ConnectionPtr connection, 
                const char* scriptpath, const char *cgipath, 
                int execute,int only_header)
{
/*!
 *ISAPI works only on the windows architecture.
 */
#ifdef NOT_WIN
  return ((Http*)td->lhttp)->raiseHTTPError(td, connection, e_501);
#endif

#ifdef WIN32
	DWORD Ret;
	EXTENSION_CONTROL_BLOCK ExtCtrlBlk;
	DynamicLibrary appHnd;
	HSE_VERSION_INFO Ver;
	u_long connIndex;
	PFN_GETEXTENSIONVERSION GetExtensionVersion;
	PFN_HTTPEXTENSIONPROC HttpExtensionProc;
  const char *loadLib;
	int retvalue=0;
	char fullpath[MAX_PATH*2];/*! Under windows there is MAX_PATH so use it. */
	if(!execute)
	{
    if(cgipath && strlen(cgipath))
			sprintf(fullpath,"%s \"%s\"",cgipath,td->filenamePath.c_str());
		else
			sprintf(fullpath,"%s",td->filenamePath.c_str());
	}
	else
	{
		sprintf(fullpath,"%s",cgipath);
	}
	EnterCriticalSection(&GetTableEntryCritSec);
	connIndex = 0;
	Isapi::isapi_mutex->lock();
	while ((connTable[connIndex].Allocated != 0) && (connIndex < max_Connections)) 
	{
		connIndex++;
	}
	Isapi::isapi_mutex->unlock();
	LeaveCriticalSection(&GetTableEntryCritSec);

	if (connIndex == max_Connections) 
	{
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)td->connection->host)->warningsLogWrite(
                                            "ISAPI: Error max connections\r\n");
		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_503);
	}
	if(execute)
  	loadLib=scriptpath;
	else
	  loadLib=cgipath;

  Ret=appHnd.loadLibrary(loadLib);

  if(Ret)
	{
    string msg;
    msg.assign("ISAPI: Cannot load library ");
    msg.append(loadLib);
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)td->connection->host)->warningsLogWrite(msg.c_str());
		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
	}
	connTable[connIndex].connection = connection;
	connTable[connIndex].td = td;
  connTable[connIndex].only_header = only_header;
	connTable[connIndex].headerSent=0;
	connTable[connIndex].headerSize=0;
	connTable[connIndex].dataSent=0;
	connTable[connIndex].Allocated = 1;
	connTable[connIndex].lisapi = this;
	connTable[connIndex].ISAPIDoneEvent = CreateEvent(NULL, 1, 0, NULL);
  
	GetExtensionVersion = (PFN_GETEXTENSIONVERSION)appHnd.getProc("GetExtensionVersion");

	if (GetExtensionVersion == NULL) 
	{
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)(td->connection->host))->warningsLogWrite(
           "ISAPI: Failed to get pointer to GetExtensionVersion() in ISAPI application\r\n");
		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		if(!appHnd.close())
		{
			((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
			((Vhost*)(td->connection->host))->warningsLogWrite(
                                    "ISAPI: Failed to FreeLibrary in ISAPI module: ");
			((Vhost*)(td->connection->host))->warningsLogWrite(cgipath);
			((Vhost*)(td->connection->host))->warningsLogWrite("\r\n");
			((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		}
		return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
	}
	if(!GetExtensionVersion(&Ver)) 
	{
    ((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)(td->connection->host))->warningsLogWrite(
                               "ISAPI: GetExtensionVersion() returned FALSE\r\n");
		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		if(!appHnd.close())
		{
			((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
			((Vhost*)(td->connection->host))->warningsLogWrite(
                              "ISAPI: Failed to FreeLibrary in ISAPI module: ");
			((Vhost*)(td->connection->host))->warningsLogWrite(cgipath);
			((Vhost*)(td->connection->host))->warningsLogWrite("\r\n");
			((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		}
		return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
	}
	if (Ver.dwExtensionVersion > MAKELONG(HSE_VERSION_MINOR, HSE_VERSION_MAJOR)) 
	{
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)(td->connection->host))->warningsLogWrite(
                                       "ISAPI: version not supported\r\n");
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		if(!appHnd.close())
		{
			((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
			((Vhost*)(td->connection->host))->warningsLogWrite(
                                  "ISAPI: Failed to FreeLibrary in ISAPI module: ");
			((Vhost*)(td->connection->host))->warningsLogWrite(cgipath);
			((Vhost*)(td->connection->host))->warningsLogWrite("\r\n");
			((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		}
		return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
	}
	/*!
   *Store the environment string in the buffer2.
	*/
	connTable[connIndex].envString=td->buffer2->GetBuffer();
	
	/*!
   *Build the environment string.
   */
	td->scriptPath.assign(scriptpath);

  {
    string tmp;
    tmp.assign(cgipath);
    File::splitPath(tmp, td->cgiRoot, td->cgiFile);
    tmp.assign(scriptpath);
    File::splitPath(tmp, td->scriptDir, td->scriptFile);
  }
	connTable[connIndex].envString[0]='\0';
	Cgi::buildCGIEnvironmentString(td,connTable[connIndex].envString);
  
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
	ExtCtrlBlk.lpszMethod = (char*)td->request.CMD.c_str();
	ExtCtrlBlk.lpszQueryString =(char*) td->request.URIOPTS.c_str();
	ExtCtrlBlk.lpszPathInfo = td->pathInfo.length() ? (char*)td->pathInfo.c_str() : (CHAR*)"" ;
	if(td->pathInfo.length())
		ExtCtrlBlk.lpszPathTranslated = (char*)td->pathTranslated.c_str();
	else
		ExtCtrlBlk.lpszPathTranslated = (char*)td->filenamePath.c_str();
	ExtCtrlBlk.cbTotalBytes = td->inputData.getFileSize();
	ExtCtrlBlk.cbAvailable = 0;
	ExtCtrlBlk.lpbData = 0;
	ExtCtrlBlk.lpszContentType =(char*)td->request.CONTENT_TYPE.c_str();

	connTable[connIndex].td->buffer->SetLength(0);
	connTable[connIndex].td->buffer->GetAt(0)='\0';
	HttpExtensionProc = (PFN_HTTPEXTENSIONPROC)appHnd.getProc("HttpExtensionProc");
	if (HttpExtensionProc == NULL) 
	{
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)(td->connection->host))->warningsLogWrite(
                             "ISAPI: Failed to get pointer to HttpExtensionProc() in ISAPI \
                              application module\r\n");

		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
    appHnd.close();
    return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
	}

	Ret = HttpExtensionProc(&ExtCtrlBlk);
	if (Ret == HSE_STATUS_PENDING) 
	{
		WaitForSingleObject(connTable[connIndex].ISAPIDoneEvent, timeout);
	}
	connTable[connIndex].connection->socket.send("\r\n\r\n",4, 0);

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
	if(!appHnd.close())
	{
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)(td->connection->host))->warningsLogWrite(
                                      "ISAPI: Failed to FreeLibrary module");
		((Vhost*)(td->connection->host))->warningsLogWrite(cgipath);
		((Vhost*)(td->connection->host))->warningsLogWrite("\r\n");
		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
	}

	{
    ostringstream tmp;
    tmp <<  connTable[connIndex].dataSent;
    connTable[connIndex].td->response.CONTENT_LENGTH.assign(tmp.str()); 
  }                                      

	connTable[connIndex].Allocated = 0;
	return retvalue;
#else
  /*!
   *On other archs returns a non implemented error. 
   */
	((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
	((Vhost*)td->connection->host)->warningsLogWrite(
                                   "Error ISAPI is not implemented\r\n");
	((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);	
	return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_501);
#endif	
}

/*!
*Constructor for the ISAPI class.
*/
Isapi::Isapi()
{

}

/*!
*Initialize the ISAPI engine under WIN32.
*/
int Isapi::load(XmlParser*/* confFile*/)
{
#ifdef WIN32
  u_long n_threads = lserver->getNumThreads();
	if(initialized)
		return 0;
	isapi_mutex = new Mutex;
	max_Connections= n_threads ? n_threads : 20 ;
	
	if(connTable)
		free(connTable);
		
	connTable = new ConnTableRecord[max_Connections];
  for(int i=0;i<max_Connections; i++)
  {
   	connTable[i].Allocated=0;
    connTable[i].only_header=0;
	  connTable[i].headerSent=0;
	  connTable[i].headerSize=0;
	  connTable[i].td=0;
    connTable[i].dataSent=0;
  	connTable[i].envString=0;
  	connTable[i].connection=0;
  	connTable[i].ISAPIDoneEvent=0;
  	connTable[i].lisapi=0;         
  }
	InitializeCriticalSection(&GetTableEntryCritSec);	
	initialized=1;
#endif
  return 0;
}

/*!
 *Cleanup the memory used by ISAPI.
 */
int Isapi::unload()
{
#ifdef WIN32
	delete isapi_mutex;
	DeleteCriticalSection(&GetTableEntryCritSec);
	if(connTable)
		delete [] connTable;
	connTable=0;
	initialized=0;
#endif
  return 0;
}

/*!
 *Set a new timeout value used with the isapi modules.
 */
void Isapi::setTimeout(u_long ntimeout)
{
  timeout = ntimeout;
}

/*!
 *Return the timeout value used with the isapi modules.
 */
u_long Isapi::getTimeout()
{
  return timeout;
}
