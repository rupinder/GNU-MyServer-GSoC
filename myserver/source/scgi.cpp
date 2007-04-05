/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006, 2007 The MyServer Team
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
#include "../include/scgi.h"
#include "../include/cgi.h"
#include "../include/http.h"
#include "../include/http_constants.h"
#include "../include/stringutils.h"
#include "../include/server.h"
#include "../include/files_utility.h"
#include "../include/securestr.h"

#include <string>
#include <sstream>
using namespace std;

#define SERVERS_DOMAIN "scgi"

/*! Is the scgi initialized?  */
int Scgi::initialized = 0;

/*! Use a default timeout of 15 seconds.  */
int Scgi::timeout = MYSERVER_SEC(15);

/*! Process server manager.  */
ProcessServerManager *Scgi::processServerManager = 0;


/*!
 *Entry-Point to manage a SCGI request.
 */
int Scgi::send(HttpThreadContext* td, ConnectionPtr connection,
							 const char* scriptpath, const char *cgipath,
							 int execute, int onlyHeader)
{
	ScgiContext con;
  FiltersChain chain;

  int ret;

	string outDataPath;

  int sizeEnvString;
  ScgiServer* server = 0;
	ostringstream cmdLine;

  string moreArg;
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
			td->connection->host->warningslogRequestAccess(td->id);
			td->connection->host->warningsLogWrite("SCGI: Error loading filters");
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

  Cgi::buildCGIEnvironmentString(td, td->buffer->getBuffer());
  sizeEnvString = buildScgiEnvironmentString(td,td->buffer->getBuffer(),
																						 td->buffer2->getBuffer());
  if(sizeEnvString == -1)
  {
		td->buffer->setLength(0);
    if(Server::getInstance()->getVerbosity() > 2)
    {
      *td->buffer << "SCGI: Error to build env string" << '\0';
      td->connection->host->warningslogRequestAccess(td->id);
      td->connection->host->warningsLogWrite(td->buffer->getBuffer());
      td->connection->host->warningslogTerminateAccess(td->id);
    }
    chain.clearAllFilters();
		return td->http->raiseHTTPError(e_500);
  }
	td->inputData.closeFile();
	if(td->inputData.openFile(td->inputDataPath, File::MYSERVER_OPEN_READ | 
														File::MYSERVER_OPEN_ALWAYS |
                            File::MYSERVER_NO_INHERIT))
  {
		td->buffer->setLength(0);
    if(Server::getInstance()->getVerbosity() > 2)
    {
      *td->buffer << "SCGI: Error opening stdin file" << '\0';
      td->connection->host->warningslogRequestAccess(td->id);
      td->connection->host->warningsLogWrite(td->buffer->getBuffer());
      td->connection->host->warningslogTerminateAccess(td->id);
    }
    chain.clearAllFilters();
		return td->http->raiseHTTPError(e_500);
  }

  server = connect(&con, cmdLine.str().c_str());

	if(server == 0)
  {
		td->buffer->setLength(0);
    if(Server::getInstance()->getVerbosity() > 2)
    {
      *td->buffer << "SCGI: Error connecting to SCGI "
                  << cmdLine.str().c_str() << " process" << '\0';
      td->connection->host->warningslogRequestAccess(td->id);
      td->connection->host->warningsLogWrite(td->buffer->getBuffer());
      td->connection->host->warningslogTerminateAccess(td->id);
    }
    chain.clearAllFilters();
		return td->http->raiseHTTPError(e_500);
  }
	ret = sendNetString(&con, td->buffer2->getBuffer(), sizeEnvString);
	
	if(td->request.contentLength.size() && 
		 !td->request.contentLength.compare("0"))
	{
		if(sendPostData(&con))
	 	{
			chain.clearAllFilters();
			return td->http->raiseHTTPError(e_500);
		}
	}
	
	ret = !sendResponse(&con, onlyHeader, &chain);


  chain.clearAllFilters();
	con.tempOut.closeFile();

	con.sock.closesocket();
	return ret;
}


