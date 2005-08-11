/*
MyServer
Copyright (C) 2002, 2003, 2004 The MyServer Team
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "../include/security.h"
#include "../include/AMMimeUtils.h"
#include "../include/filemanager.h"
#include "../include/sockets.h"
#include "../include/cserver.h"
#include "../include/http_headers.h"
#include "../include/http.h"
#include "../include/HTTPmsg.h"
#include "../include/utility.h"
#include "../include/winCGI.h"
#include "../include/stringutils.h"
#include "../include/filters_chain.h"
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
u_long WinCgi::timeout=MYSERVER_SEC(15);

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
                 int /*execute*/, int onlyHeader)
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
	time_t ltime=100;
	int gmhour;
	int bias;
  
  int ret;
	char execname[MAX_PATH];/*! Use MAX_PATH under windows. */
	char pathname[MAX_PATH];/*! Use MAX_PATH under windows. */

	u_long nBytesRead=0;
	u_long headerSize=0;
	u_long nbw=0;
  ostringstream stream;

	if(!File::fileExists(filename))
		return ((Http*)td->lhttp)->raiseHTTPError(td,s,e_404);

	File::splitPath(filename,pathname,execname);
	
	getdefaultwd(dataFilePath,MAX_PATH);
	GetShortPathName(dataFilePath,dataFilePath,MAX_PATH);
	sprintf(&dataFilePath[strlen(dataFilePath)],"/data_%u.ini",td->id);
	
	strcpy(outFilePath,td->outputDataPath.c_str());
	strcat(outFilePath,"WC");
	td->inputData.setFilePointer(0);

  chain.setProtocol((Http*)td->lhttp);
  chain.setProtocolData(td);
  chain.setStream(&(td->connection->socket));
  if(td->mime)
  {
    u_long nbw2;
    if(td->mime && lserver->getFiltersFactory()->chain(&chain, 
                                                 ((MimeManager::MimeRecord*)td->mime)->filters, 
                                                       &(td->connection->socket) , &nbw2, 1))
      {
        ((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
        ((Vhost*)td->connection->host)->warningsLogWrite("Error loading filters\r\n");
        ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);

        chain.clearAllFilters(); 
        return ((Http*)td->lhttp)->raiseHTTPError(td, s, e_500);
      }
  }
	

	/*!
   *The WinCGI protocol uses a .ini file to send data to the new process.
   */
	ret=DataFileHandle.openFile(dataFilePath,
                     FILE_CREATE_ALWAYS|FILE_OPEN_WRITE);
	if ( ret ) 
	{
		((Vhost*)td->connection->host)->warningslogRequestAccess(td->id);
		((Vhost*)td->connection->host)->warningsLogWrite("WinCGI: Error creating ini\r\n");
		((Vhost*)td->connection->host)->warningslogTerminateAccess(td->id);
		return ((Http*)td->lhttp)->raiseHTTPError(td,s,e_500);
	}

	td->buffer2->setLength(0);
	buffer=td->buffer2->getBuffer();

	strcpy(buffer,"[CGI]\r\n");
	DataFileHandle.writeToFile(buffer,7,&nbr);

	strcpy(buffer,"CGI Version=CGI/1.3a WIN\r\n");
	DataFileHandle.writeToFile(buffer,26,&nbr);

	*td->buffer2 << "Server Admin=" << lserver->getServerAdmin() << "\r\n";
	DataFileHandle.writeToFile(buffer,td->buffer2->getLength(),&nbr);

	if(stringcmpi(td->request.connection,"Keep-Alive"))
	{
		strcpy(buffer,"Request Keep-Alive=No\r\n");
		DataFileHandle.writeToFile(buffer,23,&nbr);
	}
	else
	{
		strcpy(buffer,"Request Keep-Alive=Yes\r\n");
		DataFileHandle.writeToFile(buffer,24,&nbr);
	}

	td->buffer2->setLength(0);
	*td->buffer2 << "Request Method=" << td->request.cmd << "\r\n";
	DataFileHandle.writeToFile(buffer,td->buffer2->getLength(),&nbr);

	td->buffer2->setLength(0);
	*td->buffer2 << "Request Protocol=HTTP/" << td->request.ver << "\r\n";
	DataFileHandle.writeToFile(buffer,td->buffer2->getLength(),&nbr);

	 td->buffer2->setLength(0);	
	*td->buffer2 << "Executable Path=" << execname << "\r\n";
	DataFileHandle.writeToFile(buffer,td->buffer2->getLength(),&nbr);

	if(td->request.uriOpts[0])
	{
		sprintf(buffer,"Query String=%s\r\n",td->request.uriOpts.c_str());
		DataFileHandle.writeToFile(buffer,(u_long)strlen(buffer),&nbr);
	}
	if(td->request.referer[0])
	{
		sprintf(buffer,"Referer=%s\r\n",td->request.referer.c_str());
		DataFileHandle.writeToFile(buffer,(u_long)strlen(buffer),&nbr);
	}
	if(td->request.contentType[0])
	{
		sprintf(buffer,"Content Type=%s\r\n",td->request.contentType.c_str());
		DataFileHandle.writeToFile(buffer,(u_long)strlen(buffer),&nbr);
	}

	if(td->request.userAgent[0])
	{
		sprintf(buffer,"User Agent=%s\r\n",td->request.userAgent.c_str());
		DataFileHandle.writeToFile(buffer,(u_long)strlen(buffer),&nbr);
	}

	sprintf(buffer,"Content File=%s\r\n",td->inputData.getFilename());
	DataFileHandle.writeToFile(buffer,(u_long)strlen(buffer),&nbr);

	if(td->request.contentLength[0])
	{
		sprintf(buffer,"Content Length=%s\r\n",td->request.contentLength.c_str());
		DataFileHandle.writeToFile(buffer,(u_long)strlen(buffer),&nbr);
	}
	else
	{
		strcpy(buffer,"Content Length=0\r\n");
		DataFileHandle.writeToFile(buffer,18,&nbr);	
	}

	strcpy(buffer,"Server Software=MyServer\r\n");
	DataFileHandle.writeToFile(buffer,26,&nbr);

	sprintf(buffer,"Remote Address=%s\r\n",td->connection->getIpAddr());
	DataFileHandle.writeToFile(buffer,(u_long)strlen(buffer),&nbr);

	sprintf(buffer,"Server Port=%u\r\n",td->connection->getLocalPort());
	DataFileHandle.writeToFile(buffer,(u_long)strlen(buffer),&nbr);

	sprintf(buffer,"Server Name=%s\r\n",td->request.host.c_str());
	DataFileHandle.writeToFile(buffer,(u_long)strlen(buffer),&nbr);

	strcpy(buffer,"[System]\r\n");
	DataFileHandle.writeToFile(buffer,10,&nbr);

	sprintf(buffer,"Output File=%s\r\n",outFilePath);
	DataFileHandle.writeToFile(buffer,(u_long)strlen(buffer),&nbr);

	sprintf(buffer,"Content File=%s\r\n",td->inputData.getFilename());
	DataFileHandle.writeToFile(buffer,(u_long)strlen(buffer),&nbr);

	/*!
   *Compute the local offset from the GMT time
   */
	ltime=100;
	gmhour=gmtime( &ltime)->tm_hour;
	bias=localtime(&ltime)->tm_hour-gmhour;

	sprintf(buffer,"GMT Offset=%i\r\n",bias);
	DataFileHandle.writeToFile(buffer,(u_long)strlen(buffer),&nbr);

	sprintf(buffer,"Debug Mode=No\r\n",bias);
	DataFileHandle.writeToFile(buffer,15,&nbr);

	DataFileHandle.closeFile();

	/*!
   *Create the out file.
   */
	if(!File::fileExists(outFilePath))
	{
		ret = OutFileHandle.openFile(outFilePath,FILE_CREATE_ALWAYS);
		if (ret)
		{
			((Vhost*)td->connection->host)->warningslogRequestAccess(td->id);
			((Vhost*)td->connection->host)->warningsLogWrite(
                                      "WinCGI: Error creating output file\r\n");
			((Vhost*)td->connection->host)->warningslogTerminateAccess(td->id);
			DataFileHandle.closeFile();
			File::deleteFile(outFilePath);
			File::deleteFile(dataFilePath);
      chain.clearAllFilters(); 
			return ((Http*)td->lhttp)->raiseHTTPError(td,s,e_500);
		}
	}
	OutFileHandle.closeFile();
	spi.cmdLine.assign("cmd /c \"");
	spi.cmdLine.append(filename);
	spi.cmdLine.append("\" ");
	spi.cmdLine.append(dataFilePath);
	spi.cwd.assign(pathname);

	if (proc.execHiddenProcess(&spi, timeout))
	{
    ostringstream msg;
    msg << "WinCGI: Error executing process " << filename << "\r\n";
		((Vhost*)td->connection->host)->warningslogRequestAccess(td->id);
		((Vhost*)td->connection->host)->warningsLogWrite(msg.str().c_str());
		((Vhost*)td->connection->host)->warningslogTerminateAccess(td->id);
		File::deleteFile(outFilePath);
		File::deleteFile(dataFilePath);
    chain.clearAllFilters(); 
		return ((Http*)td->lhttp)->raiseHTTPError(td,s,e_500);
	}

	ret=OutFileHandle.openFile(outFilePath,FILE_OPEN_ALWAYS|
                             FILE_OPEN_READ);
	if (ret)
	{
    ostringstream msg;
    msg << "WinCGI: Error opening output file " << outFilePath << "\r\n";
		((Vhost*)td->connection->host)->warningslogRequestAccess(td->id);
		((Vhost*)td->connection->host)->warningsLogWrite(msg.str().c_str());
		((Vhost*)td->connection->host)->warningslogTerminateAccess(td->id);
    chain.clearAllFilters();
		return ((Http*)td->lhttp)->raiseHTTPError(td,s,e_500);
	}
	OutFileHandle.readFromFile(buffer,td->buffer2->getRealLength(),&nBytesRead);
	if(nBytesRead==0)
	{
    ostringstream msg;
    msg << "WinCGI: Error zero bytes read from the WinCGI output file " 
        << outFilePath << "\r\n";
		((Vhost*)td->connection->host)->warningslogRequestAccess(td->id);
		((Vhost*)td->connection->host)->warningsLogWrite(msg.str().c_str());
		((Vhost*)td->connection->host)->warningslogTerminateAccess(td->id);
		OutFileHandle.closeFile();
		File::deleteFile(outFilePath);
		File::deleteFile(dataFilePath);
    chain.clearAllFilters();
		return ((Http*)td->lhttp)->raiseHTTPError(td,s,e_500);
	}

	for(u_long i=0;i<nBytesRead;i++)
	{
		if((buffer[i]=='\r')&&(buffer[i+1]=='\n')
       &&(buffer[i+2]=='\r')&&(buffer[i+3]=='\n'))
		{
			/*!
       *The HTTP header ends with a \r\n\r\n sequence so
       *determinate where it ends and set the header size
       *to i + 4.
       */
			headerSize=i+4;
			break;
		}
	}
	if(!stringcmpi(td->request.connection,"Keep-Alive"))
		td->response.connection.assign("Keep-Alive");
	HttpHeaders::buildHTTPResponseHeaderStruct(&td->response,td,buffer);
	/*!
   *Always specify the size of the HTTP contents.
   */
  stream << OutFileHandle.getFileSize()-headerSize;
	td->response.contentLength.assign(stream.str());
	if(!td->appendOutputs)
	{
    u_long nbw2;
    /*!
     *Send the header if it is not appending.
     */
		HttpHeaders::buildHTTPResponseHeader(td->buffer->getBuffer(),
                                          &td->response);
		s->socket.send((const char*)td->buffer->getBuffer(),
                   (int)strlen((const char*)td->buffer->getBuffer()), 0);
    if(onlyHeader)
    {
      OutFileHandle.closeFile();
      File::deleteFile(outFilePath);
      File::deleteFile(dataFilePath);
      chain.clearAllFilters();
      return 1;
    }
    /*!
     *Send other data in the buffer.
     */
		chain.write((char*)(buffer+headerSize),nBytesRead-headerSize, &nbw2);
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
    
		td->outputData.writeToFile((char*)(buffer+headerSize),
                               nBytesRead-headerSize,&nbw2);
    nbw +=    nbw2;
  }

  /*! Flush the rest of the file. */
  do
	{
    OutFileHandle.readFromFile(buffer,td->buffer2->getLength(),&nBytesRead);
		if(nBytesRead)
		{
      int ret;
			if(td->appendOutputs)
      {
				ret = td->outputData.writeToFile(buffer,nBytesRead,&nbw);
        if(ret)
        {
          OutFileHandle.closeFile();
          File::deleteFile(outFilePath);
          File::deleteFile(dataFilePath);
          chain.clearAllFilters();
          return 0;
        }
      }			
      else
      {
        u_long nbw2;
				ret = chain.write((char*)buffer,nBytesRead, &nbw2);
        if(ret == -1)
        {
          OutFileHandle.closeFile();
          File::deleteFile(outFilePath);
          File::deleteFile(dataFilePath);
          chain.clearAllFilters();
          return 0;
        }
      }
		}
		else
			break;

	}while(nBytesRead);

  {
    ostringstream buffer;
    buffer << nbw;
    td->response.contentLength.assign(buffer.str());
  }

  chain.clearAllFilters();	
	OutFileHandle.closeFile();
	File::deleteFile(outFilePath);
	File::deleteFile(dataFilePath);
	return 1;
#endif

#ifdef NOT_WIN
  /*! WinCGI is available only under windows. */
	td->buffer->setLength(0);
	*td->buffer << "Error WinCGI is not implemented on the current architecture\r\n" << '\0';
	((Vhost*)td->connection->host)->warningslogRequestAccess(td->id);
	((Vhost*)td->connection->host)->warningsLogWrite(td->buffer->getBuffer());
	((Vhost*)td->connection->host)->warningslogTerminateAccess(td->id);
	return ((Http*)td->lhttp)->raiseHTTPError(td,s,e_501);
#endif
}
