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
 
/*!
 *To get more info about the FastCGI protocol please visit the official FastCGI site
 *at: http://www.fastcgi.com.
 *On that site you can find samples and all the languages supported.
 */
#include "../include/fastCGI.h"
#include "../include/cgi.h"
#include "../include/http.h"
#include "../include/HTTPmsg.h"
struct sfCGIservers *FastCgi::fCGIservers = 0;

/*! Number of thread currently loaded. */
int FastCgi::fCGIserversN=0;

/*! Is the fastcgi initialized? */
int FastCgi::initialized=0;

/*! By default allows 25 servers. */
int FastCgi::max_fcgi_servers=25;

/*! Use a default timeout of 15 seconds. */
int FastCgi::timeout=MYSERVER_SEC(15);

/*! Mutex used to access fastCGI servers. */
myserver_mutex FastCgi::servers_mutex;

struct fourchar
{	
	union
	{
		unsigned int i;
		unsigned char c[4];
	};
};

/*!
 *Set a new value for the max number of servers that can be executed.
 */
void FastCgi::setMaxFcgiServers(int max)
{
  max_fcgi_servers = max;
}

/*!
 *Get the max number of servers that can be executed.
 */
int FastCgi::getMaxFcgiServers()
{
  return max_fcgi_servers;
}

/*!
 *Entry-Point to manage a FastCGI request.
 */