/*!
 *Send the response to the client.
 */
int Scgi::sendResponse(ScgiContext* ctx, int onlyHeader, FiltersChain* chain)
{
	clock_t initialTicks = getTicks();
	bool useChunks = false;
	bool keepalive = false;
	u_long read = 0;
	u_long headerSize = 0;
	u_long tmpHeaderSize = 0;
	u_long nbw, nbr;
	u_long sentData = 0;
	HttpThreadContext* td = ctx->td;
	HttpRequestHeader::Entry* e = td->request.other.get("Connection");
	if(e)
		keepalive = !lstrcmpi(e->value->c_str(),"keep-alive");
	else
		keepalive = false;

	/* Do not use chunked transfer with old HTTP/1.0 clients.  */
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

	for(;;)
	{
		while(!ctx->sock.bytesToRead())
		{
			if((clock_t)(getTicks() - initialTicks) > timeout)
				break;
			Thread::wait(1);
		}

		if(!ctx->sock.bytesToRead())
			return -1;

		nbr = ctx->sock.recv(td->buffer2->getBuffer() + read,
												td->buffer2->getRealLength() - read,
												static_cast<u_long>(timeout));

		read += nbr;


		for(tmpHeaderSize = (tmpHeaderSize > 3) 
					? tmpHeaderSize - 4 : tmpHeaderSize;
				tmpHeaderSize < read - 4; tmpHeaderSize++)
			if((td->buffer2->getBuffer()[tmpHeaderSize] == '\r') &&
				 (td->buffer2->getBuffer()[tmpHeaderSize + 1] == '\n') &&
				 (td->buffer2->getBuffer()[tmpHeaderSize + 2] == '\r') &&
				 (td->buffer2->getBuffer()[tmpHeaderSize + 3] == '\n'))
			{
				headerSize = tmpHeaderSize + 4;
				break;
			}
		if(headerSize)
			break;
	}


	if(!td->appendOutputs)
	{
		if(headerSize)
		{
			HttpHeaders::buildHTTPResponseHeaderStruct(&td->response, td, 
																								 td->buffer2->getBuffer());
		}
		HttpHeaders::buildHTTPResponseHeader(td->buffer->getBuffer(),
																				 &td->response);
		
		if(chain->write(td->buffer->getBuffer(), strlen(td->buffer->getBuffer()),
										&nbw))
			return -1;
	}

	if(read - headerSize)
		if(appendDataToHTTPChannel(td, td->buffer2->getBuffer() + headerSize,
															 read - headerSize,
															 &(td->outputData), 
															 chain,
															 td->appendOutputs, 
															 useChunks))
		 return -1;
	
	sentData += read - headerSize;

	for(;;)
	{
		nbr = ctx->sock.recv(td->buffer2->getBuffer(),
												 td->buffer2->getRealLength(),
												 0);

		if(!nbr || (nbr == (u_long)-1))
			break;

		if(appendDataToHTTPChannel(td, td->buffer2->getBuffer(),
															 nbr,
															 &(td->outputData), 
															 chain,
															 td->appendOutputs, 
															 useChunks))
		 return -1;

		sentData += nbr;
	}

	if(!td->appendOutputs && useChunks)
	{
		if(chain->write("0\r\n\r\n", 5, &nbw))
			return -1;
	}

	/* For logging activity.  */	
	td->sentData += sentData;

	return 0;
}

/*!
 *Send a netstring to the SCGI server.
 */
int Scgi::sendNetString(ScgiContext* ctx, const char* data, int len)
{
	char header[7];
	int headerLen = sprintf(header, "%i:", len);

	if(ctx->sock.send(header, headerLen, 0) == -1)
		return -1;

	if(ctx->sock.send(data, len, 0) == -1)
		return -1;

	if(ctx->sock.send(",", 1, 0) == -1)
		return -1;

	return 0;
}

