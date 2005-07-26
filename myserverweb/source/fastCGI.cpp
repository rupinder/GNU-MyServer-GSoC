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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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
#include "../include/stringutils.h"
#include "../include/cserver.h"

#include <string>
#include <sstream>
using namespace std;

/*! Running servers. */
HashDictionary<sserversList*> FastCgi::serversList;

/*! Is the fastcgi initialized? */
int FastCgi::initialized=0;

/*! By default allows 25 servers. */
int FastCgi::max_fcgi_servers=25;

/*! Use a default timeout of 15 seconds. */
int FastCgi::timeout=MYSERVER_SEC(15);

/*! By default start binding ports from 3333. */
int FastCgi::initialPort=3333;

/*! Mutex used to access fastCGI servers. */
Mutex FastCgi::servers_mutex;

struct FourChar
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
                  const char* scriptpath, const char *cgipath, 
                  int execute, int onlyHeader)
{
	FcgiContext con;
	FcgiBeginRequestBody tBody;
	con.td=td;
	u_long nbr=0;
	FcgiHeader header;

	u_long headerSize=0;

	int exit;
  int ret;
	
	clock_t time1;
	
	ostringstream outDataPath;

  int sizeEnvString;
  sserversList* server=0;
  int id;
	ostringstream fullpath;
  char *buffer=0;
  char tmpSize[11];

  const size_t maxStdinChunk=8192;/*! Size of data chunks to use with STDIN. */

  td->scriptPath.assign(scriptpath);

  {
    string tmp;
    tmp.assign(cgipath);
    File::splitPath(tmp, td->cgiRoot, td->cgiFile);
    tmp.assign(scriptpath);
    File::splitPath(tmp, td->scriptDir, td->scriptFile);
  }

  td->buffer->setLength(0);
	td->buffer2->getAt(0)='\0';
	if(execute)
	{
		if(cgipath && strlen(cgipath))
    {
#ifdef WIN32
      {
        int x;
        string cgipathString(cgipath);
        int len=strlen(cgipath);
        for(x=1;x< len; x++)
          if(cgipath[x]==' ' && cgipath[x-1]!='\\')
            break;
        if(x<len)
          fullpath << "\"" << cgipathString.substr(0, x) << "\"  " <<  
                   cgipathString.substr(x, len-1) << " " <<
                   td->filenamePath;
        else
			    fullpath << "\"" << cgipath << "\" \"" <<  td->filenamePath << "\"";
      }
#else
 			fullpath << cgipath << " " << td->filenamePath;     
#endif
    }
		else
    {
#ifdef WIN32
      {
        int x;
        int len = td->filenamePath.length();
        for(x=1;x<len; x++)
          if(td->filenamePath[x]==' ' && td->filenamePath[x-1]!='\\')
            break;
        
        if(x<len)
			    fullpath << "\"" << td->filenamePath.substr(0, x) << "\"" 
                   << td->filenamePath.substr(x, len-1);
        else
 			    fullpath << "\"" << td->filenamePath << "\"";           	
      }
#else
      fullpath << td->filenamePath;		   
#endif
		
    }
	}
	else
	{
#ifdef WIN32
    int x;
    string cgipathString(cgipath);
    int len=strlen(cgipath);
    for(x=1;x<len; x++)
    if(cgipath[x]==' ' && cgipath[x-1]!='\\')
      break;
    if(x<len)
      fullpath << "\"" << cgipathString.substr(0, x) << "\" " <<  
              cgipathString.substr(x, len-1);
      else
			  fullpath << "\"" << cgipath << "\"";
#else
    fullpath << cgipath;   
#endif
	}

  Cgi::buildCGIEnvironmentString(td,td->buffer->getBuffer());
  sizeEnvString=buildFASTCGIEnvironmentString(td,td->buffer->getBuffer(),
                                              td->buffer2->getBuffer());
  if(sizeEnvString == -1)
  {
		td->buffer->setLength(0);
    if(lserver->getVerbosity() > 2)
    {
      *td->buffer<< "FastCGI: Error to build env string\r\n" << '\0';
      ((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
      ((Vhost*)td->connection->host)->warningsLogWrite(td->buffer->getBuffer());
      ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
    }
		return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
  }
	td->inputData.closeFile();
	if(td->inputData.openFile(td->inputDataPath,
                         FILE_OPEN_READ | FILE_OPEN_ALWAYS | 
                            FILE_NO_INHERIT))
  {
		td->buffer->setLength(0);
    if(lserver->getVerbosity() > 2)
    {
      *td->buffer<< "FastCGI: Error opening stdin file\r\n" << '\0';
      ((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
      ((Vhost*)td->connection->host)->warningsLogWrite(td->buffer->getBuffer());
      ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
    }
		return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
  }

  server = fcgiConnect(&con,fullpath.str().c_str());
	if(server == 0)
  {
		td->buffer->setLength(0);
    if(lserver->getVerbosity() > 2)
    {
      *td->buffer<< "FastCGI: Error connecting to FastCGI "
                 << fullpath.str().c_str() << " process\r\n" << '\0';
      ((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
      ((Vhost*)td->connection->host)->warningsLogWrite(td->buffer->getBuffer());
      ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
    }
		return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
  }

	id=td->id+1;
	tBody.roleB1 = ( FCGIRESPONDER >> 8 ) & 0xff;
	tBody.roleB0 = ( FCGIRESPONDER ) & 0xff;
	tBody.flags = 0;
	memset( tBody.reserved, 0, sizeof( tBody.reserved ) );

	if(sendFcgiBody(&con,(char*)&tBody,sizeof(tBody),FCGIBEGIN_REQUEST,id))
	{
		td->buffer->setLength(0);
    if(lserver->getVerbosity() > 2)
    {
      *td->buffer<< "FastCGI: Error beginning the request\r\n" << '\0';
      ((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
      ((Vhost*)td->connection->host)->warningsLogWrite(td->buffer->getBuffer());
      ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
    }
		con.sock.closesocket();
		return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_501);
	}

	if(sendFcgiBody(&con,td->buffer2->getBuffer(),sizeEnvString,
                  FCGIPARAMS,id))
	{
		td->buffer->setLength(0);
    if(lserver->getVerbosity() > 2)
    {
      *td->buffer << "FastCGI: Error sending params\r\n" << '\0';
      ((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
      ((Vhost*)td->connection->host)->warningsLogWrite(td->buffer->getBuffer());
      ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
    }
		con.sock.closesocket();
		return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_501);
	}

	if(sendFcgiBody(&con,0,0,FCGIPARAMS,id))
	{
		td->buffer->setLength(0);
    if(lserver->getVerbosity() > 2)
    {
      *td->buffer << "FastCGI: Error sending params\r\n" << '\0';
      ((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
      ((Vhost*)td->connection->host)->warningsLogWrite(td->buffer->getBuffer());
      ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
    }
		con.sock.closesocket();
		return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
	}
	
	if(atoi(td->request.contentLength.c_str()))
	{
		td->buffer->setLength(0);


		if(td->inputData.setFilePointer(0))
      if(lserver->getVerbosity() > 2)
      {
        *td->buffer << "FastCGI: Error sending POST data\r\n"<< '\0';
        ((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
        ((Vhost*)td->connection->host)->warningsLogWrite(td->buffer->getBuffer());
        ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
      }

    /*! Send the STDIN data. */
		do
		{
      if(td->inputData.readFromFile(td->buffer->getBuffer(),
                                    maxStdinChunk, &nbr))
      {
        td->buffer->setLength(0);
        if(lserver->getVerbosity() > 2)
        {
          *td->buffer << "FastCGI: Error reading from file\r\n" << '\0';
          ((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
          ((Vhost*)td->connection->host)->warningsLogWrite(
                                                    td->buffer->getBuffer());
          ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
        }
        return ((Http*)td->lhttp)->sendHTTPhardError500(td, connection);
      }

      if(!nbr)
        break;
			
      generateFcgiHeader( header, FCGISTDIN, id, nbr);
      if(con.sock.send((char*)&header, sizeof(header), 0) == -1)
      {
        return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_501);
      }

      if(con.sock.send(td->buffer->getBuffer(),nbr,0) == -1)
      {
        td->buffer->setLength(0);
        if(lserver->getVerbosity() > 2)
        {
          *td->buffer << "FastCGI: Error sending data\r\n" << '\0';
          ((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
          ((Vhost*)td->connection->host)->warningsLogWrite(
                                                           td->buffer->getBuffer());
          ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
        }
        return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
      }
    }while(nbr==maxStdinChunk);
	}

  /*! Final stdin chunk. */
	if(sendFcgiBody(&con,0,0,FCGISTDIN,id))
	{
		td->buffer->setLength(0);
    if(lserver->getVerbosity() > 2)
    {
      *td->buffer << "FastCGI: Error sending POST data\r\n"<< '\0';
      ((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
      ((Vhost*)td->connection->host)->
                     warningsLogWrite(td->buffer->getBuffer());

      ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
    }
		con.sock.closesocket();
    return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
	}	

	/*! Now read the output. This flag is used by the external loop. */
	exit=0;

  /*! Return 1 if keep the connection. A nonzero value also mean no errors. */
  ret = 1;
	
	time1 = get_ticks();

  outDataPath << getdefaultwd(0, 0) << "/stdOutFileFcgi_" << (u_int)td->id ;

	if(con.tempOut.openFile(outDataPath.str().c_str(),FILE_OPEN_WRITE | 
                          FILE_OPEN_READ | FILE_CREATE_ALWAYS |
                          FILE_NO_INHERIT))
  {
    td->buffer->setLength(0);
		*td->buffer << "FastCGI: Error opening stdout file\r\n"<< '\0';
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)td->connection->host)->
                            warningsLogWrite(td->buffer->getBuffer());

		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
    return ((Http*)td->lhttp)->raiseHTTPError(td,connection,e_500);
  }

	do	
	{
		u_long dim;
		u_long data_sent;
    u_long nbw;
		while(con.sock.bytesToRead()<sizeof(FcgiHeader))
		{
			if((clock_t)(get_ticks()-time1) > timeout)
				break;
		}
		if(con.sock.bytesToRead())
    {
			nbr=con.sock.recv((char*)&header,sizeof(FcgiHeader),0);
      if(nbr == (u_long)-1)
      {
        td->buffer->setLength(0);
        *td->buffer << "FastCGI: Error reading data\r\n"<< '\0';
        ((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
        ((Vhost*)td->connection->host)->warningsLogWrite(
                                             td->buffer->getBuffer());
        ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
        sendFcgiBody(&con,0,0,FCGIABORT_REQUEST,id);
        ret = 0;
        break;
      }
    }
		else
		{
			td->buffer->setLength(0);
			*td->buffer << "FastCGI: Error timeout\r\n"<< '\0';
			((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
			((Vhost*)td->connection->host)->warningsLogWrite(td->buffer->getBuffer());
			((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
			sendFcgiBody(&con,0,0,FCGIABORT_REQUEST,id);
			con.sock.shutdown(2);
			con.sock.closesocket();
			break;
		}
		/*!
     *contentLengthB1 is the high word of the content length value
     *while contentLengthB0 is the low one.
     *To retrieve the value of content length push left contentLengthB1
     *of eight byte then do an or with contentLengthB0.
     */
		dim=(header.contentLengthB1<<8) | header.contentLengthB0;
		data_sent=0;
		if(dim==0)
		{
      exit = 1;
      ret = 1;
		}
		else
		{
			switch(header.type)
			{
				case FCGISTDERR:
					con.sock.closesocket();
					((Http*)td->lhttp)->raiseHTTPError(td, connection, e_501);
					exit = 1;
          ret = 0;
					break;
				case FCGISTDOUT:
					nbr=con.sock.recv(td->buffer->getBuffer(), (dim < td->buffer->getRealLength())
                            ? dim: td->buffer->getRealLength(), 0);
          if(nbr == (u_long)-1)
          {
						exit = 1;
            ret = 0;
						break;
          }

          if(con.tempOut.writeToFile(td->buffer->getBuffer(), nbr, &nbw))
          {
						exit = 1;
            ret = 0;
						break;
          }

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
							nbr=con.sock.recv(td->buffer->getBuffer(), (dim<data_sent)?dim :data_sent, 
                                td->buffer->getRealLength(), 0);
              if(nbr == (u_long)-1)
              {
                exit = 1;
                ret = 0;
                break;
              }
            }
						else
						{
              ret = 0;
							exit = 1;
							break;
						}
						if(con.tempOut.writeToFile((char*)(td->buffer->getBuffer()),nbr,&nbw))
            {
              ret = 0;
              exit = 1;
              break;
            }
						data_sent+=nbw;
					}
					break;
				case FCGIEND_REQUEST:
					exit = 1;
					break;			
				case FCGIGET_VALUES_RESULT:
				case FCGIUNKNOWN_TYPE:
				default:
					break;
			}
		}
	}while((!exit) && nbr);

	con.tempOut.setFilePointer(0);
	td->buffer->getAt(0)='\0';
	buffer=td->buffer->getBuffer();

  con.tempOut.setFilePointer(0);

  /*! Return an error message if ret is 0. */
  if((!ret) || con.tempOut.readFromFile(buffer,td->buffer->getRealLength(),&nbr))
  {
    con.tempOut.closeFile();
    File::deleteFile(outDataPath.str().c_str());
    con.sock.closesocket();
    return ((Http*)td->lhttp)->sendHTTPhardError500(td, connection);
  }
	
  /*!
   *find the \r\n\r\n sequence.
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
	sprintf(tmpSize, "%u", (u_int)(con.tempOut.getFileSize()-headerSize));
  td->response.contentLength.assign(tmpSize);
	HttpHeaders::buildHTTPResponseHeaderStruct(&td->response,td,
                                             td->buffer->getBuffer());

	for(;;)
	{
		if(td->response.location[0])
		{
      con.tempOut.closeFile();
      File::deleteFile(outDataPath.str().c_str());
      con.sock.closesocket();
			return ((Http*)td->lhttp)->sendHTTPRedirect(td, connection, 
                                              (char*)td->response.location.c_str());
		}
		/*! Send the header. */
		if(!td->appendOutputs)
		{
			if(!lstrcmpi(td->request.connection.c_str(), "Keep-Alive"))
				td->response.connection.assign("Keep-Alive");		
			HttpHeaders::buildHTTPResponseHeader(td->buffer2->getBuffer(),
                                            &td->response);
			if(td->connection->socket.send( td->buffer2->getBuffer(),
                                      static_cast<int>(strlen(td->buffer2->getBuffer())),
                                      0) == SOCKET_ERROR )
      {
				exit = 1;
        ret = 0;
				break;
			}

      if(onlyHeader)
      {
        exit = 1;
        ret = 1;
        break;
      }

			if(td->connection->socket.send((char*)((td->buffer->getBuffer())
                                     +headerSize), nbr - headerSize, 0)==SOCKET_ERROR)
			{
				exit = 0;
        ret = 0;
				break;
			}
		}
		else/*! If appendOutputs. */
		{
      if(onlyHeader)
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
			if(td->outputData.writeToFile((char*)((td->buffer2->getBuffer())
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
			if(con.tempOut.readFromFile(td->buffer->getBuffer(), 
                                  td->buffer->getRealLength(), &nbr))
      {
        exit = 1;
        ret = 0;
				break;
      }
			
			if(!td->appendOutputs)
			{
				if(td->connection->socket.send(td->buffer->getBuffer(),nbr, 0)
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
				if(td->outputData.writeToFile(td->buffer->getBuffer(),nbr,&nbw))
        {
          exit=1;
          ret = 0;
					break;     
        }
			}
		}while(nbr);
	
    break;
	}
	con.tempOut.closeFile();
	File::deleteFile(outDataPath.str().c_str());
	con.sock.closesocket();
	return ret;
}

/*!
 *Send the buffer content over the FastCGI connection
 *Return non-zero on errors.
 */
int FastCgi::sendFcgiBody(FcgiContext* con,char* buffer,int len,int type,int id)
{
	FcgiHeader header;
	generateFcgiHeader( header, type, id, len );
	
	if(con->sock.send((char*)&header,sizeof(header),0)==-1)
		return -1;
	if(con->sock.send((char*)buffer,len,0)==-1)
		return -1;
	return 0;
}

/*!
 *Trasform from a standard environment string to the FastCGI environment 
 *string.
 */
int FastCgi::buildFASTCGIEnvironmentString(HttpThreadContext*, char* sp, 
                                           char* ep)
{
	char *ptr=ep;
	char *sptr=sp;
	char varName[100];
	char varValue[2500];
	for(;;)
	{
    int max = 100;
		FourChar varNameLen;
		FourChar varValueLen;

		varNameLen.i=varValueLen.i=0;
		varName[0]='\0';
		varValue[0]='\0';
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
	return static_cast<int>(ptr-ep);
}

/*!
 *Fill the FcgiHeader structure
 */
void FastCgi::generateFcgiHeader( FcgiHeader &header, int iType,
                                  int iRequestId, int iContentLength )
{
	header.version = FCGIVERSION;
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
int FastCgi::load(XmlParser* /*confFile*/)
{
	if(initialized)
		return 1;
	initialized=1;
  servers_mutex.init();
	return 1;
}

/*!
 *Clean the memory and the processes occuped by the FastCGI servers
 */
int FastCgi::unload()
{
  servers_mutex.lock();
  try
  {
    int i;
    for(i=0; i<serversList.size() ; i++)
    {
      sserversList* server=serversList.getData(i);
      if(!server)
        continue;
      /*! If the server is a remote one do nothing. */
      if(server->path.length() && server->path[0]!='@')
      {
        server->socket.closesocket();
        server->process.terminateProcess();
      }
      server->path.assign("");
      delete server;
    }
    serversList.clear();
  }
  catch(bad_alloc& b)
  {
    servers_mutex.unlock();
    return 0;
  }
  catch(exception& e)
  {
    servers_mutex.unlock();
    return 0;
  }
  catch(...)
  {
    servers_mutex.unlock();
    return 0;
  };
  servers_mutex.unlock();
  servers_mutex.destroy();
  initialized=0;
	return 0;
}

/*!
 *Return the the running server specified by path.
 *If the server is not running returns 0.
 */
sserversList* FastCgi::isFcgiServerRunning(const char* path)
{
  servers_mutex.lock();

  try
  { 
    sserversList *s = serversList.getData(path);
    servers_mutex.unlock();
    return s;
  }
  catch(...)
  {
    servers_mutex.unlock();
  };
	return 0;
}

/*!
 *Get a client socket in the fCGI context structure
 */
int FastCgi::fcgiConnectSocket(FcgiContext* con, sserversList* server )
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
sserversList* FastCgi::fcgiConnect(FcgiContext* con, const char* path)
{

	sserversList* server = runFcgiServer(con, path);
	/*!
   *If we find a valid server try the connection to it.
   */
	if(server)
	{
		/*!
     *Connect to the FastCGI server.
     */
		int ret=fcgiConnectSocket(con, server);
		if(ret==-1)
			return 0;
	}
	return server;
}

/*!
 *Run the FastCGI server.
 *If the path starts with a @ character, the path is handled as a remote server.
 */
sserversList* FastCgi::runFcgiServer(FcgiContext* context, const char* path)
{
  /*! 
   *Flag to identify a local server(running on localhost) from a 
   *remote one. 
   */
	int localServer;
  int toReboot=0;
  sserversList* server;
	static u_short portsDelta=0;
 
  /*! Path that init with @ are not local path. */
	localServer=path[0]!='@';

  /*! Get the server position in the array. */
  server = isFcgiServerRunning(path);

  /*! If the process was yet initialized return it. */
	if(server)
  {
    if(!localServer)
      return server;
    if(server->process.isProcessAlive())
      return server;
    else 
      toReboot=1;
  }

  /*! Do not create it if we reach the max allowed. */
	if(serversList.size()==max_fcgi_servers-1)
		return 0;

  servers_mutex.lock();

  try
  {
    /*! Create the new structure if necessary. */
    if(!toReboot)
      server = new sserversList();
    if(server == 0)
    {
      if(lserver->getVerbosity() > 2)
      {
        *context->td->buffer<< "FastCGI: Error alloc memory\r\n" << '\0';
        ((Vhost*)(context->td->connection->host))->warningslogRequestAccess(context->td->id);
        ((Vhost*)context->td->connection->host)->warningsLogWrite(context->td->buffer->getBuffer());
        ((Vhost*)(context->td->connection->host))->warningslogTerminateAccess(context->td->id);
      }
      servers_mutex.unlock();
      return 0;
    }

    /*! Create the server socket. */
    if(localServer)
    {
      if(toReboot)
      {  
        int ret;
        server->socket.closesocket();
        server->process.terminateProcess();
        ret=runLocalServer(server, path, server->port);
        if(ret)
        {
          if(lserver->getVerbosity() > 1)
          {
            *context->td->buffer << "FastCGI: Error while rebooting " 
                                 << path << "\r\n" << '\0';
            ((Vhost*)(context->td->connection->host))->warningslogRequestAccess(context->td->id);
            ((Vhost*)context->td->connection->host)->warningsLogWrite(
                                                                      context->td->buffer->getBuffer());
            ((Vhost*)(context->td->connection->host))->warningslogTerminateAccess(context->td->id);
          }
          servers_mutex.unlock();
          return 0;
        }
        servers_mutex.unlock();
        return server;
      }
      else
      {
        int ret=runLocalServer(server, path, initialPort + (portsDelta++));
        servers_mutex.unlock();
        if(ret)
        {
          if(lserver->getVerbosity() > 1)
          {
            *context->td->buffer << "FastCGI: Error running " 
                                 << path << "\r\n" << '\0';
            ((Vhost*)(context->td->connection->host))->warningslogRequestAccess(context->td->id);
            ((Vhost*)context->td->connection->host)->warningsLogWrite(
                                                     context->td->buffer->getBuffer());
            ((Vhost*)(context->td->connection->host))->warningslogTerminateAccess(context->td->id);
          }
          delete server;
          servers_mutex.unlock();
          return 0;
        }
      }
    }
    else
    {
      /*! Do not copy the @ character. */
      int i=1;
      
      /*! Fill the structure with a remote server. */
      server->path.assign(path);
        
      memset(server->host, 0, 128);
      
      /*!
       *A remote server path has the form @hosttoconnect:porttouse.
       */
      while(path[i]!=':')
      {
        server->host[i-1]=path[i];
        i++;
      }      
      server->host[i-1]='\0';
      server->port=(u_short)atoi(&path[++i]);
    }
  
    serversList.insert(server->path.c_str(), server);

    servers_mutex.unlock();  

  }
  catch(...)
  {
    servers_mutex.unlock();
    throw;
  };
  /*!
   *Return the server.
   */
  return server;
}

/*!
 *Set the initial port for new servers.
 */
void FastCgi::setInitialPort(int nport)
{
  initialPort = nport;
}

/*!
 *Get the initial port.
 */
int FastCgi::getInitialPort()
{
  return initialPort;
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

/*!
 *Start the server on the specified port. Return zero on success.
 */
int FastCgi::runLocalServer(sserversList* server, const char* path, int port)
{
  int ret;
  StartProcInfo spi;
  MYSERVER_SOCKADDRIN sock_inserverSocket;
  strcpy(server->host, "localhost");
  server->port=port;
  server->socket.socket(AF_INET,SOCK_STREAM,0);
  if(server->socket.getHandle() == (SocketHandle)INVALID_SOCKET)
  {
    return 1;
  }
  
  sock_inserverSocket.sin_family=AF_INET;
  
  /*! The FastCGI server accepts connections only by the localhost. */
  sock_inserverSocket.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
  sock_inserverSocket.sin_port=htons(server->port);
  if(server->socket.bind((sockaddr*)&sock_inserverSocket, 
                             sizeof(sock_inserverSocket)))
  {
    server->socket.closesocket();
    return 1;
  }
  if(server->socket.listen(SOMAXCONN))
	{
    server->socket.closesocket();
    return 1;
  }
  server->DESCRIPTOR.fileHandle = server->socket.getHandle();
  spi.envString=0; 
  spi.stdIn = (FileHandle)server->DESCRIPTOR.fileHandle;
  spi.cmd.assign(path);
  spi.cmdLine.assign(path);
  server->path.assign(path);
  
  spi.stdOut = spi.stdError =(FileHandle) -1;
  
  ret = server->process.execConcurrentProcess(&spi);
  
  if(ret == -1)
	{
    server->socket.closesocket();
    return 1;
  }
  return 0;
}
