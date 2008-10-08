/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007, 2008 Free Software Foundation, Inc.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"
#include <include/http_handler/wincgi/wincgi.h>
#include <include/base/base64/mime_utils.h>
#include <include/base/file/file.h>
#include <include/base/file/files_utility.h>
#include <include/server/server.h>
#include <include/protocol/http/http_headers.h>
#include <include/protocol/http/http.h>
#include <include/base/utility.h>
#include <include/base/string/stringutils.h>
#include <include/filter/filters_chain.h>
#include <include/base/safetime/safetime.h>

extern "C" 
{
#ifdef WIN32
#include <direct.h>
#endif
#include <string.h>
}

#include <string>
#include <sstream>
using namespace std;

/*!
 *Initialize the timeout value to 15 seconds.
 */
u_long WinCgi::timeout = MYSERVER_SEC(15);

/*!
 *Set a new timeout value to use for new processes.
 */
void WinCgi::setTimeout(u_long ntimeout)
{
  timeout = ntimeout;
}

/*!
 *Constructor for the wincgi class.
 */
WinCgi::WinCgi()
{

}

/*!
 *Destructor for the wincgi class.
 */
WinCgi::~WinCgi()
{

}

/*!
 *Get the timeout value for the new process. 
 */
u_long WinCgi::getTimeout()
{
  return timeout;
}

/*!
 *Send the WinCGI data.
 */
