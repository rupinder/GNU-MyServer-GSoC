/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008 Free Software Foundation, Inc.
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

/*!
 *To get more info about the FastCGI protocol please visit the official 
 *FastCGI site at: http://www.fastcgi.com.
 *On that site you can find samples and all the supported languages.
 */
#include <include/http_handler/fastcgi/fastcgi.h>
#include <include/protocol/http/env/env.h>
#include <include/protocol/http/http.h>
#include <include/base/string/stringutils.h>
#include <include/server/server.h>
#include <include/filter/filters_chain.h>
#include <include/base/file/files_utility.h>
#include <include/base/string/securestr.h>

#include <string>
#include <sstream>
using namespace std;

#define SERVERS_DOMAIN "fastcgi"

/*! Is the fastcgi initialized?  */
int FastCgi::initialized = 0;

/*! Use a default timeout of 15 seconds.  */
int FastCgi::timeout = MYSERVER_SEC(15);

/*! Process server manager.  */
ProcessServerManager *FastCgi::processServerManager = 0;


struct FourChar
{
  union
  {
    unsigned int i;
    unsigned char c[4];
  };
};

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

  clock_t initialTicks;

  string outDataPath;

  int sizeEnvString;
  FastCgiServer* server = 0;
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
  chain.setStream(td->connection->socket);
  if(td->mime)
  {
    u_long nbw;
    if(td->mime && Server::getInstance()->getFiltersFactory()->chain(&chain,
                                                    td->mime->filters,
                                                    td->connection->socket,
                                                    &nbw, 
                                                    1))
      {
        td->connection->host->warningsLogWrite(
                                             "FastCGI: Error loading filters");
        chain.clearAllFilters();
        return td->http->raiseHTTPError(500);
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
        int end = tmpString[i] == '"' ? i - 1: i;
        tmpCgiPath.assign(tmpString.substr(begin, end - begin));
        moreArg.assign(tmpString.substr(i, len - 1));
      }
      else
      {
        int begin = (cgipath[0] == '"') ? 1 : 0;
        int end   = (cgipath[len] == '"') ? len - 1 : len;
        tmpCgiPath.assign(&cgipath[begin], end - begin);
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
    cmdLine << cgipath;
#endif
  }

  Env::buildEnvironmentString(td, td->buffer->getBuffer());
  sizeEnvString = buildFASTCGIEnvironmentString(td,td->buffer->getBuffer(),
                                                td->buffer2->getBuffer());
  if(sizeEnvString == -1)
  {
    td->buffer->setLength(0);
    if(Server::getInstance()->getVerbosity() > 2)
    {
      *td->buffer << "FastCGI: Error to build env string" << '\0';
      td->connection->host->warningsLogWrite(td->buffer->getBuffer());
    }
    chain.clearAllFilters();
    return td->http->raiseHTTPError(500);
  }
  td->inputData.close();
  if(td->inputData.openFile(td->inputDataPath, File::MYSERVER_OPEN_READ | 
                            File::MYSERVER_OPEN_ALWAYS |
                            File::MYSERVER_NO_INHERIT))
  {
    td->buffer->setLength(0);
    if(Server::getInstance()->getVerbosity() > 2)
    {
      *td->buffer << "FastCGI: Error opening stdin file" << '\0';
      td->connection->host->warningsLogWrite(td->buffer->getBuffer());
    }
    chain.clearAllFilters();
    return td->http->raiseHTTPError(500);
  }

  server = connect(&con, cmdLine.str().c_str());

  if(server == 0)
  {
    td->buffer->setLength(0);
    if(Server::getInstance()->getVerbosity() > 2)
    {
      *td->buffer << "FastCGI: Error connecting to FastCGI "
                  << cmdLine.str().c_str() << " process" << '\0';
      td->connection->host->warningsLogWrite(td->buffer->getBuffer());
    }
    chain.clearAllFilters();
    return td->http->raiseHTTPError(500);
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
      td->connection->host->warningsLogWrite(td->buffer->getBuffer());
    }
    chain.clearAllFilters();
    con.sock.close();
    return td->http->raiseHTTPError(500);
  }

  if(sendFcgiBody(&con,td->buffer2->getBuffer(), sizeEnvString,
                  FCGIPARAMS, id))
  {
    td->buffer->setLength(0);
    if(Server::getInstance()->getVerbosity() > 2)
    {
      *td->buffer << "FastCGI: Error sending params" << '\0';
      td->connection->host->warningsLogWrite(td->buffer->getBuffer());
    }
    chain.clearAllFilters();
    con.sock.close();
    return td->http->raiseHTTPError(501);
  }

  if(sendFcgiBody(&con, 0, 0, FCGIPARAMS, id))
  {
    td->buffer->setLength(0);
    if(Server::getInstance()->getVerbosity() > 2)
    {
      *td->buffer << "FastCGI: Error sending params" << '\0';
      td->connection->host->warningsLogWrite(td->buffer->getBuffer());
    }
    chain.clearAllFilters();
    con.sock.close();
    return td->http->raiseHTTPError(500);
  }

  if(atoi(td->request.contentLength.c_str()))
  {
    td->buffer->setLength(0);


    if(td->inputData.setFilePointer(0))
      if(Server::getInstance()->getVerbosity() > 2)
      {
        *td->buffer << "FastCGI: Error sending POST data" << '\0';
        td->connection->host->warningsLogWrite(td->buffer->getBuffer());
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
          td->connection->host->warningsLogWrite(
                                                    td->buffer->getBuffer());
        }
        return td->http->sendHTTPhardError500();
      }

      if(!nbr)
        break;

      generateFcgiHeader( header, FCGISTDIN, id, nbr);
      if(con.sock.send((char*)&header, sizeof(header), 0) == -1)
      {
        chain.clearAllFilters();
        return td->http->raiseHTTPError(501);
      }

      if(con.sock.send(td->buffer->getBuffer(),nbr,0) == -1)
      {
        td->buffer->setLength(0);
        if(Server::getInstance()->getVerbosity() > 2)
        {
          *td->buffer << "FastCGI: Error sending data" << '\0';
          td->connection->host->warningsLogWrite(td->buffer->getBuffer());
        }
        chain.clearAllFilters();
        return td->http->raiseHTTPError(500);
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
      td->connection->host->
                     warningsLogWrite(td->buffer->getBuffer());

    }
    con.sock.close();
    return td->http->raiseHTTPError(500);
  }

  /*! Now read the output. This flag is used by the external loop.  */
  exit = 0;

  /*! Return 1 if keep the connection. A nonzero value also mean no errors. */
  ret = 1;

  initialTicks = getTicks();

  FilesUtility::temporaryFileName(td->id, outDataPath);

  if(con.tempOut.createTemporaryFile(outDataPath.c_str()))
  {
    td->buffer->setLength(0);
    *td->buffer << "FastCGI: Error opening stdout file" << '\0';
    td->connection->host->warningsLogWrite(td->buffer->getBuffer());
    return td->http->raiseHTTPError(500);
  }

  do
  {
    u_long dim;
    u_long dataSent;
    u_long nbw;

    while(con.sock.bytesToRead() < sizeof(FcgiHeader))
    {
      if((clock_t)(getTicks() - initialTicks) > timeout)
        break;
      Thread::wait(1);
    }

    if(con.sock.bytesToRead() >= sizeof(FcgiHeader))
    {
      nbr = con.sock.recv((char*)&header, sizeof(FcgiHeader), 0, 
                          static_cast<u_long>(timeout));
      if(nbr != sizeof(FcgiHeader))
      {
        td->buffer->setLength(0);
        *td->buffer << "FastCGI: Error reading data" << '\0';
        td->connection->host->warningsLogWrite(td->buffer->getBuffer());
        sendFcgiBody(&con, 0, 0, FCGIABORT_REQUEST, id);
        ret = 0;
        break;
      }
    }
    else
    {
      td->buffer->setLength(0);
      *td->buffer << "FastCGI: Error timeout" << '\0';
      td->connection->host->warningsLogWrite(td->buffer->getBuffer());
      sendFcgiBody(&con, 0, 0, FCGIABORT_REQUEST, id);
      con.sock.shutdown(2);
      con.sock.close();
      break;
    }
    /*!
     *contentLengthB1 is the high word of the content length value
     *while contentLengthB0 is the low one.
     *To retrieve the value of content length push left contentLengthB1
     *of eight byte then do an or with contentLengthB0.
     */
    dim = (header.contentLengthB1 << 8) | header.contentLengthB0;
    dataSent = 0;
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
          con.sock.close();
          td->http->raiseHTTPError(501);
          exit = 1;
          ret = 0;
          break;
        case FCGISTDOUT:
          dataSent = 0;

          while(dataSent < dim)
          {
            nbr = con.sock.recv(td->buffer->getBuffer(),
                                std::min(static_cast<u_long>(td->buffer->getRealLength()),
                                         dim - dataSent), 0, static_cast<u_long>(timeout));
            if(nbr == (u_long)-1)
            {
              exit = 1;
              ret = 0;
              break;
            }

            if(con.tempOut.writeToFile((char*)(td->buffer->getBuffer()), 
                                       nbr, &nbw))
            {
              exit = 1;
              ret = 0;
              break;
            }
            dataSent += nbr;
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

    if(header.paddingLength)
    {
      u_long toPad = header.paddingLength;
      while(toPad)
      {
        nbr = con.sock.recv(td->buffer->getBuffer(),
                            std::min(toPad, (u_long)td->buffer->getRealLength()), 0, 
                            static_cast<u_long>(timeout));
        if(nbr == (u_long)-1)
        {
          exit = 1;
          ret = 0;
          break;
        }
        toPad -= nbr;
      }
    }

  }while((!exit) && nbr);

  con.tempOut.setFilePointer(0);
  td->buffer->getAt(0) = '\0';
  buffer = td->buffer->getBuffer();



  /*! Return an error message if ret is 0.  */
  if((!ret) || con.tempOut.readFromFile(buffer, 
                                        td->buffer->getRealLength(), &nbr))
  {
    con.tempOut.close();
    FilesUtility::deleteFile(outDataPath.c_str());
    con.sock.close();
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

  HttpHeaders::buildHTTPResponseHeaderStruct(td->buffer->getBuffer(),
                                             &td->response, 
                                             &(td->nBytesToRead));


  for(;;)
  {
    u_long nbw2;
    if(td->response.location[0])
    {
      con.tempOut.close();
      FilesUtility::deleteFile(outDataPath.c_str());
      con.sock.close();
      chain.clearAllFilters();
      return td->http->sendHTTPRedirect((char*)td->response.location.c_str());
    }
    /*! Send the header.  */
    if(!td->appendOutputs)
    {
      checkDataChunks(td, &keepalive, &useChunks);

      HttpHeaders::buildHTTPResponseHeader(td->buffer2->getBuffer(),
                                            &td->response);
      if(td->connection->socket->send( td->buffer2->getBuffer(),
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

      if(nbr - headerSize)
      {
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

    if (td->response.getStatusType () == HttpResponseHeader::SUCCESSFUL)
    {
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
    }

    break;
  }
  chain.clearAllFilters();
  con.tempOut.close();
  FilesUtility::deleteFile(outDataPath.c_str());
  con.sock.close();
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
int FastCgi::buildFASTCGIEnvironmentString(HttpThreadContext*, char* src,
                                           char* dest)
{
  char *ptr = dest;
  char *sptr = src;
  char varName[100];
  char varValue[2500];
  for(;;)
  {
    int max = 100;
    u_long i;
    FourChar varNameLen;
    FourChar varValueLen;

    varNameLen.i = varValueLen.i = 0;
    varName[0] = '\0';
    varValue[0] = '\0';

    while(*sptr == '\0')
      sptr++;

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
      unsigned char fb = varValueLen.c[3] | 0x80;
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
      unsigned char fb = varValueLen.c[3] | 0x80;
      *ptr++ = fb;
      *ptr++ = varValueLen.c[2];
      *ptr++ = varValueLen.c[1];
      *ptr++ = varValueLen.c[0];
    }
    else
    {
      *ptr++ = varValueLen.c[0];
    }
    for(i = 0; i < varNameLen.i; i++)
      *ptr++ = varName[i];
    for(i = 0; i < varValueLen.i; i++)
      *ptr++ = varValue[i];
    if(*(++sptr) == '\0')
      break;
  }
  return static_cast<int>(ptr - dest);
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
}

/*!
 *Constructor for the FASTCGI class
 */
FastCgi::FastCgi()
{
  initialized = 0;
}

/*!
 *Initialize the FastCGI protocol implementation
 */
int FastCgi::load(XmlParser* /*confFile*/)
{
  if(initialized)
    return 1;
  initialized = 1;
  processServerManager = Server::getInstance()->getProcessServerManager();
  processServerManager->createDomain(SERVERS_DOMAIN);
  return 0;
}

/*!
 *Clean the memory and the processes occuped by the FastCGI servers
 */
int FastCgi::unLoad()
{
  initialized = 0;
  return 0;
}

/*!
 *Return the the running server specified by path.
 *If the server is not running returns 0.
 */
FastCgiServer* FastCgi::isFcgiServerRunning(const char* path)
{
  return processServerManager->getServer(SERVERS_DOMAIN, path);
}


/*!
 *Get a connection to the FastCGI server.
 */
FastCgiServer* FastCgi::connect(FcgiContext* con, const char* path)
{
  FastCgiServer* server = runFcgiServer(con, path);
  /*!
   *If we find a valid server try the connection to it.
   */
  if(server)
  {
    int ret = processServerManager->connect(&(con->sock), server);

    if(ret == -1)
      return 0;
  }
  return server;
}

/*!
 *Run the FastCGI server.
 *If the path starts with a @ character, the path is handled as a 
 *remote server.
 */
FastCgiServer* FastCgi::runFcgiServer(FcgiContext* context, 
                                      const char* path)
{
  FastCgiServer* server =  processServerManager->getServer(SERVERS_DOMAIN, 
                                                          path);
  if(server)
    return server;

  /* If the path starts with @ then the server is remote.  */
  if(path[0] == '@')
  {
    int i = 1;
    char host[128];
    char port[6];

    while(path[i] && path[i] != ':')
      i++;

    myserver_strlcpy(host, &path[1], min(128, i));

    myserver_strlcpy(port, &path[i + 1], 6);
    
    return processServerManager->addRemoteServer(SERVERS_DOMAIN, path, 
                                                 host, atoi(port));
  }

  return processServerManager->runAndAddServer(SERVERS_DOMAIN, path);
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
