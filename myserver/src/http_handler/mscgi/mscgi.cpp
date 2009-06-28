/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008, 2009 Free Software Foundation, Inc.
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
#include <include/http_handler/mscgi/mscgi.h>
#include <include/protocol/http/http.h>
#include <include/server/server.h>
#include <include/protocol/http/env/env.h>
#include <include/base/base64/mime_utils.h>
#include <include/base/file/file.h>
#include <include/base/file/files_utility.h>
#include <include/base/socket/socket.h>
#include <include/base/utility.h>
#include <include/filter/filters_chain.h>
#include <sstream>
using namespace std; 

DynamicLibrary MsCgi::mscgiModule;

/*!
 *Sends the MyServer CGI; differently from standard CGI this doesn't 
 *need a new process to run making it faster.
 *\param td The HTTP thread context.
 *\param exec The script path.
 *\param cmdLine The command line.
 *\param execute Specify if the script has to be executed.
 *\param onlyHeader Specify if send only the HTTP header.
 */
int MsCgi::send(HttpThreadContext* td, const char* exec, const char* cmdLine,
                bool /*execute*/, bool onlyHeader)

{
  /*
   *This is the code for manage a .mscgi file.
   *This files differently from standard CGI don't need a new process to run
   *but are allocated in the caller process virtual space.
   *Usually these files are faster than standard CGI.
   *Actually myServerCGI(.mscgi) is only at an alpha status.
   */
  ostringstream tmpStream;
  string outDataPath;
  FiltersChain chain;
  u_long nbw;
#ifndef WIN32
#ifdef DO_NOT_USE_MSCGI
  /*!
   *On the platforms where is not available the MSCGI support send a 
   *non implemented error.
   */
  return td->http->raiseHTTPError(501);
#endif
#endif

#ifndef DO_NOT_USE_MSCGI 
  DynamicLibrary hinstLib; 
  CGIMAIN ProcMain = 0;
  int ret = 0;

   data.envString = td->request.uriOptsPtr ?
                    td->request.uriOptsPtr : (char*) td->buffer->getBuffer();
  
  data.td = td;
  data.errorPage = 0;
  data.server = Server::getInstance();
  data.mscgi = this;
  data.useChunks = false;
  data.onlyHeader = onlyHeader ? true : false;
  data.error = false;
  data.filtersChain = &chain;
  data.headerSent = false;
  data.keepAlive = false;
  data.useChunks = false;

  if (!(td->permissions & MYSERVER_PERMISSION_EXECUTE))
    return td->http->sendAuth();

  td->scriptPath.assign(exec);

  {
    string tmp;
    tmp.assign(exec);
    FilesUtility::splitPath(tmp, td->cgiRoot, td->cgiFile);
    FilesUtility::splitPath(exec, td->scriptDir, td->scriptFile);
  }

  Env::buildEnvironmentString(td,data.envString);
  
  chain.setProtocol(td->http);
  chain.setProtocolData(td);
  chain.setStream(td->connection->socket);

  if(td->mime && Server::getInstance()->getFiltersFactory()->chain(&chain, 
                                                 td->mime->filters, 
                                                 td->connection->socket, 
                                                                   &nbw, 
                                                                   1))
  {
    td->connection->host->warningsLogWrite("MSCGI: Error loading filters");
    chain.clearAllFilters(); 
    return td->http->raiseHTTPError(500);
  }

  checkDataChunks(td, &(data.keepAlive), &(data.useChunks));

  ret = hinstLib.loadLibrary(exec, 0);

  if (!ret) 
  { 
    td->secondaryBuffer->getAt(0) = '\0';

    ProcMain = (CGIMAIN) hinstLib.getProc( "myserver_main"); 

    if(ProcMain)
    {
      (ProcMain)(td->request.uriOpts.c_str (), &data);
    }
    else
    {
      string msg;
      msg.assign("MSCGI: error accessing entrypoint for ");
      msg.append(exec);
      td->connection->host->warningsLogWrite(msg.c_str());
      return td->http->raiseHTTPError(500);
    }
    hinstLib.close();
  } 
  else
  {
    chain.clearAllFilters(); 

    /* Internal server error.  */
    return td->http->raiseHTTPError(500);
  }
  if(data.errorPage)
  {
    chain.clearAllFilters(); 
    return td->http->raiseHTTPError(data.errorPage);
  }

  if(!td->appendOutputs && data.useChunks && !data.error)
  {
    if(chain.getStream ()->write("0\r\n\r\n", 5, &nbw))
      return 0;
  }

  if(!data.error)
    return 0;

  {
    ostringstream tmp;
    tmp << td->sentData;
    td->response.contentLength.assign(tmp.str()); 
  }
  chain.clearAllFilters(); 
  return 1;

#endif
}

/*!
 *Send a chunk of data to the client.
 */
int MsCgi::write(const char* data, u_long len, MsCgiData* mcd)
{
  if(mcd->error)
    return 1;

  if(!mcd->headerSent)
  {
    if(sendHeader(mcd))
       return 1;
  }

  if(mcd->onlyHeader)
    return 0;

  if(HttpDataHandler::appendDataToHTTPChannel(mcd->td, 
                                              (char*) data,
                                              len,
                                              &(mcd->td->outputData), 
                                              mcd->filtersChain,
                                              mcd->td->appendOutputs, 
                                              mcd->useChunks))
    return 1;

  mcd->td->sentData +=len;
  return 0;

}

/*!
 *Send the HTTP header.
 */
int MsCgi::sendHeader(MsCgiData* mcd)
{
  if(mcd->error)
    return 1;

  if(mcd->headerSent)
    return 0;

  if(!mcd->td->appendOutputs)
  {
    HttpThreadContext* td = mcd->td;
    char *buffer = td->secondaryBuffer->getBuffer();
    ConnectionPtr s = td->connection;

    u_long hdrLen = HttpHeaders::buildHTTPResponseHeader(buffer, &(td->response));
    if(td->connection->socket->send(buffer, hdrLen, 0) == SOCKET_ERROR)
      return 1;
  }

  mcd->headerSent = true;
  return 0;
}

/*!
 *Map the library in the application address space.
 *\param confFile The xml parser with configuration.
 */
int MsCgi::load()
{
  return 1;
}

/*!
 *Free the memory allocated by the MSCGI library.
 */
int MsCgi::unLoad()
 {
  return mscgiModule.close();
}
