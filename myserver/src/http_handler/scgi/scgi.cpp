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

#include <include/http_handler/scgi/scgi.h>
#include <include/protocol/http/env/env.h>
#include <include/protocol/http/http.h>
#include <include/base/string/stringutils.h>
#include <include/server/server.h>
#include <include/base/file/files_utility.h>
#include <include/base/string/securestr.h>

#include <string>
#include <sstream>
using namespace std;

#define SERVERS_DOMAIN "scgi"

/*! Is the scgi initialized?  */
int Scgi::initialized = 0;

/*! Process server manager.  */
ProcessServerManager *Scgi::processServerManager = 0;

/*!
  Entry-Point to manage a SCGI request.
*/
int Scgi::send (HttpThreadContext* td, const char* scriptpath,
               const char *cgipath, bool execute, bool onlyHeader)
{
  ScgiContext con;
  FiltersChain chain;

  string outDataPath;

  int sizeEnvString;
  ScgiServer* server = 0;
  ostringstream cmdLine;

  string moreArg;
  con.td = td;

  td->scriptPath.assign (scriptpath);

  try
    {
      string tmp;
      if (!(td->permissions & MYSERVER_PERMISSION_EXECUTE))
        return td->http->sendAuth ();

      tmp.assign (cgipath);
      FilesUtility::splitPath (tmp, td->cgiRoot, td->cgiFile);
      tmp.assign (scriptpath);
      FilesUtility::splitPath (tmp, td->scriptDir, td->scriptFile);

      chain.setStream (td->connection->socket);
      if (td->mime)
        {
          u_long nbw;
          if (td->mime)
            Server::getInstance ()->getFiltersFactory ()->chain (&chain,
                                                                 td->mime->filters,
                                                                 td->connection->socket,
                                                                 &nbw,
                                                                 1);
        }

      td->buffer->setLength (0);
      td->auxiliaryBuffer->getAt (0) = '\0';

      {
        /* Do not modify the text between " and ".  */
        int i;
        int subString = cgipath[0] == '"';
        int len = strlen (cgipath);
        string tmpCgiPath;
        for (i = 1; i < len; i++)
          {
            if (!subString && cgipath[i]==' ')
              break;
            if (cgipath[i] == '"' && cgipath[i - 1] != '\\')
              subString = !subString;
          }

        /*
          Save the cgi path and the possible arguments.
          the (x < len) case is when additional arguments are specified.
          If the cgipath is enclosed between " and " do not consider them
          when splitting directory and file name.
        */
        if (len)
          {
            if (i < len)
              {
                string tmpString (cgipath);
                int begin = tmpString[0]=='"' ? 1: 0;
                int end = tmpString[i] == '"' ? i - 1: i;
                tmpCgiPath.assign (tmpString.substr (begin, end - begin));
                moreArg.assign (tmpString.substr (i, len - 1));
              }
            else
              {
                int begin = (cgipath[0] == '"') ? 1 : 0;
                int end   = (cgipath[len] == '"') ? len - 1 : len;
                tmpCgiPath.assign (&cgipath[begin], end - begin);
                moreArg.assign ("");
              }
            FilesUtility::splitPath (tmpCgiPath, td->cgiRoot, td->cgiFile);
          }
        tmpCgiPath.assign (scriptpath);
        FilesUtility::splitPath (tmpCgiPath, td->scriptDir, td->scriptFile);
      }

      if (execute)
        {
          if (cgipath && strlen (cgipath))
            {
#ifdef WIN32
              {
                int x;
                string cgipathString (cgipath);
                int len = strlen (cgipath);
                int subString = cgipath[0] == '"';

                cmdLine << "\"" << td->cgiRoot << "/" << td->cgiFile << "\" "
                        << moreArg << " \"" <<  td->filenamePath << "\"";
              }
#else
              cmdLine << cgipath << " " << td->filenamePath;
#endif
            }/*if (execute).  */
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

      Env::buildEnvironmentString (td, td->buffer->getBuffer ());
      sizeEnvString = buildScgiEnvironmentString (td,td->buffer->getBuffer (),
                                                  td->auxiliaryBuffer->getBuffer ());
      if (sizeEnvString == -1)
        {
          td->connection->host->warningsLogWrite (_("SCGI: internal error"));
          chain.clearAllFilters ();
          return td->http->raiseHTTPError (500);
        }
      td->inputData.close ();
      td->inputData.openFile (td->inputDataPath, File::READ
                              | File::FILE_OPEN_ALWAYS | File::NO_INHERIT);

      try
        {
          server = connect (&con, cmdLine.str ().c_str ());
        }
      catch (exception & e)
        {
          td->connection->host->warningsLogWrite
            (_E ("SCGI: error connecting to the process %s"),
             cmdLine.str ().c_str (), &e);
          chain.clearAllFilters ();
          return td->http->raiseHTTPError (500);
        }

      sendNetString (&con, td->auxiliaryBuffer->getBuffer (), sizeEnvString);
      if (td->request.contentLength.size ()
          && !td->request.contentLength.compare ("0"))
        sendPostData (&con);

      sendResponse (&con, onlyHeader, &chain);

      chain.clearAllFilters ();
      con.tempOut.close ();
      con.sock.close ();
    }
  catch (exception & e)
    {
      td->connection->host->warningsLogWrite (_E ("SCGI: internal error"), &e);
      chain.clearAllFilters ();
      return td->http->raiseHTTPError (500);
    }

  return HttpDataHandler::RET_OK;
}


/*!
  Send the response to the client.
 */
int Scgi::sendResponse (ScgiContext* ctx, int onlyHeader, FiltersChain* chain)
{
  clock_t initialTicks = getTicks ();
  bool useChunks = false;
  bool keepalive = false;
  u_long read = 0;
  u_long headerSize = 0;
  u_long tmpHeaderSize = 0;
  u_long nbw, nbr;
  u_long sentData = 0;
  HttpThreadContext* td = ctx->td;

  checkDataChunks (td, &keepalive, &useChunks);

  for (;;)
    {
      while (!ctx->sock.bytesToRead ())
        {
          if ((getTicks () - initialTicks) > (u_long) td->http->getTimeout ())
            break;
          Thread::wait (1);
        }

      if (!ctx->sock.bytesToRead ())
        return HttpDataHandler::RET_FAILURE;

      nbr = ctx->sock.recv (td->auxiliaryBuffer->getBuffer () + read,
                            td->auxiliaryBuffer->getRealLength () - read,
                            td->http->getTimeout ());

      read += nbr;

      for (tmpHeaderSize = (tmpHeaderSize > 3)
             ? tmpHeaderSize - 4 : tmpHeaderSize;
           tmpHeaderSize < read - 4; tmpHeaderSize++)
        if ((td->auxiliaryBuffer->getBuffer ()[tmpHeaderSize] == '\r')
            && (td->auxiliaryBuffer->getBuffer ()[tmpHeaderSize + 1] == '\n')
            && (td->auxiliaryBuffer->getBuffer ()[tmpHeaderSize + 2] == '\r')
            && (td->auxiliaryBuffer->getBuffer ()[tmpHeaderSize + 3] == '\n'))
          {
            headerSize = tmpHeaderSize + 4;
            break;
          }

      if (headerSize)
        break;
    }

  if (headerSize)
    HttpHeaders::buildHTTPResponseHeaderStruct (td->auxiliaryBuffer->getBuffer (),
                                                &td->response,
                                                &(td->nBytesToRead));

  HttpHeaders::sendHeader (td->response, *td->connection->socket,
                           *td->auxiliaryBuffer, td);

  if (onlyHeader)
    return HttpDataHandler::RET_OK;

  if (read - headerSize)
    appendDataToHTTPChannel (td, td->auxiliaryBuffer->getBuffer () + headerSize,
                             read - headerSize,
                             &(td->outputData),
                             chain,
                             td->appendOutputs,
                             useChunks);

  sentData += read - headerSize;

  if (td->response.getStatusType () == HttpResponseHeader::SUCCESSFUL)
    {
      for (;;)
        {
          nbr = ctx->sock.recv (td->auxiliaryBuffer->getBuffer (),
                                td->auxiliaryBuffer->getRealLength (),
                                0);

          if (!nbr || (nbr == (u_long)-1))
            break;

          appendDataToHTTPChannel (td, td->auxiliaryBuffer->getBuffer (),
                                   nbr,
                                   &(td->outputData),
                                   chain,
                                   td->appendOutputs,
                                   useChunks);

          sentData += nbr;
        }

      if (!td->appendOutputs && useChunks)
        chain->getStream ()->write ("0\r\n\r\n", 5, &nbw);
    }
  /* For logging activity.  */
  td->sentData += sentData;

  return HttpDataHandler::RET_OK;
}

/*!
  Send a netstring to the SCGI server.
 */
int Scgi::sendNetString (ScgiContext* ctx, const char* data, int len)
{
  char header[7];
  int headerLen = sprintf (header, "%i:", len);

  ctx->sock.send (header, headerLen, 0);
  ctx->sock.send (data, len, 0);
  ctx->sock.send (",", 1, 0);

  return 0;
}

/*!
  Send the post data to the SCGI server.
 */
int Scgi::sendPostData (ScgiContext* ctx)
{
  u_long nbr;
  do
    {
      ctx->td->inputData.read (ctx->td->auxiliaryBuffer->getBuffer (),
                               ctx->td->auxiliaryBuffer->getRealLength (),
                               &nbr);

      if (nbr)
        ctx->sock.send (ctx->td->auxiliaryBuffer->getBuffer (), nbr, 0);
    }
  while (nbr);

  return 0;
}

/*!
  Trasform from a standard environment string to the SCGI environment
  string.
 */
int Scgi::buildScgiEnvironmentString (HttpThreadContext* td, char* src,
                                      char* dest)
{
  char *ptr = dest;
  char *sptr = src;
  char varName[100];
  char varValue[2500];

  ptr += myserver_strlcpy (ptr, "CONTENT_LENGTH", 15);
  *ptr++ = '\0';

  if ( td->request.contentLength.size ())
    ptr += myserver_strlcpy (ptr, td->request.contentLength.c_str (),
                             td->request.contentLength.size () + 1);
  else
    *ptr++ = '0';

  *ptr++ = '\0';

  ptr += myserver_strlcpy (ptr, "SCGI", 5);
  *ptr++ = '\0';

  ptr += myserver_strlcpy (ptr, "1", 2);
  *ptr++ = '\0';

  for (;;)
    {
      int i;
      int max = 100;
      int varNameLen;
      int varValueLen;

      varNameLen = varValueLen = 0;
      varName[0] = '\0';
      varValue[0] = '\0';

      while (*sptr == '\0')
        sptr++;

      while ((--max) && *sptr != '=')
        {
          varName[varNameLen++] = *sptr++;
          varName[varNameLen] = '\0';
        }
      if (max == 0)
        return -1;
      sptr++;
      max = 2500;
      while ((--max) && *sptr != '\0')
        {
          varValue[varValueLen++] = *sptr++;
          varValue[varValueLen] = '\0';
        }

      if (max == 0)
        return -1;

      if (! strcasecmp (varName, "CONTENT_LENGTH")
          || ! strcasecmp (varName, "SCGI")
          || !varNameLen || !varValueLen)
        continue;

      for (i = 0; i < varNameLen; i++)
        *ptr++ = varName[i];
      *ptr++ = '\0';

      for (i = 0; i < varValueLen; i++)
        *ptr++ = varValue[i];
      *ptr++ = '\0';

      if (*(++sptr) == '\0')
        break;
    }
  return static_cast<int> (ptr - dest);
}

/*!
  Constructor for the FASTCGI class
 */
Scgi::Scgi ()
{

}

/*!
  Initialize the SCGI protocol implementation
 */
int Scgi::load ()
{
  if (initialized)
    return 1;
  initialized = 1;
  processServerManager = Server::getInstance ()->getProcessServerManager ();
  processServerManager->createDomain (SERVERS_DOMAIN);
  return 0;
}

/*!
  Clean the memory and the processes occuped by the FastCGI servers
 */
int Scgi::unLoad ()
{
  initialized = 0;
  return 0;
}

/*!
  Return the the running server specified by path.
  If the server is not running returns 0.
 */
ScgiServer* Scgi::isScgiServerRunning (const char* path)
{
  return processServerManager->getServer (SERVERS_DOMAIN, path);
}


/*!
  Get a connection to the FastCGI server.
 */
ScgiServer* Scgi::connect (ScgiContext* con, const char* path)
{
  ScgiServer* server = runScgiServer (con, path);
  /*!
   *If we find a valid server try the connection to it.
   */
  if (server)
    {
      int ret = processServerManager->connect (&(con->sock), server);

      if (ret == -1)
        return 0;
    }
  return server;
}

/*!
  Run the SCGI server.
  If the path starts with a @ character, the path is handled as a
  remote server.
 */
ScgiServer* Scgi::runScgiServer (ScgiContext* context,
                                 const char* path)
{
  /* This method needs a better home (and maybe better code).
    Compute a simple hash from the IP address.  */
  const char *ip = context->td->connection->getIpAddr ();
  int seed = 13;
  for (const char *c = ip; *c; c++)
    seed = *c * 21 + seed;

  ScgiServer* server =  processServerManager->getServer (SERVERS_DOMAIN,
                                                         path,
                                                         seed);
  if (server)
    return server;

  /* If the path starts with @ then the server is remote.  */
  if (path[0] == '@')
    {
      int i = 1;
      char host[128];
      char port[6];

      while (path[i] && path[i] != ':')
        i++;

      myserver_strlcpy (host, &path[1], min (128, i));

      myserver_strlcpy (port, &path[i + 1], 6);

      return processServerManager->addRemoteServer (SERVERS_DOMAIN, path,
                                                    host, atoi (port));
    }

  return processServerManager->runAndAddServer (SERVERS_DOMAIN, path);
}