int FastCgi::send(HttpThreadContext* td, ConnectionPtr connection,
                  char* scriptpath,char *cgipath,int execute, int only_header)
{
	fCGIContext con;
	con.td=td;
	u_long nbr=0;
	FCGI_Header header;
  
  int scriptDirLen = 0;
  int scriptFileLen = 0;
  int cgiRootLen = 0;
  int cgiFileLen = 0;
  int scriptpathLen = strlen(scriptpath) + 1;

	int exit;
  int ret;
	
	clock_t time1;
	
	char *outDataPath;
  int outDataPathLen;

  int sizeEnvString;
  sfCGIservers* server;
  int id;
	char *fullpath;

  if(td->scriptPath)
    delete [] td->scriptPath;
  td->scriptPath = new char[scriptpathLen];
  if(td->scriptPath == 0)
  {
    return ((Http*)td->lhttp)->sendHTTPhardError500(td, connection);
  }
	lstrcpy(td->scriptPath, scriptpath);

  File::splitPathLength(scriptpath, &scriptDirLen, &scriptFileLen);
  if(cgipath)
	File::splitPathLength(cgipath, &cgiRootLen, &cgiFileLen);

  if(td->scriptDir)
    delete [] td->scriptDir;
  td->scriptDir = new char[scriptDirLen+1];
  if(td->scriptDir == 0)
  {
    return ((Http*)td->lhttp)->sendHTTPhardError500(td, connection);
  }

  if(td->scriptFile)
    delete [] td->scriptFile;
  td->scriptFile = new char[scriptFileLen+1];
  if(td->scriptFile == 0)
  {
    return ((Http*)td->lhttp)->sendHTTPhardError500(td, connection);
  }

  if(td->cgiRoot)
    delete [] td->cgiRoot;
  td->cgiRoot = new char[cgiRootLen+1];
  if(td->cgiRoot == 0)
  {
    return ((Http*)td->lhttp)->sendHTTPhardError500(td, connection); 
  }

  if(td->cgiFile)
    delete [] td->cgiFile;
  td->cgiFile = new char[cgiFileLen+1];
  if(td->cgiFile == 0)
  {
    return ((Http*)td->lhttp)->sendHTTPhardError500(td, connection); 
  }
	td->cgiRoot[0] = td->cgiFile[0] = '\0';
	File::splitPath(scriptpath, td->scriptDir, td->scriptFile);
	File::splitPath(cgipath, td->cgiRoot, td->cgiFile);


  td->buffer->SetLength(0);
	td->buffer2->GetAt(0)='\0';
	if(execute)
	{
		if(cgipath)
    {
      int fullpathLen = strlen(cgipath) + strlen(td->filenamePath) + 4;
      fullpath = new char[fullpathLen];
      if(fullpath == 0)
      {
        return ((Http*)td->lhttp)->sendHTTPhardError500(td, connection); 
      }
			sprintf(fullpath,"%s \"%s\"", cgipath, td->filenamePath);
    }
		else
    {
      int fullpathLen = strlen(td->filenamePath) + 1;
      fullpath = new char[fullpathLen];
      if(fullpath == 0)
      {
        return ((Http*)td->lhttp)->sendHTTPhardError500(td, connection);
      }
			sprintf(fullpath,"%s",td->filenamePath);	
    }
	}
	else
	{
    int fullpathLen = strlen(cgipath) + 1;
    fullpath = new char[fullpathLen];
    if(fullpath == 0)
    {
      return ((Http*)td->lhttp)->sendHTTPhardError500(td, connection);
    }
		sprintf(fullpath,"%s",cgipath);
	}
  Cgi::buildCGIEnvironmentString(td,(char*)td->buffer->GetBuffer());
  sizeEnvString=buildFASTCGIEnvironmentString(td,(char*)td->buffer->GetBuffer(),
                                              (char*)td->buffer2->GetBuffer());
  if(sizeEnvString == -1)
  {
    delete [] fullpath;
		td->buffer->SetLength(0);
		*td->buffer<< "Error FastCGI to build env string\r\n" << '\0';
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)td->connection->host)->warningsLogWrite((char*)td->buffer->GetBuffer());
		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
  }
	td->inputData.closeFile();
	if(td->inputData.openFile(td->inputDataPath,
                         File::OPEN_READ | File::OPEN_ALWAYS | 
                            File::NO_INHERIT))
  {
    delete [] fullpath;
		td->buffer->SetLength(0);
		*td->buffer<< "Error opening stdin file\r\n" << '\0';
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)td->connection->host)->warningsLogWrite((char*)td->buffer->GetBuffer());
		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
  }

  server = FcgiConnect(&con,fullpath);
  delete [] fullpath;
	if(server == 0)
  {
    td->inputData.closeFile();
		File::deleteFile(td->inputDataPath);
		td->buffer->SetLength(0);
		*td->buffer<< "Error FastCGI to get process ID\r\n" << '\0';
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)td->connection->host)->warningsLogWrite((char*)td->buffer->GetBuffer());
		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
  }

	id=td->id+1;
	FCGI_BeginRequestBody tBody;
	tBody.roleB1 = ( FCGI_RESPONDER >> 8 ) & 0xff;
	tBody.roleB0 = ( FCGI_RESPONDER ) & 0xff;
	tBody.flags = 0;
	memset( tBody.reserved, 0, sizeof( tBody.reserved ) );

	if(sendFcgiBody(&con,(char*)&tBody,sizeof(tBody),FCGI_BEGIN_REQUEST,id))
	{
    td->inputData.closeFile();
		File::deleteFile(td->inputDataPath);
		td->buffer->SetLength(0);
		*td->buffer<< "Error FastCGI to begin the request\r\n" << '\0';
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)td->connection->host)->warningsLogWrite((char*)td->buffer->GetBuffer());
		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		con.sock.closesocket();
		return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_501);
	}

	if(sendFcgiBody(&con,(char*)td->buffer2->GetBuffer(),sizeEnvString,
                  FCGI_PARAMS,id))
	{
    td->inputData.closeFile();
		File::deleteFile(td->inputDataPath);
		td->buffer->SetLength(0);
		*td->buffer << "Error FastCGI to send params\r\n" << '\0';
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)td->connection->host)->warningsLogWrite((char*)td->buffer->GetBuffer());
		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		con.sock.closesocket();
		return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_501);
	}

	if(sendFcgiBody(&con,0,0,FCGI_PARAMS,id))
	{
    td->inputData.closeFile();
		File::deleteFile(td->inputDataPath);
		td->buffer->SetLength(0);
		*td->buffer << "Error FastCGI to send params\r\n" << '\0';
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)td->connection->host)->warningsLogWrite((char*)td->buffer->GetBuffer());
		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		con.sock.closesocket();
		return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
	}	
	if(atoi(td->request.CONTENT_LENGTH))
	{
		td->buffer->SetLength(0);
		*td->buffer << "Error FastCGI to send POST data\r\n"<< '\0';
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)td->connection->host)->warningsLogWrite((char*)td->buffer->GetBuffer());
		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		generateFcgiHeader( header, FCGI_STDIN, id, atoi(td->request.CONTENT_LENGTH));
		if(con.sock.send((char*)&header,sizeof(header),0)==-1)
    {
      td->inputData.closeFile();
      File::deleteFile(td->inputDataPath);
			return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_501);
    }
		td->inputData.setFilePointer(0);
		do
		{
			if(td->inputData.readFromFile((char*)td->buffer->GetBuffer(),
                                    td->buffer->GetRealLength(),&nbr))
      {
        td->inputData.closeFile();
        File::deleteFile(td->inputDataPath);
        td->buffer->SetLength(0);
        *td->buffer << "Error FastCGI to read from file\r\n" << '\0';
        ((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
        ((Vhost*)td->connection->host)->warningsLogWrite(
                                                    (char*)td->buffer->GetBuffer());
        ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
        return ((Http*)td->lhttp)->sendHTTPhardError500(td, connection);
      }      
      
			if(nbr)
			{
				if(con.sock.send((char*)td->buffer->GetBuffer(),nbr,0) == -1)
        {
          td->inputData.closeFile();
          File::deleteFile(td->inputDataPath);
          td->buffer->SetLength(0);
          *td->buffer << "Error FastCGI to send data\r\n" << '\0';
          ((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
          ((Vhost*)td->connection->host)->warningsLogWrite(
                                                  (char*)td->buffer->GetBuffer());
          ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
					return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
        }
			}
		}while(nbr==td->buffer->GetRealLength());
	}
	if(sendFcgiBody(&con,0,0,FCGI_STDIN,id))
	{
    td->inputData.closeFile();
		File::deleteFile(td->inputDataPath);
		td->buffer->SetLength(0);
		*td->buffer << "Error FastCGI to send POST data\r\n"<< '\0';
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)td->connection->host)->
                            warningsLogWrite((char*)td->buffer->GetBuffer());

		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		con.sock.closesocket();
    return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
	}	

	/*! Now read the output. This flag is used by the external loop. */
	exit=0;

  /*! Return 1 if keep the connection. */
  ret = 1;
	
	time1 = get_ticks();
	
	outDataPath=0;
  outDataPathLen = getdefaultwdlen() + 24;
	outDataPath = new char[outDataPathLen];
  if(outDataPath == 0)
  {
    td->inputData.closeFile();
		File::deleteFile(td->inputDataPath);
    td->buffer->SetLength(0);
		*td->buffer << "Error allocating memory\r\n"<< '\0';
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)td->connection->host)->
                            warningsLogWrite((char*)td->buffer->GetBuffer());

		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
    return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
  }
	getdefaultwd(outDataPath, outDataPathLen);
	sprintf(&(outDataPath)[strlen(outDataPath)],"/stdOutFileFCGI_%u",(u_int)td->id);
	
	if(con.tempOut.openFile(outDataPath,File::OPEN_WRITE | 
                          File::OPEN_READ | File::CREATE_ALWAYS |
                          File::NO_INHERIT))
  {
    td->inputData.closeFile();
		File::deleteFile(td->inputDataPath);
    delete [] outDataPath;
    td->buffer->SetLength(0);
		*td->buffer << "Error opening stdout file\r\n"<< '\0';
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)td->connection->host)->
                            warningsLogWrite((char*)td->buffer->GetBuffer());

		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
    return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
  }

	do	
	{
		while(con.sock.bytesToRead()<sizeof(FCGI_Header))
		{
			if((clock_t)(get_ticks()-time1) > timeout)
				break;
		}
		if(con.sock.bytesToRead())
    {
			nbr=con.sock.recv((char*)&header,sizeof(FCGI_Header),0);
      if(nbr == (u_long)-1)
      {
        td->buffer->SetLength(0);
        *td->buffer << "Error FastCGI reading data\r\n"<< '\0';
        ((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
        ((Vhost*)td->connection->host)->warningsLogWrite(
                                             (char*)td->buffer->GetBuffer());
        ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
        sendFcgiBody(&con,0,0,FCGI_ABORT_REQUEST,id);
        con.sock.shutdown(2);
        con.sock.closesocket();
        break;
      }
    }
		else
		{
			td->buffer->SetLength(0);
			*td->buffer << "Error FastCGI timeout\r\n"<< '\0';
			((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
			((Vhost*)td->connection->host)->warningsLogWrite((char*)td->buffer->GetBuffer());
			((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
			sendFcgiBody(&con,0,0,FCGI_ABORT_REQUEST,id);
			con.sock.shutdown(2);
			con.sock.closesocket();
			break;
		}
		/*!
     *contentLengthB1 is the high word of the content length value
     *while contentLengthB0 is the low one.
     *To retrieve the value of content length push left contentLengthB1
     *of eight byte then do a or with contentLengthB0.
     */
		u_long dim=(header.contentLengthB1<<8) | header.contentLengthB0;
		u_long data_sent=0;
		if(dim==0)
		{
      exit = 1;
      ret = 1;
		}
		else
		{
			switch(header.type)
			{
				case FCGI_STDERR:
					con.sock.closesocket();
					((Http*)td->lhttp)->raiseHTTPError(td,connection,e_501);
					exit = 1;
          ret = 0;
					break;
				case FCGI_STDOUT:
					nbr=con.sock.recv((char*)td->buffer->GetBuffer(),
                            min(dim,td->buffer->GetRealLength()),0);
					u_long nbw;
					con.tempOut.writeToFile((char*)td->buffer->GetBuffer(),nbr,&nbw);
					data_sent=nbw;
					if(data_sent==0)
					{
						exit = 1;
            ret = 0;
						break;
					}
					while(data_sent<dim)
					{
						if( con.sock.bytesToRead() )
            {
							nbr=con.sock.recv((char*)td->buffer->GetBuffer(), 
                                min(dim-data_sent,td->buffer->GetRealLength()),0);
            }
						else
						{
              ret = 0;
							exit = 1;
							break;
						}
						if(con.tempOut.writeToFile((char*)(td->buffer->GetBuffer()),nbr,&nbw))
            {
              ret = 0;
              exit = 1;
              break;
            }
						data_sent+=nbw;
					}
					break;
				case FCGI_END_REQUEST:
					exit = 1;
					break;			
				case FCGI_GET_VALUES_RESULT:
				case FCGI_UNKNOWN_TYPE:
				default:
					break;
			}
		}
	}while((!exit) && nbr);
	u_long headerSize=0;
	con.tempOut.setFilePointer(0);
	td->buffer->GetAt(0)='\0';
	char *buffer=(char*)td->buffer->GetBuffer();
	con.tempOut.readFromFile(buffer,td->buffer->GetRealLength(),&nbr);

	/*!
   *Find the \r\n\r\n sequence.
   */
	for(u_long i=0; i<nbr; i++)
	{
		if((buffer[i]=='\r')&&(buffer[i+1]=='\n')&&
       (buffer[i+2]=='\r') &&(buffer[i+3]=='\n'))
		{
			headerSize= i + 4 ;
			break;
		}
	}
	sprintf(td->response.CONTENT_LENGTH, "%u", 
          (u_int)(con.tempOut.getFileSize()-headerSize));
	HttpHeaders::buildHTTPResponseHeaderStruct(&td->response,td,
                                             (char*)td->buffer->GetBuffer());

	for(;;)
	{
		if(td->response.LOCATION[0])
		{
      td->inputData.closeFile();
      File::deleteFile(td->inputDataPath);
      con.tempOut.closeFile();
      File::deleteFile(outDataPath);
      con.sock.closesocket();
			return ((Http*)td->lhttp)->sendHTTPRedirect(td, connection, 
                                                  td->response.LOCATION);
		}
		/*! Send the header. */
		if(!td->appendOutputs)
		{
			if(!lstrcmpi(td->request.CONNECTION,"Keep-Alive"))
				strcpy(td->response.CONNECTION,"Keep-Alive");		
			HttpHeaders::buildHTTPResponseHeader((char*)td->buffer2->GetBuffer(),
                                            &td->response);
			if(td->connection->socket.send( (char*)td->buffer2->GetBuffer(),
                                      (int)strlen((char*)td->buffer2->GetBuffer()),
                                      0) == SOCKET_ERROR )
      {
				exit = 1;
        ret = 0;
				break;
			}

      if(only_header)
      {
        exit = 1;
        ret = 1;
        break;
      }

			if(td->connection->socket.send((char*)(((char*)td->buffer->GetBuffer())
                                     +headerSize), nbr - headerSize, 0)==SOCKET_ERROR)
			{
				exit = 0;
        ret = 0;
				break;
			}
		}
		else/*! If appendOutputs. */
		{
      if(only_header)
      {
        exit = 1;
        ret = 1;
        break;
      }
			u_long nbw=0;

      /*!
       *Send remaining data stored in the buffer. 
       *This is the HTTP header.
       */
			if(td->outputData.writeToFile((char*)(((char*)td->buffer2->GetBuffer())
                                            + headerSize), nbr - headerSize, &nbw))
      {
				exit = 1;
        ret = 0;
				break;
      }
		}

    /*! Flush the data. */
		do
		{
			if(con.tempOut.readFromFile((char*)td->buffer->GetBuffer(), 
                                  td->buffer->GetRealLength(), &nbr))
      {
        exit = 1;
        ret = 0;
				break;
      }
			
			if(!td->appendOutputs)
			{
				if(td->connection->socket.send((char*)td->buffer->GetBuffer(),nbr, 0)
                                      == SOCKET_ERROR)
        {
          exit=1;
          ret = 0;
					break;
        }
			}
			else
			{
				u_long nbw=0;
				if(td->outputData.writeToFile((char*)td->buffer->GetBuffer(),nbr,&nbw))
        {
          exit=1;
          ret = 0;
					break;     
        }
			}
		}while(nbr);
	
    break;
	}
  td->inputData.closeFile();
  File::deleteFile(td->inputDataPath);
	con.tempOut.closeFile();
	File::deleteFile(outDataPath);
	con.sock.closesocket();
	return ret;
}

/*!
 *Send the buffer content over the FastCGI connection
 *Return non-zero on errors.
 */
int FastCgi::sendFcgiBody(fCGIContext* con,char* buffer,int len,int type,int id)
{
	FCGI_Header header;
	generateFcgiHeader( header, type, id, len );
	
	if(con->sock.send((char*)&header,sizeof(header),0)==-1)
		return -1;
	if(con->sock.send((char*)buffer,len,0)==-1)
		return -1;
	return 0;
}

/*!
 *Trasform from a standard environment string to the FastCGI environment string.
 */
int FastCgi::buildFASTCGIEnvironmentString(HttpThreadContext*,char* sp,char* ep)
{
	char *ptr=ep;
	char *sptr=sp;
	char varName[100];
	char varValue[2500];
	for(;;)
	{
		fourchar varNameLen;
		fourchar varValueLen;

		varNameLen.i=varValueLen.i=0;
		varName[0]='\0';
		varValue[0]='\0';
    int max = 100;
		while((--max) && *sptr != '=')
		{
			varName[varNameLen.i++]=*sptr++;
			varName[varNameLen.i]='\0';
		}
    if(max == 0)
      return -1;
		sptr++;
    max = 2500;
		while((--max) && *sptr != '\0')
		{
			varValue[varValueLen.i++]=*sptr++;
			varValue[varValueLen.i]='\0';
		}
    if(max == 0)
      return -1;
		if(varNameLen.i > 127)
		{
			unsigned char fb=varValueLen.c[3]|0x80;
			*ptr++=fb;
			*ptr++=varNameLen.c[2];
			*ptr++=varNameLen.c[1];
			*ptr++=varNameLen.c[0];
		}
		else
		{
			*ptr++=varNameLen.c[0];
		}

		if(varValueLen.i > 127)
		{
			unsigned char fb=varValueLen.c[3]|0x80;
			*ptr++=fb;
			*ptr++=varValueLen.c[2];
			*ptr++=varValueLen.c[1];
			*ptr++=varValueLen.c[0];
		}
		else
		{
			*ptr++=varValueLen.c[0];
		}
		u_long i;
		for(i=0;i<varNameLen.i; i++)
			*ptr++=varName[i];
		for(i=0;i<varValueLen.i; i++)
			*ptr++=varValue[i];
		if(*(++sptr)=='\0')
			break;
	}
	return (int)(ptr-ep);
}

/*!
 *Fill the FCGI_Header structure
 */
void FastCgi::generateFcgiHeader( FCGI_Header &header, int iType,
                                  int iRequestId, int iContentLength )
{
	header.version = FCGI_VERSION_1;
	header.type = (u_char)iType;
	header.requestIdB1 = (u_char)((iRequestId >> 8 ) & 0xff);
	header.requestIdB0 = (u_char)((iRequestId ) & 0xff);
	header.contentLengthB1 = (u_char)((iContentLength >> 8 ) & 0xff);
	header.contentLengthB0 = (u_char)((iContentLength ) & 0xff);
	header.paddingLength = 0;
	header.reserved = 0;
};

/*!
 *Constructor for the FASTCGI class
 */
FastCgi::FastCgi()
{
	initialized=0;
}

/*!
 *Initialize the FastCGI protocol implementation
 */
int FastCgi::load()
{
	if(initialized)
		return 1;
  fCGIservers = 0;
	fCGIserversN=0;
	memset(&fCGIservers, 0, sizeof(fCGIservers));
	initialized=1;
  servers_mutex.myserver_mutex_init();
	return 1;
}

/*!
 *Clean the memory and the processes occuped by the FastCGI servers
 */
int FastCgi::unload()
{
  sfCGIservers* list = fCGIservers;
  servers_mutex.myserver_mutex_lock();
  while(list)
  {
    /*! If the server is a remote one do nothing. */
		if(list->path[0]!='@')
    {
      list->socket.closesocket();
      terminateProcess(list->pid);
    }
    delete [] list->path;

    sfCGIservers* toremove = list;
    list = list->next;
    delete toremove;
  }
  servers_mutex.myserver_mutex_unlock();
  list = 0;
  servers_mutex.myserver_mutex_destroy();
	initialized=0;
	return 1;
}

/*!
 *Return the the running server speicified by path.
 *If the server is not running returns 0.
 */
sfCGIservers* FastCgi::isFcgiServerRunning(char* path)
{
  servers_mutex.myserver_mutex_lock();

  sfCGIservers *cur = fCGIservers;
  while(cur)
  {
    if(cur->path && (!lstrcmpi(path,cur->path)))
    {
      servers_mutex.myserver_mutex_unlock();
			return cur;  
    }
  }

  servers_mutex.myserver_mutex_unlock();
	return 0;
}

/*!
 *Get a client socket in the fCGI context structure
 */
int FastCgi::FcgiConnectSocket(fCGIContext* con, sfCGIservers* server )
{
	MYSERVER_HOSTENT *hp=Socket::gethostbyname(server->host);
	struct sockaddr_in sockAddr;
	int sockLen;
  if(hp == 0)
    return -1;

  sockLen = sizeof(sockAddr);
  memset(&sockAddr, 0, sizeof(sockAddr));
  sockAddr.sin_family = AF_INET;
	memcpy(&sockAddr.sin_addr, hp->h_addr, hp->h_length);
	sockAddr.sin_port = htons(server->port);
  
	/*! Try to create the socket. */
	if(con->sock.socket(AF_INET, SOCK_STREAM, 0) == -1)
	{
		return -1;
	}
  /*! If the socket was created try to connect. */
	if(con->sock.connect((MYSERVER_SOCKADDR*)&sockAddr, sockLen) == -1)
	{
		con->sock.closesocket();
		return -1;
	}
	con->sock.setNonBlocking(1);
	con->server = server;
	return 1;
}

/*!
 *Get a connection to the FastCGI server.
 */
sfCGIservers* FastCgi::FcgiConnect(fCGIContext* con,char* path)
{

	sfCGIservers* server = runFcgiServer(con, path);
	/*!
   *If we find a valid server try the connection to it.
   */
	if(server)
	{
		/*!
     *Connect to the FastCGI server.
     */
		int ret=FcgiConnectSocket(con, server);
		if(ret==-1)
			return 0;
	}
	return server;
}

/*!
 *Run the FastCGI server.
 *If the path starts with a @ character, the path is handled as a remote server.
 */
sfCGIservers* FastCgi::runFcgiServer(fCGIContext*,char* path)
{
  /*! Flag to identify a local server(running on localhost) from a remote one. */
	int localServer;
  
  /*! Path that init with @ are not local path. */
	localServer=path[0]!='@';

  /*! Get the server position in the array. */
  sfCGIservers* server	= isFcgiServerRunning(path);

  /*! If the process was yet initialized return its position. */
	if(server)
		return server;
	if(fCGIserversN==max_fcgi_servers-2)
		return 0;

  servers_mutex.myserver_mutex_lock();
	static u_short port=3333;

  sfCGIservers* new_server = new sfCGIservers();
  if(new_server == 0)
  {
    servers_mutex.myserver_mutex_unlock();
    return 0;
  }

	{
		/*! Create the server socket. */
		if(localServer)
		{/*! Initialize the local server. */
			strcpy(new_server->host, "localhost");
			new_server->port=port++;
			new_server->socket.socket(AF_INET,SOCK_STREAM,0);
			if(new_server->socket.getHandle() == (SocketHandle)INVALID_SOCKET)
      {
        servers_mutex.myserver_mutex_unlock();
        delete new_server;
				return 0;
      }

			MYSERVER_SOCKADDRIN sock_inserverSocket;
			sock_inserverSocket.sin_family=AF_INET;

			/*! The FastCGI server accepts connections only by the localhost. */
			sock_inserverSocket.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
			sock_inserverSocket.sin_port=htons(new_server->port);
			if(new_server->socket.bind((sockaddr*)&sock_inserverSocket, 
                                               sizeof(sock_inserverSocket)))
			{
        servers_mutex.myserver_mutex_unlock();
				new_server->socket.closesocket();
        delete new_server;
				return 0;
			}
			if(new_server->socket.listen(SOMAXCONN))
			{
        servers_mutex.myserver_mutex_unlock();
        new_server->socket.closesocket();
				return 0;
			}
			new_server->DESCRIPTOR.fileHandle = new_server->socket.getHandle();
			START_PROC_INFO spi;
			spi.cwd=0;
			spi.envString=0; 
			spi.cmd=path;
			spi.stdIn = (FileHandle)new_server->DESCRIPTOR.fileHandle;
			spi.cmdLine=path;

			/*! No argument so clear it. */
			spi.arg = NULL; 
      new_server->path = new char[strlen(spi.cmd)+1];

      if(new_server->path == 0)
      {
        servers_mutex.myserver_mutex_unlock();
        delete new_server;
        return 0;
      }
			strcpy(new_server->path, spi.cmd);

			spi.stdOut = spi.stdError =(FileHandle) -1;

			new_server->pid=execConcurrentProcess(&spi);

			if(new_server->pid == -1)
			{
        servers_mutex.myserver_mutex_unlock();
				new_server->socket.closesocket();
        delete new_server;
				return 0;
			}
		}/*! End local server initialization. */
		else
		{
      /*! Fill the structure with a remote server. */
      new_server->path = new char[ strlen(path) + 1 ];
      if(fCGIservers[fCGIserversN].path == 0)
      {
        servers_mutex.myserver_mutex_unlock();
        delete new_server;
        return 0;
      }
			strcpy(fCGIservers[fCGIserversN].path, path);

      /*! Do not copy the @ character. */
			int i=1;

			memset(new_server->host, 0, 128);

			/*!
       *A remote server path has the form @hosttoconnect:porttouse.
       */
			while(path[i]!=':')
				new_server->host[i-1]=path[i++];
			new_server->port=(u_short)atoi(&path[++i]);
		}
		
	}
  new_server->next = fCGIservers;
  fCGIservers = new_server;

  /*!
   *Increase the number of running servers.
   */
  fCGIserversN++;

  servers_mutex.myserver_mutex_unlock();  

  /*!
   *Return the new server.
   */
  return new_server;
}

/*!
 *Return the timeout value.
 */
int FastCgi::getTimeout()
{
  return timeout;
}

/*!
 *Set a new timeout.
 */
void FastCgi::setTimeout(int ntimeout)
{
  timeout = ntimeout;
}
