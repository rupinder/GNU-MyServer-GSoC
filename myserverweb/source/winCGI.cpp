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
extern "C" 
{
#ifdef WIN32
#include <direct.h>
#endif
#include <string.h>
}

/*!
 *Initialize the timeout value to 15 seconds.
 */
u_long wincgi::timeout=SEC(15);

/*!
 *Set a new timeout value to use for new processes.
 */
void wincgi::setTimeout(u_long ntimeout)
{
  timeout = ntimeout;
}

/*!
 *Constructor for the wincgi class.
 */
wincgi::wincgi()
{

}

/*!
 *Destructor for the wincgi class.
 */
wincgi::~wincgi()
{

}

/*!
 *Get the timeout value for the new process. 
 */
u_long wincgi::getTimeout()
{
  return timeout;
}

/*!
 *Send the WinCGI data.
 */
int wincgi::send(httpThreadContext* td,LPCONNECTION s,char* filename, 
                 int /*execute*/, int only_header)
{
#ifdef WIN32
	u_long nbr;
	char cmdLine[MAX_PATH + 120];/*! Use MAX_PATH under windows. */
	char  dataFilePath[MAX_PATH];/*! Use MAX_PATH under windows. */
  char outFilePath[MAX_PATH];  /*! Use MAX_PATH under windows. */
  char *buffer;
	MYSERVER_FILE DataFileHandle, OutFileHandle;
	time_t ltime=100;
	int gmhour;
	int bias;

  int ret;
	char execname[MAX_PATH];/*! Use MAX_PATH under windows. */
	char pathname[MAX_PATH];/*! Use MAX_PATH under windows. */

	if(!MYSERVER_FILE::fileExists(filename))
		return ((http*)td->lhttp)->raiseHTTPError(td,s,e_404);

	MYSERVER_FILE::splitPath(filename,pathname,execname);
	
	getdefaultwd(dataFilePath,MAX_PATH);
	GetShortPathName(dataFilePath,dataFilePath,MAX_PATH);
	sprintf(&dataFilePath[strlen(dataFilePath)],"/data_%u.ini",td->id);
	
	strcpy(outFilePath,td->outputDataPath);
	strcat(outFilePath,"WC");
	td->inputData.setFilePointer(0);

	/*!
   *The WinCGI protocol uses a .ini file to send data to the new process.
   */
	ret=DataFileHandle.openFile(dataFilePath,
                     MYSERVER_FILE_CREATE_ALWAYS|MYSERVER_FILE_OPEN_WRITE);
	if ( ret ) 
	{
		((vhost*)td->connection->host)->warningslogRequestAccess(td->id);
		((vhost*)td->connection->host)->warningsLogWrite("Error creating WinCGI ini\r\n");
		((vhost*)td->connection->host)->warningslogTerminateAccess(td->id);
		return ((http*)td->lhttp)->raiseHTTPError(td,s,e_500);
	}
	td->buffer2->SetLength(0);
	buffer=(char*)td->buffer2->GetBuffer();

	strcpy(buffer,"[CGI]\r\n");
	DataFileHandle.writeToFile(buffer,7,&nbr);

	strcpy(buffer,"CGI Version=CGI/1.3a WIN\r\n");
	DataFileHandle.writeToFile(buffer,26,&nbr);

	*td->buffer2 << "Server Admin=" << lserver->getServerAdmin() << "\r\n";
	DataFileHandle.writeToFile(buffer,td->buffer2->GetLength(),&nbr);

	if(lstrcmpi(td->request.CONNECTION,"Keep-Alive"))
	{
		strcpy(buffer,"Request Keep-Alive=No\r\n");
		DataFileHandle.writeToFile(buffer,23,&nbr);
	}
	else
	{
		strcpy(buffer,"Request Keep-Alive=Yes\r\n");
		DataFileHandle.writeToFile(buffer,24,&nbr);
	}

	td->buffer2->SetLength(0);
	*td->buffer2 << "Request Method=" << td->request.CMD << "\r\n";
	DataFileHandle.writeToFile(buffer,td->buffer2->GetLength(),&nbr);

	td->buffer2->SetLength(0);
	*td->buffer2 << "Request Protocol=HTTP/" << td->request.VER << "\r\n";
	DataFileHandle.writeToFile(buffer,td->buffer2->GetLength(),&nbr);

	 td->buffer2->SetLength(0);	
	*td->buffer2 << "Executable Path=" << execname << "\r\n";
	DataFileHandle.writeToFile(buffer,td->buffer2->GetLength(),&nbr);

	if(td->request.URIOPTS[0])
	{
		sprintf(buffer,"Query String=%s\r\n",td->request.URIOPTS);
		DataFileHandle.writeToFile(buffer,(u_long)strlen(buffer),&nbr);
	}
	if(td->request.REFERER[0])
	{
		sprintf(buffer,"Referer=%s\r\n",td->request.REFERER);
		DataFileHandle.writeToFile(buffer,(u_long)strlen(buffer),&nbr);
	}
	if(td->request.CONTENT_TYPE[0])
	{
		sprintf(buffer,"Content Type=%s\r\n",td->request.CONTENT_TYPE);
		DataFileHandle.writeToFile(buffer,(u_long)strlen(buffer),&nbr);
	}

	if(td->request.USER_AGENT[0])
	{
		sprintf(buffer,"User Agent=%s\r\n",td->request.USER_AGENT);
		DataFileHandle.writeToFile(buffer,(u_long)strlen(buffer),&nbr);
	}

	sprintf(buffer,"Content File=%s\r\n",td->inputData.getFilename());
	DataFileHandle.writeToFile(buffer,(u_long)strlen(buffer),&nbr);

	if(td->request.CONTENT_LENGTH[0])
	{
		sprintf(buffer,"Content Length=%s\r\n",td->request.CONTENT_LENGTH);
		DataFileHandle.writeToFile(buffer,(u_long)strlen(buffer),&nbr);
	}
	else
	{
		strcpy(buffer,"Content Length=0\r\n");
		DataFileHandle.writeToFile(buffer,18,&nbr);	
	}

	strcpy(buffer,"Server Software=MyServer\r\n");
	DataFileHandle.writeToFile(buffer,26,&nbr);

	sprintf(buffer,"Remote Address=%s\r\n",td->connection->getipAddr());
	DataFileHandle.writeToFile(buffer,(u_long)strlen(buffer),&nbr);

	sprintf(buffer,"Server Port=%u\r\n",td->connection->getLocalPort());
	DataFileHandle.writeToFile(buffer,(u_long)strlen(buffer),&nbr);

	sprintf(buffer,"Server Name=%s\r\n",td->request.HOST);
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
	if(!MYSERVER_FILE::fileExists(outFilePath))
	{
		ret = OutFileHandle.openFile(outFilePath,MYSERVER_FILE_CREATE_ALWAYS);
		if (ret)
		{
			((vhost*)td->connection->host)->warningslogRequestAccess(td->id);
			((vhost*)td->connection->host)->warningsLogWrite(
                                      "Error creating WinCGI output file\r\n");
			((vhost*)td->connection->host)->warningslogTerminateAccess(td->id);
			DataFileHandle.closeFile();
			MYSERVER_FILE::deleteFile(outFilePath);
			MYSERVER_FILE::deleteFile(dataFilePath);
			return ((http*)td->lhttp)->raiseHTTPError(td,s,e_500);
		}
	}
	OutFileHandle.closeFile();
	strcpy(cmdLine,"cmd /c \"");
	strcat(cmdLine, filename);
	strcat(cmdLine, "\" ");
	strcat(cmdLine, dataFilePath);
	START_PROC_INFO spi;
	memset(&spi,0,sizeof(spi));
	spi.cwd = pathname;
	spi.cmdLine = cmdLine;

	if (execHiddenProcess(&spi, timeout))
	{
		((vhost*)td->connection->host)->warningslogRequestAccess(td->id);
		((vhost*)td->connection->host)->warningsLogWrite(
                            "Error executing WinCGI process\r\n");
		((vhost*)td->connection->host)->warningslogTerminateAccess(td->id);
		MYSERVER_FILE::deleteFile(outFilePath);
		MYSERVER_FILE::deleteFile(dataFilePath);
		return ((http*)td->lhttp)->raiseHTTPError(td,s,e_500);
	}

	ret=OutFileHandle.openFile(outFilePath,MYSERVER_FILE_OPEN_ALWAYS|
                             MYSERVER_FILE_OPEN_READ);
	if (ret)
	{
		((vhost*)td->connection->host)->warningslogRequestAccess(td->id);
		((vhost*)td->connection->host)->warningsLogWrite(
                                       "Error opening WinCGI output file\r\n");
		((vhost*)td->connection->host)->warningslogTerminateAccess(td->id);
		return ((http*)td->lhttp)->raiseHTTPError(td,s,e_500);
	}
	u_long nBytesRead=0;
	OutFileHandle.readFromFile(buffer,td->buffer2->GetRealLength(),&nBytesRead);
	if(nBytesRead==0)
	{
		((vhost*)td->connection->host)->warningslogRequestAccess(td->id);
		((vhost*)td->connection->host)->warningsLogWrite(
                    "Error zero bytes read from the WinCGI output file\r\n");
		((vhost*)td->connection->host)->warningslogTerminateAccess(td->id);
		OutFileHandle.closeFile();
		MYSERVER_FILE::deleteFile(outFilePath);
		MYSERVER_FILE::deleteFile(dataFilePath);
		return ((http*)td->lhttp)->raiseHTTPError(td,s,e_500);
	}
	u_long headerSize=0;

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
	if(!lstrcmpi(td->request.CONNECTION,"Keep-Alive"))
		strcpy(td->response.CONNECTION,"Keep-Alive");
	http_headers::buildHTTPResponseHeaderStruct(&td->response,td,buffer);
	/*!
   *Always specify the size of the HTTP contents.
   */
	sprintf(td->response.CONTENT_LENGTH,"%u",
          OutFileHandle.getFileSize()-headerSize);
	u_long nbw=0;
	if(!td->appendOutputs)
	{
    /*!
     *Send the header if it is not appending.
     */
		http_headers::buildHTTPResponseHeader((char*)td->buffer->GetBuffer(),
                                          &td->response);
		s->socket.send((const char*)td->buffer->GetBuffer(),
                   (int)strlen((const char*)td->buffer->GetBuffer()), 0);
    if(only_header)
    {
      OutFileHandle.closeFile();
      MYSERVER_FILE::deleteFile(outFilePath);
      MYSERVER_FILE::deleteFile(dataFilePath);
      return 1;
    }
    /*!
     *Send other data in the buffer.
     */
		s->socket.send((char*)(buffer+headerSize),nBytesRead-headerSize, 0);
	}
	else
  {
		http_headers::buildHTTPResponseHeader((char*)td->buffer->GetBuffer(),
                                          &td->response);
    if(only_header)
    {
      return 1;
    }
		td->outputData.writeToFile((char*)(buffer+headerSize),
                               nBytesRead-headerSize,&nbw);
  }

  /*! Flush the rest of the file. */
  do
	{
    OutFileHandle.readFromFile(buffer,td->buffer2->GetLength(),&nBytesRead);
		if(nBytesRead)
		{
      int ret;
			if(td->appendOutputs)
      {
				ret = td->outputData.writeToFile(buffer,nBytesRead,&nbw);
        if(ret)
        {
          OutFileHandle.closeFile();
          MYSERVER_FILE::deleteFile(outFilePath);
          MYSERVER_FILE::deleteFile(dataFilePath);
          return 0;
        }
      }			
      else
      {
				ret = s->socket.send((char*)buffer,nBytesRead, 0);
        if(ret == -1)
        {
          OutFileHandle.closeFile();
          MYSERVER_FILE::deleteFile(outFilePath);
          MYSERVER_FILE::deleteFile(dataFilePath);
          return 0;
        }
      }
		}
		else
			break;

	}while(nBytesRead);
	
	OutFileHandle.closeFile();
	MYSERVER_FILE::deleteFile(outFilePath);
	MYSERVER_FILE::deleteFile(dataFilePath);
	return 1;
#endif

#ifdef NOT_WIN
  /*! WinCGI is available only under windows. */
	td->buffer->SetLength(0);
	*td->buffer << "Error WinCGI is not implemented\r\n" << '\0';
	((vhost*)td->connection->host)->warningslogRequestAccess(td->id);
	((vhost*)td->connection->host)->warningsLogWrite((char*)td->buffer->GetBuffer());
	((vhost*)td->connection->host)->warningslogTerminateAccess(td->id);
	return ((http*)td->lhttp)->raiseHTTPError(td,s,e_501);
#endif
}