/*!
 *Send the post data to the SCGI server.
 */
int Scgi::sendPostData(ScgiContext* ctx)
{
		u_long nbr;
		do
		{
			if(ctx->td->inputData.readFromFile(ctx->td->buffer2->getBuffer(),
																				 ctx->td->buffer2->getRealLength(), 
																				 &nbr))
			{
				return -1;
			}

			if(nbr && (ctx->sock.send(ctx->td->buffer2->getBuffer(), nbr, 0) == -1))
			{ 
				return -1;
			}
		}
		while(nbr);
		return 0;
}

/*!
 *Trasform from a standard environment string to the SCGI environment
 *string.
 */
int Scgi::buildScgiEnvironmentString(HttpThreadContext* td, char* src,
																		 char* dest)
{
	char *ptr = dest;
	char *sptr = src;
	char varName[100];
	char varValue[2500];
	
	ptr += myserver_strlcpy(ptr, "CONTENT_LENGTH", 15);
	*ptr++ = '\0';

	if( td->request.contentLength.size())
		ptr += myserver_strlcpy(ptr, td->request.contentLength.c_str(),  
														td->request.contentLength.size() + 1);
	else
		*ptr++ = '0';
		
	*ptr++ = '\0';

	ptr += myserver_strlcpy(ptr, "SCGI", 5);
	*ptr++ = '\0';

	ptr += myserver_strlcpy(ptr, "1", 2);
	*ptr++ = '\0';

	for(;;)
	{
		int i;
    int max = 100;
		int varNameLen;
		int varValueLen;

		varNameLen = varValueLen = 0;
		varName[0] = '\0';
		varValue[0] = '\0';

		while(*sptr == '\0')
			sptr++;

		while((--max) && *sptr != '=')
		{
			varName[varNameLen++] = *sptr++;
			varName[varNameLen] = '\0';
		}
    if(max == 0)
      return -1;
		sptr++;
    max = 2500;
		while((--max) && *sptr != '\0')
	 	{
			varValue[varValueLen++] = *sptr++;
			varValue[varValueLen] = '\0';
		}

    if(max == 0)
      return -1;

		if(!strcmpi(varName, "CONTENT_LENGTH") || !strcmpi(varName, "SCGI") ||
			 !varNameLen || !varValueLen)
			continue;

		for(i = 0; i < varNameLen; i++)
			*ptr++ = varName[i];
		*ptr++ = '\0';

		for(i = 0; i < varValueLen; i++)
			*ptr++ = varValue[i];
		*ptr++ = '\0';

		if(*(++sptr) == '\0')
			break;
	}
	return static_cast<int>(ptr - dest);
}

/*!
 *Constructor for the FASTCGI class
 */
Scgi::Scgi()
{
	initialized = 0;
}

/*!
 *Initialize the SCGI protocol implementation
 */
int Scgi::load(XmlParser* /*confFile*/)
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
int Scgi::unload()
{
  initialized = 0;
	return 0;
}

/*!
 *Return the the running server specified by path.
 *If the server is not running returns 0.
 */
ScgiServer* Scgi::isScgiServerRunning(const char* path)
{
	return processServerManager->getServer(SERVERS_DOMAIN, path);
}


/*!
 *Get a connection to the FastCGI server.
 */
ScgiServer* Scgi::connect(ScgiContext* con, const char* path)
{
	ScgiServer* server = runScgiServer(con, path);
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
 *Run the SCGI server.
 *If the path starts with a @ character, the path is handled as a 
 *remote server.
 */
ScgiServer* Scgi::runScgiServer(ScgiContext* context, 
																const char* path)
{
	ScgiServer* server =	processServerManager->getServer(SERVERS_DOMAIN, 
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
int Scgi::getTimeout()
{
  return timeout;
}

/*!
 *Set a new timeout.
 */
void Scgi::setTimeout(int ntimeout)
{
  timeout = ntimeout;
}