int WinCgi::send(HttpThreadContext* td,ConnectionPtr s,const char* filename, 
                 const char* cmdLine, int /*execute*/, int onlyHeader)
{
#ifdef WIN32
  FiltersChain chain;
  Process proc;
  u_long nbr;
  char  dataFilePath[MAX_PATH];/*! Use MAX_PATH under windows. */
  char outFilePath[MAX_PATH];  /*! Use MAX_PATH under windows. */
  char *buffer;
  File DataFileHandle, OutFileHandle;
  StartProcInfo spi;
  time_t ltime = 100;
  int gmhour;
  int bias;
  
  int ret;
  char execname[MAX_PATH];/*! Use MAX_PATH under windows. */
  char pathname[MAX_PATH];/*! Use MAX_PATH under windows. */

  u_long nBytesRead = 0;
  u_long headerSize = 0;
  u_long nbw = 0;
  ostringstream stream;

  if(!FilesUtility::fileExists(filename))
    return td->http->raiseHTTPError(404);

  FilesUtility::splitPath(filename,pathname,execname);
  
  getdefaultwd(dataFilePath,MAX_PATH);
  GetShortPathName(dataFilePath,dataFilePath,MAX_PATH);
  sprintf(&dataFilePath[strlen(dataFilePath)],"/data_%u.ini",td->id);
  
  strcpy(outFilePath,td->outputDataPath.c_str());
  strcat(outFilePath,"WC");
  td->inputData.setFilePointer(0);

  chain.setProtocol(td->http);
  chain.setProtocolData(td);
  chain.setStream(td->connection->socket);
  if(td->mime)
  {
    u_long nbw2;
    if(td->mime && Server::getInstance()->getFiltersFactory()->chain(&chain, 
                                                 td->mime->filters, 
                                                 td->connection->socket, &nbw2, 1))
      {
        td->connection->host->warningsLogRequestAccess(td->id);
        td->connection->host->warningsLogWrite("WinCGI: Error loading filters");
        td->connection->host->warningsLogTerminateAccess(td->id);

        chain.clearAllFilters(); 
        return td->http->raiseHTTPError(500);
      }
  }
  

  /*!
   *The WinCGI protocol uses a .ini file to send data to the new process.
   */
  ret=DataFileHandle.openFile(dataFilePath,
                     File::MYSERVER_CREATE_ALWAYS|File::MYSERVER_OPEN_WRITE);
  if ( ret ) 
  {
    td->connection->host->warningsLogRequestAccess(td->id);
    td->connection->host->warningsLogWrite("WinCGI: Error creating .ini");
    td->connection->host->warningsLogTerminateAccess(td->id);
    return td->http->raiseHTTPError(500);
  }

  td->buffer2->setLength(0);
  buffer=td->buffer2->getBuffer();

  strcpy(buffer, "[CGI]\r\n");
  DataFileHandle.writeToFile(buffer,7,&nbr);

  strcpy(buffer, "CGI Version=CGI/1.3a WIN\r\n");
  DataFileHandle.writeToFile(buffer,26,&nbr);

  *td->buffer2 << "Server Admin=" << Server::getInstance()->getServerAdmin() << "\r\n";
  DataFileHandle.writeToFile(buffer,td->buffer2->getLength(),&nbr);

  {
    if(td->request.isKeepAlive())
    {
      strcpy(buffer,"Request Keep-Alive=No\r\n");
      DataFileHandle.writeToFile(buffer, 23, &nbr);
    }
    else
    {
      strcpy(buffer,"Request Keep-Alive=Yes\r\n");
      DataFileHandle.writeToFile(buffer, 24, &nbr);
    }
  }

  td->buffer2->setLength(0);
  *td->buffer2 << "Request Method=" << td->request.cmd << "\r\n";
  DataFileHandle.writeToFile(buffer, td->buffer2->getLength(), &nbr);

  td->buffer2->setLength(0);
  *td->buffer2 << "Request Protocol=HTTP/" << td->request.ver << "\r\n";
  DataFileHandle.writeToFile(buffer, td->buffer2->getLength(), &nbr);

   td->buffer2->setLength(0);  
  *td->buffer2 << "Executable Path=" << execname << "\r\n";
  DataFileHandle.writeToFile(buffer,td->buffer2->getLength(),&nbr);

  if(td->request.uriOpts[0])
  {
    sprintf(buffer, "Query String=%s\r\n", td->request.uriOpts.c_str());
    DataFileHandle.writeToFile(buffer,(u_long)strlen(buffer), &nbr);
  }

  {
    HttpRequestHeader::Entry *referer = td->request.other.get("Referer");

    if(referer && referer->value->length())
    {
      sprintf(buffer,"Referer=%s\r\n", referer->value->c_str());
      DataFileHandle.writeToFile(buffer,(u_long)strlen(buffer),&nbr);
    }
  }

  {
    HttpRequestHeader::Entry *contentType = td->request.other.get("Content-Type");

    if(contentType && contentType->value->length())
    {
      sprintf(buffer, "Content Type=%s\r\n", contentType->value->c_str());
      DataFileHandle.writeToFile(buffer, (u_long)strlen(buffer), &nbr);
    }
  }

  {
    HttpRequestHeader::Entry *userAgent = td->request.other.get("User-Agent");
    
    if(userAgent && userAgent->value->length())
    {
      sprintf(buffer,"User Agent=%s\r\n", userAgent->value->c_str());
      DataFileHandle.writeToFile(buffer, (u_long)strlen(buffer), &nbr);
    }
  }

  sprintf(buffer,"Content File=%s\r\n", td->inputData.getFilename());
  DataFileHandle.writeToFile(buffer, (u_long)strlen(buffer), &nbr);

  if(td->request.contentLength[0])
  {
    sprintf(buffer, "Content Length=%s\r\n", 
            td->request.contentLength.c_str());
    DataFileHandle.writeToFile(buffer, (u_long)strlen(buffer), &nbr);
  }
  else
  {
    strcpy(buffer, "Content Length=0\r\n");
    DataFileHandle.writeToFile(buffer, 18, &nbr);  
  }

  strcpy(buffer,"Server Software=MyServer\r\n");
  DataFileHandle.writeToFile(buffer, 26, &nbr);

  sprintf(buffer, "Remote Address=%s\r\n", td->connection->getIpAddr());
  DataFileHandle.writeToFile(buffer, (u_long)strlen(buffer), &nbr);

  sprintf(buffer, "Server Port=%u\r\n", td->connection->getLocalPort());
  DataFileHandle.writeToFile(buffer, (u_long)strlen(buffer), &nbr);
  
  {
    HttpRequestHeader::Entry *host = td->request.other.get("Host");
    if(host)
      sprintf(buffer, "Server Name=%s\r\n", host->value->c_str());
    DataFileHandle.writeToFile(buffer, (u_long)strlen(buffer), &nbr);
  }

  strcpy(buffer, "[System]\r\n");
  DataFileHandle.writeToFile(buffer, 10, &nbr);

  sprintf(buffer, "Output File=%s\r\n", outFilePath);
  DataFileHandle.writeToFile(buffer, (u_long)strlen(buffer), &nbr);

  sprintf(buffer,"Content File=%s\r\n", td->inputData.getFilename());
  DataFileHandle.writeToFile(buffer, (u_long)strlen(buffer), &nbr);

  /*
   *Compute the local offset from the GMT time
   */
  {
    tm tmpTm;
    ltime = 100;
    gmhour = myserver_gmtime( &ltime, &tmpTm)->tm_hour;
    bias = myserver_localtime(&ltime, &tmpTm)->tm_hour - gmhour;
  }
  sprintf(buffer, "GMT Offset=%i\r\n", bias);
  DataFileHandle.writeToFile(buffer, (u_long)strlen(buffer), &nbr);

  sprintf(buffer, "Debug Mode=No\r\n", bias);
  DataFileHandle.writeToFile(buffer, 15, &nbr);

  DataFileHandle.close();

  /*
   *Create the out file.
   */
  if(!FilesUtility::fileExists(outFilePath))
  {
    ret = OutFileHandle.openFile(outFilePath, File::MYSERVER_CREATE_ALWAYS);
    if (ret)
    {
      td->connection->host->warningsLogRequestAccess(td->id);
      td->connection->host->warningsLogWrite(
                                      "WinCGI: Error creating output file");
      td->connection->host->warningsLogTerminateAccess(td->id);
      DataFileHandle.close();
      FilesUtility::deleteFile(outFilePath);
      FilesUtility::deleteFile(dataFilePath);
      chain.clearAllFilters(); 
      return td->http->raiseHTTPError(500);
    }
  }
  OutFileHandle.close();
  spi.cmdLine.assign("cmd /c \"");
  spi.cmdLine.append(filename);
  spi.cmdLine.append("\" ");
  spi.cmdLine.append(dataFilePath);
  spi.cwd.assign(pathname);
  spi.envString = 0;
  if (proc.execHiddenProcess(&spi, timeout))
  {
    ostringstream msg;
    msg << "WinCGI: Error executing process " << filename;
    td->connection->host->warningsLogRequestAccess(td->id);
    td->connection->host->warningsLogWrite(msg.str().c_str());
    td->connection->host->warningsLogTerminateAccess(td->id);
    FilesUtility::deleteFile(outFilePath);
    FilesUtility::deleteFile(dataFilePath);
    chain.clearAllFilters();
    return td->http->raiseHTTPError(500);
  }

  ret=OutFileHandle.openFile(outFilePath, File::MYSERVER_OPEN_ALWAYS|
                             File::MYSERVER_OPEN_READ);
  if (ret)
  {
    ostringstream msg;
    msg << "WinCGI: Error opening output file " << outFilePath;
    td->connection->host->warningsLogRequestAccess(td->id);
    td->connection->host->warningsLogWrite(msg.str().c_str());
    td->connection->host->warningsLogTerminateAccess(td->id);
    chain.clearAllFilters();
    return td->http->raiseHTTPError(500);
  }
  OutFileHandle.readFromFile(buffer,td->buffer2->getRealLength(),&nBytesRead);
  if(!nBytesRead)
  {
    ostringstream msg;
    msg << "WinCGI: Error zero bytes read from the WinCGI output file " 
        << outFilePath;
    td->connection->host->warningsLogRequestAccess(td->id);
    td->connection->host->warningsLogWrite(msg.str().c_str());
    td->connection->host->warningsLogTerminateAccess(td->id);
    OutFileHandle.close();
    FilesUtility::deleteFile(outFilePath);
    FilesUtility::deleteFile(dataFilePath);
    chain.clearAllFilters();
    return td->http->raiseHTTPError(500);
  }

  for(u_long i = 0; i < nBytesRead; i++)
  {
    if((buffer[i] == '\r') && (buffer[i+1] == '\n')
       &&(buffer[i+2] == '\r') && (buffer[i+3] == '\n'))
    {
      /*
       *The HTTP header ends with a \r\n\r\n sequence so
       *determinate where it ends and set the header size
       *to i + 4.
       */
      headerSize = i + 4;
      break;
    }
  }

  if(td->request.isKeepAlive())
    td->response.connection.assign("keep-alive");

  HttpHeaders::buildHTTPResponseHeaderStruct(buffer, &td->response, &(td->nBytesToRead));

  /*
   *Always specify the size of the HTTP contents.
   */
  stream << OutFileHandle.getFileSize() - headerSize;
  td->response.contentLength.assign(stream.str());
  if(!td->appendOutputs)
  {
    u_long nbw2;
    /*!
     *Send the header if it is not appending.
     */
    HttpHeaders::buildHTTPResponseHeader(td->buffer->getBuffer(),
                                          &td->response);
    if(chain.write((const char*)td->buffer->getBuffer(),
                          (int)strlen((const char*)td->buffer->getBuffer()), 
                   &nbw2))
    {
      OutFileHandle.close();
      FilesUtility::deleteFile(outFilePath);
      FilesUtility::deleteFile(dataFilePath);
      chain.clearAllFilters();
      return 0;
    }
      
    if(onlyHeader)
    {
      OutFileHandle.close();
      FilesUtility::deleteFile(outFilePath);
      FilesUtility::deleteFile(dataFilePath);
      chain.clearAllFilters();
      return 1;
    }
    /*!
     *Send other data in the buffer.
     */
    chain.write((char*)(buffer + headerSize), nBytesRead - headerSize, 
                &nbw2);
    nbw += nbw2;
  }
  else
  {
    u_long nbw2;
    HttpHeaders::buildHTTPResponseHeader(td->buffer->getBuffer(),
                                         &td->response);
    if(onlyHeader)
    {
      chain.clearAllFilters();
      return 1;
    }
    
    td->outputData.writeToFile((char*)(buffer + headerSize),
                               nBytesRead - headerSize,&nbw2);
    nbw += nbw2;
  }

  /* Flush the rest of the file.  */
  do
  {
    OutFileHandle.readFromFile(buffer, td->buffer2->getLength(), 
                               &nBytesRead);
    if(nBytesRead)
    {
      int ret;
      if(td->appendOutputs)
      {
        ret = td->outputData.writeToFile(buffer, nBytesRead, &nbw);
        if(ret)
        {
          OutFileHandle.close();
          FilesUtility::deleteFile(outFilePath);
          FilesUtility::deleteFile(dataFilePath);
          chain.clearAllFilters();
          return 0;
        }
      }      
      else
      {
        u_long nbw2;
        ret = chain.write((char*)buffer, nBytesRead, &nbw2);
        if(ret == -1)
        {
          OutFileHandle.close();
          FilesUtility::deleteFile(outFilePath);
          FilesUtility::deleteFile(dataFilePath);
          chain.clearAllFilters();
          return 0;
        }
      }
    }
    else
      break;

  }while(nBytesRead);

  td->sentData += nbw;

  chain.clearAllFilters();  
  OutFileHandle.close();
  FilesUtility::deleteFile(outFilePath);
  FilesUtility::deleteFile(dataFilePath);
  return 1;
#else
  /*
   *WinCGI is available only under windows. Raise the right error 
   *when it is used under another architecture.
   */
  td->buffer->setLength(0);
  *td->buffer << "WinCGI: Not implemented";
  td->connection->host->warningsLogRequestAccess(td->id);
  td->connection->host->warningsLogWrite(td->buffer->getBuffer());
  td->connection->host->warningsLogTerminateAccess(td->id);
  return td->http->raiseHTTPError(501);
#endif
}
