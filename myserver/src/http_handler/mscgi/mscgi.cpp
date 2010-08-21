/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008, 2009, 2010 Free
  Software Foundation, Inc.
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

#include "myserver.h"
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
  Entry-point for the MyServer CGI; differently from standard CGI this doesn't
  need a new process to run making it faster.
  \param td The HTTP thread context.
  \param exec The script path.
  \param cmdLine The command line.
  \param execute Specify if the script has to be executed.
  \param onlyHeader Specify if send only the HTTP header.
 */
int MsCgi::send (HttpThreadContext* td, const char* exec, const char* cmdLine,
                 bool /*execute*/, bool onlyHeader)
{
  /*
    This is the code for manage a .mscgi file.
    This files differently from standard CGI don't need a new process to run
    but are allocated in the caller process virtual space.
    Usually these files are faster than standard CGI.
    Actually myServerCGI (.mscgi) is only at an alpha status.
   */
  ostringstream tmpStream;
  string outDataPath;
  FiltersChain chain;
  size_t nbw;
  DynamicLibrary hinstLib;
  CGIMAIN ProcMain = 0;

#if !(WIN32 | HAVE_DL)
  return td->http->raiseHTTPError (501);
#endif

  data.envString = td->request.uriOptsPtr ?
    td->request.uriOptsPtr : (char*) td->buffer->getBuffer ();

  data.td = td;
  data.errorPage = 0;
  data.server = Server::getInstance ();
  data.mscgi = this;
  data.useChunks = false;
  data.onlyHeader = onlyHeader ? true : false;
  data.error = false;
  data.filtersChain = &chain;
  data.headerSent = false;
  data.keepAlive = false;
  data.useChunks = false;

  if (!(td->permissions & MYSERVER_PERMISSION_EXECUTE))
    return td->http->sendAuth ();

  try
    {
      td->scriptPath.assign (exec);

      {
        string tmp;
        tmp.assign (exec);
        FilesUtility::splitPath (tmp, td->cgiRoot, td->cgiFile);
        FilesUtility::splitPath (exec, td->scriptDir, td->scriptFile);
      }

      Env::buildEnvironmentString (td,data.envString);
      chain.setStream (td->connection->socket);

      if (td->mime)
        Server::getInstance ()->getFiltersFactory ()->chain (&chain,
                                                             td->mime->filters,
                                                             td->connection->socket,
                                                             &nbw, 1);

      checkDataChunks (td, &(data.keepAlive), &(data.useChunks));
      try
        {
          hinstLib.loadLibrary (exec, 0);
        }
      catch (exception & e)
        {
          td->connection->host->warningsLogWrite (_E ("MSCGI: cannot load %s"),
                                                  exec, &e);
          chain.clearAllFilters ();
          /* Internal server error.  */
          return td->http->raiseHTTPError (500);
        }

      td->auxiliaryBuffer->getAt (0) = '\0';
      ProcMain = (CGIMAIN) hinstLib.getProc ( "myserver_main");
      if (ProcMain)
        (ProcMain)(td->request.uriOpts.c_str (), &data);
      else
        {
          td->connection->host->warningsLogWrite
            (_("MSCGI: cannot find entry-point for %s"),exec);
          return td->http->raiseHTTPError (500);
        }
      hinstLib.close ();

      if (data.errorPage)
        {
          chain.clearAllFilters ();
          return td->http->raiseHTTPError (data.errorPage);
        }

      if (!td->appendOutputs && data.useChunks && !data.error)
        chain.getStream ()->write ("0\r\n\r\n", 5, &nbw);

      if (!data.error)
        return HttpDataHandler::RET_FAILURE;

      ostringstream tmp;
      tmp << td->sentData;
      td->response.contentLength.assign (tmp.str ());

      chain.clearAllFilters ();

    }
  catch (exception & e)
    {
      td->connection->host->warningsLogWrite (_E ("Msgi: internal error"), &e);
      return td->http->raiseHTTPError (500);
    }

  return HttpDataHandler::RET_OK;
}

/*!
  Send a chunk of data to the client.
 */
int MsCgi::write (const char* data, u_long len, MsCgiData* mcd)
{
  if (mcd->error)
    return 1;

  if (!mcd->headerSent && sendHeader (mcd))
    return 1;

  if (mcd->onlyHeader)
    return 0;

  HttpDataHandler::appendDataToHTTPChannel (mcd->td,
                                            (char*) data,
                                            len,
                                            &(mcd->td->outputData),
                                            mcd->filtersChain,
                                            mcd->td->appendOutputs,
                                            mcd->useChunks);

  mcd->td->sentData +=len;
  return 0;
}

/*!
  Send the HTTP header.
 */
int MsCgi::sendHeader (MsCgiData* mcd)
{
  HttpThreadContext* td = mcd->td;

  if (mcd->error)
    return 1;

  if (mcd->headerSent)
    return 0;

  HttpHeaders::sendHeader (td->response, *td->connection->socket,
                           *td->auxiliaryBuffer, td);

  mcd->headerSent = true;
  return 0;
}

/*!
  Map the library in the application address space.
  \param confFile The xml parser with configuration.
 */
int MsCgi::load ()
{
  return 1;
}

/*!
  Free the memory allocated by the MSCGI library.
 */
int MsCgi::unLoad ()
{
  return mscgiModule.close ();
}
