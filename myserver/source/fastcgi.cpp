/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006 The MyServer Team
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
 *To get more info about the FastCGI protocol please visit the official 
 *FastCGI site at: http://www.fastcgi.com.
 *On that site you can find samples and all the supported languages.
 */
#include "../include/fastcgi.h"
#include "../include/cgi.h"
#include "../include/http.h"
#include "../include/http_constants.h"
#include "../include/stringutils.h"
#include "../include/server.h"
#include "../include/filters_chain.h"
#include "../include/files_utility.h"
#include <string>
#include <sstream>
using namespace std;

/*! Running servers.  */
HashMap<string, FastCgiServersList*> FastCgi::serversList;

/*! Is the fastcgi initialized?  */
int FastCgi::initialized = 0;

/*! By default allows 25 servers.  */
int FastCgi::maxFcgiServers = 25;

/*! Use a default timeout of 15 seconds.  */
int FastCgi::timeout = MYSERVER_SEC(15);

/*! By default start binding ports from 3333.  */
int FastCgi::initialPort = 3333;

/*! Mutex used to access fastCGI servers.  */
Mutex FastCgi::serversMutex;

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
  maxFcgiServers = max;
}

/*!
 *Get the max number of servers that can be executed.
 */
int FastCgi::getMaxFcgiServers()
{
  return maxFcgiServers;
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
	u_long nbr = 0;
	FcgiHeader header;
  FiltersChain chain;

	u_long headerSize = 0;

	int exit;
  int ret;

	clock_t time1;

	ostringstream outDataPath;

  int sizeEnvString;
  FastCgiServersList* server = 0;
  int id;
	ostringstream cmdLine;
  char *buffer = 0;

  string moreArg;

	bool useChunks = false;
	bool keepalive = false;

	/*! Size of data chunks to use with STDIN.  */
  const size_t maxStdinChunk = 8192;

	con.td = td;

  td->scriptPath.assign(scriptpath);

  {
    string tmp;
    tmp.assign(cgipath);
    FilesUtility::splitPath(tmp, td->cgiRoot, td->cgiFile);
    tmp.assign(scriptpath);
    FilesUtility::splitPath(tmp, td->scriptDir, td->scriptFile);
  }

  chain.setProtocol(td->http);
  chain.setProtocolData(td);
  chain.setStream(&(td->connection->socket));
  if(td->mime)
  {
    u_long nbw;
    if(td->mime && Server::getInstance()->getFiltersFactory()->chain(&chain,
																										td->mime->filters,
                                                    &(td->connection->socket),
																										&nbw, 
																										1))
      {
        td->connection->host->warningslogRequestAccess(td->id);
        td->connection->host->warningsLogWrite(
                                             "FastCGI: Error loading filters");
        td->connection->host->warningslogTerminateAccess(td->id);
        chain.clearAllFilters();
        return td->http->raiseHTTPError(e_500);
      }
  }

  td->buffer->setLength(0);
	td->buffer2->getAt(0) = '\0';


  {
    /*! Do not modify the text between " and ".  */
    int i;
    int subString = cgipath[0] == '"';
    int len = strlen(cgipath);
    string tmpCgiPath;
    for(i = 1; i < len; i++)
    {
      if(!subString && cgipath[i]==' ')
        break;
      if(cgipath[i] == '"' && cgipath[i - 1] != '\\')
        subString = !subString;
    }
    /*!
     *Save the cgi path and the possible arguments.
     *the (x < len) case is when additional arguments are specified.
     *If the cgipath is enclosed between " and " do not consider them
     *when splitting directory and file name.
     */
    if(len)
    {
      if(i < len)
      {
        string tmpString(cgipath);
        int begin = tmpString[0]=='"' ? 1: 0;
        int end = tmpString[i] == '"' ? i : i - 1;
        tmpCgiPath.assign(tmpString.substr(begin, end - 1));
        moreArg.assign(tmpString.substr(i, len - 1));
      }
      else
      {
        int begin = (cgipath[0] == '"') ? 1 : 0;
        int end   = (cgipath[len] == '"') ? len - 1 : len;
        tmpCgiPath.assign(&cgipath[begin], end-begin);
        moreArg.assign("");
      }
      FilesUtility::splitPath(tmpCgiPath, td->cgiRoot, td->cgiFile);
    }
    tmpCgiPath.assign(scriptpath);
    FilesUtility::splitPath(tmpCgiPath, td->scriptDir, td->scriptFile);
  }

	if(execute)
	{
		if(cgipath && strlen(cgipath))
    {
#ifdef WIN32
      {
        int x;
        string cgipathString(cgipath);
        int len = strlen(cgipath);
        int subString = cgipath[0] == '"';

		    cmdLine << "\"" << td->cgiRoot << "/" << td->cgiFile << "\" " 
								<< moreArg << " \"" <<  td->filenamePath << "\"";
      }
#else
 			cmdLine << cgipath << " " << td->filenamePath;
#endif
    }/*if(execute).  */
		else
    {
      cmdLine << scriptpath;
    }
	}
	else
	{
#ifdef WIN32
    cmdLine << "\"" << td->cgiRoot << "/" << td->cgiFile
            << "\" " << moreArg;
#else
    if(moreArg.length())
      cmdLine << cgipath << " " << moreArg;
    else
      cmdLine << cgipath;
#endif
	}

  Cgi::buildCGIEnvironmentString(td, td->buffer->getBuffer());
  sizeEnvString=buildFASTCGIEnvironmentString(td,td->buffer->getBuffer(),
                                              td->buffer2->getBuffer());
  if(sizeEnvString == -1)
  {
		td->buffer->setLength(0);
    if(Server::getInstance()->getVerbosity() > 2)
    {
      *td->buffer << "FastCGI: Error to build env string" << '\0';
      td->connection->host->warningslogRequestAccess(td->id);
      td->connection->host->warningsLogWrite(td->buffer->getBuffer());
      td->connection->host->warningslogTerminateAccess(td->id);
    }
    chain.clearAllFilters();
		return td->http->raiseHTTPError(e_500);
  }
	td->inputData.closeFile();
	if(td->inputData.openFile(td->inputDataPath,
                         File::MYSERVER_OPEN_READ | File::MYSERVER_OPEN_ALWAYS |
                            File::MYSERVER_NO_INHERIT))
  {
		td->buffer->setLength(0);
    if(Server::getInstance()->getVerbosity() > 2)
    {
      *td->buffer << "FastCGI: Error opening stdin file" << '\0';
      td->connection->host->warningslogRequestAccess(td->id);
      td->connection->host->warningsLogWrite(td->buffer->getBuffer());
      td->connection->host->warningslogTerminateAccess(td->id);
    }
    chain.clearAllFilters();
		return td->http->raiseHTTPError(e_500);
  }

  server = fcgiConnect(&con,cmdLine.str().c_str());
	if(server == 0)
  {
		td->buffer->setLength(0);
    if(Server::getInstance()->getVerbosity() > 2)
    {
      *td->buffer << "FastCGI: Error connecting to FastCGI "
                  << cmdLine.str().c_str() << " process" << '\0';
      td->connection->host->warningslogRequestAccess(td->id);
      td->connection->host->warningsLogWrite(td->buffer->getBuffer());
      td->connection->host->warningslogTerminateAccess(td->id);
    }
    chain.clearAllFilters();
		return td->http->raiseHTTPError(e_500);
  }

	id = td->id + 1;
	tBody.roleB1 = ( FCGIRESPONDER >> 8 ) & 0xff;
	tBody.roleB0 = ( FCGIRESPONDER ) & 0xff;
	tBody.flags = 0;
	memset( tBody.reserved, 0, sizeof( tBody.reserved ) );

	if(sendFcgiBody(&con, (char*)&tBody, sizeof(tBody), FCGIBEGIN_REQUEST, id))
	{
		td->buffer->setLength(0);
    if(Server::getInstance()->getVerbosity() > 2)
    {
      *td->buffer<< "FastCGI: Error beginning the request" << '\0';
      td->connection->host->warningslogRequestAccess(td->id);
      td->connection->host->warningsLogWrite(td->buffer->getBuffer());
      td->connection->host->warningslogTerminateAccess(td->id);
    }
    chain.clearAllFilters();
		con.sock.closesocket();
		return td->http->raiseHTTPError(e_501);
	}

	if(sendFcgiBody(&con,td->buffer2->getBuffer(), sizeEnvString,
                  FCGIPARAMS, id))
	{
		td->buffer->setLength(0);
    if(Server::getInstance()->getVerbosity() > 2)
    {
      *td->buffer << "FastCGI: Error sending params" << '\0';
      td->connection->host->warningslogRequestAccess(td->id);
      td->connection->host->warningsLogWrite(td->buffer->getBuffer());
      td->connection->host->warningslogTerminateAccess(td->id);
    }
    chain.clearAllFilters();
		con.sock.closesocket();
		return td->http->raiseHTTPError(e_501);
	}

	if(sendFcgiBody(&con, 0, 0, FCGIPARAMS, id))
	{
		td->buffer->setLength(0);
    if(Server::getInstance()->getVerbosity() > 2)
    {
      *td->buffer << "FastCGI: Error sending params" << '\0';
      td->connection->host->warningslogRequestAccess(td->id);
      td->connection->host->warningsLogWrite(td->buffer->getBuffer());
      td->connection->host->warningslogTerminateAccess(td->id);
    }
    chain.clearAllFilters();
		con.sock.closesocket();
		return td->http->raiseHTTPError(e_500);
	}

	if(atoi(td->request.contentLength.c_str()))
	{
		td->buffer->setLength(0);


		if(td->inputData.setFilePointer(0))
      if(Server::getInstance()->getVerbosity() > 2)
      {
        *td->buffer << "FastCGI: Error sending POST data" << '\0';
        td->connection->host->warningslogRequestAccess(td->id);
        td->connection->host->warningsLogWrite(td->buffer->getBuffer());
        td->connection->host->warningslogTerminateAccess(td->id);
      }

    /*! Send the STDIN data.  */
		do
		{
      if(td->inputData.readFromFile(td->buffer->getBuffer(),
                                    maxStdinChunk, &nbr))
      {
        td->buffer->setLength(0);
        if(Server::getInstance()->getVerbosity() > 2)
        {
          *td->buffer << "FastCGI: Error reading from file" << '\0';
          td->connection->host->warningslogRequestAccess(td->id);
          td->connection->host->warningsLogWrite(
                                                    td->buffer->getBuffer());
          td->connection->host->warningslogTerminateAccess(td->id);
        }
        return td->http->sendHTTPhardError500();
      }

      if(!nbr)
        break;

      generateFcgiHeader( header, FCGISTDIN, id, nbr);
      if(con.sock.send((char*)&header, sizeof(header), 0) == -1)
      {
        chain.clearAllFilters();
        return td->http->raiseHTTPError(e_501);
      }

      if(con.sock.send(td->buffer->getBuffer(),nbr,0) == -1)
      {
        td->buffer->setLength(0);
        if(Server::getInstance()->getVerbosity() > 2)
        {
          *td->buffer << "FastCGI: Error sending data" << '\0';
          td->connection->host->warningslogRequestAccess(td->id);
          td->connection->host->warningsLogWrite(td->buffer->getBuffer());
          td->connection->host->warningslogTerminateAccess(td->id);
        }
        chain.clearAllFilters();
        return td->http->raiseHTTPError(e_500);
      }
    }while(nbr == maxStdinChunk);
	}

  /*! Final stdin chunk.  */
	if(sendFcgiBody(&con, 0, 0, FCGISTDIN, id))
	{
		td->buffer->setLength(0);
    if(Server::getInstance()->getVerbosity() > 2)
    {
      *td->buffer << "FastCGI: Error sending POST data" << '\0';
      td->connection->host->warningslogRequestAccess(td->id);
      td->connection->host->
                     warningsLogWrite(td->buffer->getBuffer());

      td->connection->host->warningslogTerminateAccess(td->id);
    }
		con.sock.closesocket();
    return td->http->raiseHTTPError(e_500);
	}

	/*! Now read the output. This flag is used by the external loop.  */
	exit = 0;

  /*! Return 1 if keep the connection. A nonzero value also mean no errors. */
  ret = 1;

	time1 = getTicks();

  outDataPath << getdefaultwd(0, 0) << "/stdOutFileFcgi_" << (u_int)td->id ;

	if(con.tempOut.createTemporaryFile(outDataPath.str().c_str()))
  {
    td->buffer->setLength(0);
		*td->buffer << "FastCGI: Error opening stdout file" << '\0';
		td->connection->host->warningslogRequestAccess(td->id);
		td->connection->host->warningsLogWrite(td->buffer->getBuffer());
		td->connection->host->warningslogTerminateAccess(td->id);
    return td->http->raiseHTTPError(e_500);
  }

	do
	{
		u_long dim;
		u_long data_sent;
    u_long nbw;
		while(con.sock.bytesToRead() < sizeof(FcgiHeader))
		{
			if((clock_t)(getTicks()-time1) > timeout)
				break;
		}
		if(con.sock.bytesToRead())
    {
			nbr = con.sock.recv((char*)&header, sizeof(FcgiHeader), 0, 
													static_cast<u_long>(timeout));
      if(nbr == (u_long)-1)
      {
        td->buffer->setLength(0);
        *td->buffer << "FastCGI: Error reading data" << '\0';
        td->connection->host->warningslogRequestAccess(td->id);
        td->connection->host->warningsLogWrite(td->buffer->getBuffer());
        td->connection->host->warningslogTerminateAccess(td->id);
        sendFcgiBody(&con, 0, 0, FCGIABORT_REQUEST, id);
        ret = 0;
        break;
      }
    }
		else
		{
			td->buffer->setLength(0);
			*td->buffer << "FastCGI: Error timeout" << '\0';
			td->connection->host->warningslogRequestAccess(td->id);
			td->connection->host->warningsLogWrite(td->buffer->getBuffer());
			td->connection->host->warningslogTerminateAccess(td->id);
			sendFcgiBody(&con, 0, 0, FCGIABORT_REQUEST, id);
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
		dim = (header.contentLengthB1<<8) | header.contentLengthB0;
		data_sent = 0;
		if(dim == 0)
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
					td->http->raiseHTTPError(e_501);
					exit = 1;
          ret = 0;
					break;
				case FCGISTDOUT:
					nbr = con.sock.recv(td->buffer->getBuffer(), 
															(dim < td->buffer->getRealLength())
                            ? dim: td->buffer->getRealLength(), 0, 
															static_cast<u_long>(timeout));
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

					data_sent = nbw;
					if(data_sent == 0)
					{
						exit = 1;
            ret = 0;
						break;
					}

					while(data_sent < dim)
					{
            nbr=con.sock.recv(td->buffer->getBuffer(),
                    std::min(static_cast<u_long>(td->buffer->getRealLength()),
                             dim-data_sent), 0, static_cast<u_long>(timeout));
            if(nbr == (u_long)-1)
            {
              exit = 1;
              ret = 0;
              break;
            }

						if(con.tempOut.writeToFile((char*)(td->buffer->getBuffer()), 
																			 nbr, &nbw))
            {
              ret = 0;
              exit = 1;
              break;
            }
						data_sent += nbw;
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
	td->buffer->getAt(0) = '\0';
	buffer = td->buffer->getBuffer();

  con.tempOut.setFilePointer(0);

  /*! Return an error message if ret is 0.  */
  if((!ret) || con.tempOut.readFromFile(buffer, 
																				td->buffer->getRealLength(), &nbr))
  {
    con.tempOut.closeFile();
    FilesUtility::deleteFile(outDataPath.str().c_str());
    con.sock.closesocket();
    chain.clearAllFilters();
    return td->http->sendHTTPhardError500();
  }

  /*!
   *find the \r\n\r\n sequence.
   */
	for(u_long i = 0; i < nbr; i++)
	{
		if((buffer[i] == '\r') && (buffer[i + 1] == '\n') &&
       (buffer[i + 2] == '\r') && (buffer[i + 3] == '\n'))
		{
			headerSize = i + 4 ;
			break;
		}
	}

	/* For logging.  */
	td->sentData += con.tempOut.getFileSize() - headerSize;

	HttpHeaders::buildHTTPResponseHeaderStruct(&td->response, td,
                                             td->buffer->getBuffer());

	for(;;)
	{
    u_long nbw2;
		if(td->response.location[0])
		{
      con.tempOut.closeFile();
      FilesUtility::deleteFile(outDataPath.str().c_str());
      con.sock.closesocket();
      chain.clearAllFilters();
			return td->http->sendHTTPRedirect((char*)td->response.location.c_str());
		}
		/*! Send the header.  */
		if(!td->appendOutputs)
		{
			HttpRequestHeader::Entry *connection = 
				td->request.other.get("Connection");

			if(connection && !lstrcmpi(connection->value->c_str(), "keep-alive"))
			{
				keepalive = true;
				td->response.connection.assign("keep-alive");
			}

			if(keepalive)
			{
				HttpResponseHeader::Entry *e;
				e = td->response.other.get("Transfer-Encoding");
				if(e)
					e->value->assign("chunked");
				else
				{
					e = new HttpResponseHeader::Entry();
					e->name->assign("Transfer-Encoding");
					e->value->assign("chunked");
					td->response.other.put(*(e->name), e);
				}
				useChunks = true;
			}

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

			if(useChunks)
			{
				ostringstream tmp;
				tmp << hex << (nbr - headerSize) << "\r\n";
				td->response.contentLength.assign(tmp.str());
				if(chain.write(tmp.str().c_str(), tmp.str().length(), &nbw2))
				{
					exit = 0;
					ret = 0;
					break;
				}
			}

			if(chain.write((char*)((td->buffer->getBuffer())
                             +headerSize), nbr - headerSize, &nbw2))
			{
				exit = 0;
        ret = 0;
				break;
			}

			if(useChunks && chain.write("\r\n", 2, &nbw2))
			{
				exit = 0;
        ret = 0;
				break;
			}
		}
		else/*! If appendOutputs.  */
		{
      u_long nbw = 0;    
      if(onlyHeader)
      {
        exit = 1;
        ret = 1;
        break;
      }

      /*!
       *Send remaining data stored in the buffer.
       *This is the HTTP header.
       */
			if(td->outputData.writeToFile((char*)((td->buffer2->getBuffer())
                                         + headerSize), nbr - headerSize, 
																		&nbw))
      {
				exit = 1;
        ret = 0;
				break;
      }
		}

    /*! Flush the data.  */
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
        u_long nbw2;
				if(nbr)
				{
					if(useChunks)
					{
						ostringstream tmp;
						tmp << hex << nbr << "\r\n";
						td->response.contentLength.assign(tmp.str());
						if(chain.write(tmp.str().c_str(), tmp.str().length(), &nbw2))
						{
							exit = 1;
							ret = 0;
							break;
						}
					}

					if(chain.write(td->buffer->getBuffer(), nbr, &nbw2))
					{
						exit = 1;
						ret = 0;
						break;
					}

					if(useChunks && chain.write("\r\n", 2, &nbw2))
					{
						exit = 1;
						ret = 0;
						break;
					}
				}
			}
			else
			{
				u_long nbw = 0;
				if(td->outputData.writeToFile(td->buffer->getBuffer(), nbr, &nbw))
        {
          exit = 1;
          ret = 0;
					break;
        }
			}
		}while(nbr);

		if(useChunks && chain.write("0\r\n\r\n", 5, &nbw2))
		{
			exit = 1;
			ret = 0;
			break;
		}

    break;
	}
  chain.clearAllFilters();
	con.tempOut.closeFile();
	FilesUtility::deleteFile(outDataPath.str().c_str());
	con.sock.closesocket();
	return ret;
}

/*!
 *Send the buffer content over the FastCGI connection
 *Return non-zero on errors.
 */
int FastCgi::sendFcgiBody(FcgiContext* con, char* buffer, int len, int type,
													int id)
{
	FcgiHeader header;
	generateFcgiHeader( header, type, id, len );

	if(con->sock.send((char*)&header, sizeof(header), 0) == -1)
		return -1;
	if(con->sock.send((char*)buffer, len, 0) == -1)
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
	char *ptr = ep;
	char *sptr = sp;
	char varName[100];
	char varValue[2500];
	for(;;)
	{
    int max = 100;
		FourChar varNameLen;
		FourChar varValueLen;

		varNameLen.i = varValueLen.i = 0;
		varName[0] = '\0';
		varValue[0] = '\0';
		while((--max) && *sptr != '=')
		{
			varName[varNameLen.i++] = *sptr++;
			varName[varNameLen.i] = '\0';
		}
    if(max == 0)
      return -1;
		sptr++;
    max = 2500;
		while((--max) && *sptr != '\0')
		{
			varValue[varValueLen.i++] = *sptr++;
			varValue[varValueLen.i] = '\0';
		}
    if(max == 0)
      return -1;
		if(varNameLen.i > 127)
		{
			unsigned char fb = varValueLen.c[3]|0x80;
			*ptr++ = fb;
			*ptr++ = varNameLen.c[2];
			*ptr++ = varNameLen.c[1];
			*ptr++ = varNameLen.c[0];
		}
		else
		{
			*ptr++ = varNameLen.c[0];
		}

		if(varValueLen.i > 127)
		{
			unsigned char fb = varValueLen.c[3]|0x80;
			*ptr++ = fb;
			*ptr++ = varValueLen.c[2];
			*ptr++ = varValueLen.c[1];
			*ptr++ = varValueLen.c[0];
		}
		else
		{
			*ptr++ = varValueLen.c[0];
		}
		u_long i;
		for(i = 0; i < varNameLen.i; i++)
			*ptr++ = varName[i];
		for(i = 0; i < varValueLen.i; i++)
			*ptr++ = varValue[i];
		if(*(++sptr) == '\0')
			break;
	}
	return static_cast<int>(ptr - ep);
}

/*!
 *Fill the FcgiHeader structure.
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
	initialized = 1;
  serversMutex.init();
	return 1;
}

/*!
 *Clean the memory and the processes occuped by the FastCGI servers
 */
int FastCgi::unload()
{
  serversMutex.lock();
  try
  {
		HashMap<string, FastCgiServersList*>::Iterator it = serversList.begin();

		for (;it != serversList.end(); it++)
		{
      FastCgiServersList* server = *it;
      if(!server)
        continue;
      /*! If the server is a remote one do nothing.  */
      if(server->path.length() && server->path[0] != '@')
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
    serversMutex.unlock();
    return 0;
  }
  catch(exception& e)
  {
    serversMutex.unlock();
    return 0;
  }
  catch(...)
  {
    serversMutex.unlock();
    return 0;
  };
  serversMutex.unlock();
  serversMutex.destroy();
  initialized = 0;
	return 0;
}

/*!
 *Return the the running server specified by path.
 *If the server is not running returns 0.
 */
FastCgiServersList* FastCgi::isFcgiServerRunning(const char* path)
{
  serversMutex.lock();

  try
  {
    FastCgiServersList *s = serversList.get(path);
    serversMutex.unlock();
    return s;
  }
  catch(...)
  {
    serversMutex.unlock();
  };
	return 0;
}

/*!
 *Get a client socket in the fCGI context structure
 */
int FastCgi::fcgiConnectSocket(FcgiContext* con, FastCgiServersList* server )
{
	// what address family has server's socket?
	MYSERVER_SOCKADDRIN  fastcgiServerSock = { 0 };
	socklen_t nLength = sizeof(MYSERVER_SOCKADDRIN);
	getsockname(server->socket.getHandle(), (sockaddr *)&fastcgiServerSock, 
							&nLength);
	if ( fastcgiServerSock.ss_family == AF_INET )
	{
		/*! Try to create the socket. */
		if(con->sock.socket(AF_INET, SOCK_STREAM, 0) == -1)
			return -1;
	  	/*! If the socket was created try to connect. */
		if(con->sock.connect(server->host, server->port) == -1)
		{
			con->sock.closesocket();
			return -1;
		}

	}
#if ( HAVE_IPV6 && false )/* IPv6 communication not implemented yet by php.  */
	else if ( fastcgiServerSock.ss_family == AF_INET6 )
	{
		/*! Try to create the socket.  */
		if(con->sock.socket(AF_INET6, SOCK_STREAM, 0) == -1)
			return -1;
		/*! If the socket was created try to connect.  */
		if(con->sock.connect(server->host, server->port) == -1)
		{
			con->sock.closesocket();
			return -1;
		}
	}
#endif // HAVE_IPV6
	con->sock.setNonBlocking(1);

	con->server = server;
	return 1;
}

/*!
 *Get a connection to the FastCGI server.
 */
FastCgiServersList* FastCgi::fcgiConnect(FcgiContext* con, const char* path)
{

	FastCgiServersList* server = runFcgiServer(con, path);
	/*!
   *If we find a valid server try the connection to it.
   */
	if(server)
	{
		/*!
     *Connect to the FastCGI server.
     */
		int ret = fcgiConnectSocket(con, server);
		if(ret == -1)
			return 0;
	}
	return server;
}

/*!
 *Return if the location is a remote one.
 *A remote location starts with a @.
 *\param location The location to check.
 */
bool FastCgi::isRemoteServer(const char* location) 
{
	if(!location) 
		return false;
	while(*location && *location == '\"')location++;
	return *location == '@';
}

/*!
 *Run the FastCGI server.
 *If the path starts with a @ character, the path is handled as a 
 *remote server.
 */
FastCgiServersList* FastCgi::runFcgiServer(FcgiContext* context, 
																					 const char* path)
{
  /*!
   *Flag to identify a local server(running on localhost) from a
   *remote one.
   */
	int localServer;
  int toReboot = 0;
  FastCgiServersList* server;
	static u_short portsDelta = 0;

  /*! Check if the specified location is remote. */
	localServer = !isRemoteServer(path);

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
      toReboot = 1;
  }

  /*! Do not create it if we reach the max allowed. */
	if(serversList.size() == maxFcgiServers - 1)
		return 0;

  serversMutex.lock();

  try
  {
    /*! Create the new structure if necessary. */
    if(!toReboot)
      server = new FastCgiServersList();
    if(server == 0)
    {
      if(Server::getInstance()->getVerbosity() > 2)
      {
        *context->td->buffer<< "FastCGI: Error allocating memory" << '\0';
        ((Vhost*)(context->td->connection->host))->warningslogRequestAccess(
                                                       context->td->id);
        ((Vhost*)context->td->connection->host)->warningsLogWrite(
																						 context->td->buffer->getBuffer());
        ((Vhost*)(context->td->connection->host))->warningslogTerminateAccess(
                                             context->td->id);
      }
      serversMutex.unlock();
      return 0;
    }

    /*! Create the server socket.  */
    if(localServer)
    {
      if(toReboot)
      {
        int ret;
        server->socket.closesocket();
        server->process.terminateProcess();
        ret = runLocalServer(server, path, server->port);
        if(ret)
        {
          if(Server::getInstance()->getVerbosity() > 1)
          {
            *context->td->buffer << "FastCGI: Error while rebooting "
                                 << path << '\0';
            ((Vhost*)(context->td->connection->host))->
							warningslogRequestAccess(context->td->id);
            ((Vhost*)context->td->connection->host)->
							warningsLogWrite(context->td->buffer->getBuffer());
            ((Vhost*)(context->td->connection->host))->
							warningslogTerminateAccess(context->td->id);
          }
          serversMutex.unlock();
          return 0;
        }
        serversMutex.unlock();
        return server;
      }
      else
      {
        int ret = runLocalServer(server, path, initialPort + (portsDelta++));
        serversMutex.unlock();
        if(ret)
        {
          if(Server::getInstance()->getVerbosity() > 1)
          {
            *context->td->buffer << "FastCGI: Error running "
                                 << path << '\0';
            ((Vhost*)(context->td->connection->host))->
							warningslogRequestAccess(context->td->id);
            ((Vhost*)context->td->connection->host)->
							warningsLogWrite(context->td->buffer->getBuffer());
            ((Vhost*)(context->td->connection->host))->
							warningslogTerminateAccess(context->td->id);
          }
          delete server;
          serversMutex.unlock();
          return 0;
        }
      }
    }
    else
    {
      /*! Do not copy the @ character.  */
      int i = 0;
      while(path[i] && (path[i] == '\"' || path[i] == '@' ))i++;

      /*! Fill the structure with a remote server. */
      server->path.assign(path);

      memset(server->host, 0, 128);

      /*!
       *A remote server path has the form @hosttoconnect:porttouse.
       */
      while(path[i] != ':')
      {
        server->host[i - 1]=path[i];
        i++;
      }
      server->host[i - 1] = '\0';
      server->port = (u_short)atoi(&path[++i]);
    }


		{
			FastCgiServersList* old;
			old = serversList.put(server->path, server);
			if(old)
			{
				/*! If the server is a remote one do nothing.  */
				if(!isRemoteServer(old->path.c_str()))
				{
					old->socket.closesocket();
					old->process.terminateProcess();
				}
				old->path.assign("");
				delete old;
			}
		}

    serversMutex.unlock();  

  }
  catch(...)
  {
    serversMutex.unlock();
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
int FastCgi::runLocalServer(FastCgiServersList* server, const char* path, int port)
{
  StartProcInfo spi;
  MYSERVER_SOCKADDRIN sock_inserverSocket;
  strcpy(server->host, "localhost");
  server->port=port;
  server->socket.socket(AF_INET,SOCK_STREAM,0);
  if(server->socket.getHandle() != (SocketHandle)INVALID_SOCKET)
  {
	  ((sockaddr_in *)(&sock_inserverSocket))->sin_family=AF_INET;

	  /*! The FastCGI server accepts connections only by the localhost. */
	  ((sockaddr_in *)(&sock_inserverSocket))->sin_addr.s_addr = 
			htonl(INADDR_LOOPBACK);
	  ((sockaddr_in *)(&sock_inserverSocket))->sin_port = 
			htons(server->port);
	  if ( !server->socket.bind(&sock_inserverSocket,
															sizeof(sockaddr_in)) )
	  {
		  if( !server->socket.listen(SOMAXCONN) )
			{
			  server->DESCRIPTOR.fileHandle = server->socket.getHandle();
			  spi.envString = 0;
			  spi.stdIn = (FileHandle)server->DESCRIPTOR.fileHandle;
			  spi.cmd.assign(path);
			  spi.cmdLine.assign(path);
			  server->path.assign(path);

			  spi.stdOut = spi.stdError =(FileHandle) -1;
			  if(server->process.execConcurrentProcess(&spi) == -1)
				{
					server->socket.closesocket();
					//return 1; allow IPv6
			  }
			  
		  }
		  else
		  {
				server->socket.closesocket();
				//return 1; allow IPv6
		}
	  }
	  else
		{
			server->socket.closesocket();
			//return 1; allow IPv6
		}
  }
  else
  {
#if ( HAVE_IPV6 && false/* IPv6 communication not implemented yet by php.  */ )
		server->socket.socket(AF_INET6, SOCK_STREAM, 0);
		if(server->socket.getHandle() != (SocketHandle)INVALID_SOCKET)
		{
			((sockaddr_in6 *)(&sock_inserverSocket))->sin6_family=AF_INET6;

			/*! The FastCGI server accepts connections only by the localhost.  */
			((sockaddr_in6 *)(&sock_inserverSocket))->sin6_addr=in6addr_any;
			((sockaddr_in6 *)(&sock_inserverSocket))->sin6_port=htons(server->port);
			if(server->socket.bind(&sock_inserverSocket,
														 sizeof(sockaddr_in6)))
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
			spi.envString = 0;
			spi.stdIn = (FileHandle)server->DESCRIPTOR.fileHandle;
			spi.cmd.assign(path);
			spi.cmdLine.assign(path);
			server->path.assign(path);

			spi.stdOut = spi.stdError =(FileHandle) -1;

			if(server->process.execConcurrentProcess(&spi) == -1)
			{
				server->socket.closesocket();
				return 1;
			}
		}
#endif
	}
  return 0;
}
