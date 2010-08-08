/*
  MyServer
  Copyright (C) 2009, 2010 Free Software Foundation, Inc.
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
#include <include/http_handler/proxy/proxy.h>

#include <include/protocol/http/http_thread_context.h>
#include <include/protocol/http/http.h>
#include <include/protocol/url.h>
#include <include/base/socket/socket.h>
#include <include/protocol/http/http_request.h>
#include <include/protocol/http/http_response.h>
#include <include/protocol/http/http_data_read.h>
#include <include/protocol/http/http_data_handler.h>
#include <include/protocol/http/http_headers.h>
#include <include/filter/filters_chain.h>

#include <sstream>

/*
  Forward the HTTP requesto to another server.

  \param td The HTTP thread context.
  \param scriptpath Not used.
  \param exec The remote server Url.
  \param execute Not used.
  \param onlyHeader Specify if send only the HTTP header.
 */
int Proxy::send (HttpThreadContext *td, const char* scriptpath,
                 const char* exec, bool execute, bool onlyHeader)
{
  Url destUrl (exec, 80);
  ConnectionPtr con = NULL;
  Socket *sock;
  FiltersChain chain;
  HttpRequestHeader req;
  u_long nbw;

  for (HashMap<string, HttpRequestHeader::Entry*>::Iterator it =
         td->request.begin (); it != td->request.end (); it++)
    {
      HttpRequestHeader::Entry *e = *it;
      req.setValue (e->name.c_str (), e->value.c_str ());
    }

  if (destUrl.getProtocol ().compare ("http")
      && destUrl.getProtocol ().compare ("HTTP"))
    {
      td->connection->host->warningsLogWrite
        ("Proxy: %s is not a supported protocol",
         destUrl.getProtocol ().c_str ());
      return td->http->raiseHTTPError (500);
    }

  try
    {
      req.ver.assign ("HTTP/1.1");
      req.cmd.assign (td->request.cmd);
      req.uri.assign ("/");
      req.uri.append (destUrl.getResource ());
      req.uri.append (td->pathInfo);
      req.setValue ("Connection", "keep-alive");
      if (td->request.uriOptsPtr)
        {
          char buffer[32];
          u_long size = td->inputData.getFileSize ();
          sprintf (buffer, "%u", size);
          req.setValue ("Content-Length", buffer);
        }

      ostringstream host;
      host << destUrl.getHost ();
      if (destUrl.getPort () != 80 )
        host << ":" << destUrl.getPort ();

      req.setValue ("Host", host.str ().c_str ());

      string xForwardedFor;
      td->request.getValue ("X-Forwarded-For", &xForwardedFor);
      if (xForwardedFor.size ())
        xForwardedFor.append (", ");
      xForwardedFor.append (td->connection->getIpAddr ());
      req.setValue ("X-Forwarded-For", xForwardedFor.c_str ());

      con = getConnection (destUrl.getHost ().c_str (), destUrl.getPort ());
      if (! con)
        return td->http->raiseHTTPError (500);

      sock = con->socket;

      u_long hdrLen =
        HttpHeaders::buildHTTPRequestHeader (td->auxiliaryBuffer->getBuffer (),
                                             &req);


      sock->write (td->auxiliaryBuffer->getBuffer (), hdrLen, &nbw);

      if (td->request.uriOptsPtr)
        td->inputData.fastCopyToSocket (sock, 0, td->auxiliaryBuffer, &nbw);

      chain.setStream (td->connection->socket);
      if (td->mime)
        Server::getInstance ()->getFiltersFactory ()->chain (&chain,
                                                             td->mime->filters,
                                                             td->connection->socket,
                                                             &nbw,
                                                             1);


      flushToClient (td, *sock, chain, onlyHeader);

      chain.clearAllFilters ();
      addConnection (con, destUrl.getHost ().c_str (), destUrl.getPort ());
      req.free ();
    }
  catch (exception & e)
    {
      if (con)
        delete con;
      chain.clearAllFilters ();
      return td->http->raiseHTTPError (500);
    }

  return HttpDataHandler::RET_OK;
}

/*!
  Flush the server reply to the client.
 */
int Proxy::flushToClient (HttpThreadContext* td, Socket& client,
                          FiltersChain &out, bool onlyHeader)
{
  u_long read = 0;
  u_long headerLength;
  int ret;
  u_long nbw;
  bool useChunks = false;
  bool keepalive = false;

  do
    {
      ret = client.recv (td->auxiliaryBuffer->getBuffer () + read,
                         td->auxiliaryBuffer->getRealLength () - read,
                         0,
                         td->http->getTimeout ());

      read += ret;

      if (HttpHeaders::buildHTTPResponseHeaderStruct
          (td->auxiliaryBuffer->getBuffer (), &td->response, &headerLength))
        break;
    }
  while (ret);

  checkDataChunks (td, &keepalive, &useChunks);

  string *tmp = td->request.getValue ("Host", NULL);
  const char *via = tmp ? tmp->c_str () : td->connection->getLocalIpAddr ();

  tmp = td->response.getValue ("Via", NULL);
  if (tmp)
    {
      tmp->append (", ");
      tmp->append (via);
      td->response.setValue ("Via", tmp->c_str ());
    }
  else
    td->response.setValue ("Via", via);

  string transferEncoding;
  bool hasTransferEncoding = false;
  tmp = td->response.getValue ("Transfer-encoding", NULL);
  if (tmp)
    {
      hasTransferEncoding = false;
      transferEncoding.assign (*tmp);
    }

  if (useChunks)
    td->response.setValue ("Transfer-encoding", "chunked");


  u_long hdrLen = HttpHeaders::buildHTTPResponseHeader (td->buffer->getBuffer (),
                                                        &td->response);

  out.getStream ()->write (td->buffer->getBuffer (), hdrLen, &nbw);

  if (onlyHeader)
    return HttpDataHandler::RET_OK;

  readPayLoad (td, &td->response, &out, &client,
               td->auxiliaryBuffer->getBuffer () + headerLength,
               read - headerLength, td->http->getTimeout (),
               useChunks, keepalive,
               hasTransferEncoding ? &transferEncoding : NULL);

  td->sentData += ret;
  return HttpDataHandler::RET_OK;
}

/*!
  Forward the message payload to the client.

  \param td The current HTTP thread context.
  \param res Response obtained by the server.
  \param out The client chain.
  \param initBuffer Initial read data.
  \param initBufferSize Size of initial data.
  \param timeout Connection timeout.
  \param useChunks Use chunked transfer encoding
  with the client.
  \param keepalive The connection is keep-alive.
  \param serverTransferEncoding Transfer-encoding
  used by the server.

  \return -1 on error.
  \return Otherwise the number of bytes transmitted.
 */
int Proxy::readPayLoad (HttpThreadContext* td,
                        HttpResponseHeader* res,
                        FiltersChain *out,
                        Socket *client,
                        const char *initBuffer,
                        u_long initBufferSize,
                        int timeout,
                        bool useChunks,
                        bool keepalive,
                        string *serverTransferEncoding)
{
  u_long contentLength = 0;

  u_long nbr = 0, nbw = 0, length = 0, inPos = 0;
  u_long bufferDataSize = 0;
  u_long written = 0;

  /* Only the chunked transfer encoding is supported.  */
  if (serverTransferEncoding && serverTransferEncoding->compare ("chunked"))
    return HttpDataHandler::RET_FAILURE;

  if (res->contentLength.length ())
    {
      contentLength = atol (res->contentLength.c_str ());
      if (contentLength < 0)
        return HttpDataHandler::RET_FAILURE;
    }

  length = contentLength;

  bufferDataSize = (td->nBytesToRead < td->buffer->getRealLength () - 1
                    ? td->nBytesToRead
                    : td->buffer->getRealLength () - 1 ) - td->nHeaderChars;

  /* If it is specified a transfer encoding read data using it.  */
  if (serverTransferEncoding)
    {
      if (!serverTransferEncoding->compare ("chunked"))
        {
          for (;;)
            {
              HttpDataRead::readChunkedPostData (initBuffer, &inPos, initBufferSize,
                                                 client, td->buffer->getBuffer (),
                                                 td->buffer->getRealLength () - 1,
                                                 &nbr, timeout, NULL, 1);

              if (!nbr)
                break;

              HttpDataHandler::appendDataToHTTPChannel (td,
                                                        td->buffer->getBuffer (),
                                                        nbr, &(td->outputData),
                                                        out, td->appendOutputs,
                                                        useChunks);
              written += nbr;
            }
        }
    }
  else
    {
      /* If it is not specified an encoding, read the data as it is.  */
      for (;;)
        {
          u_long len = td->buffer->getRealLength () - 1;

          if (contentLength && length < len)
            len = length;

          if (len == 0)
            break;

          HttpDataRead::readContiguousPrimitivePostData (initBuffer, &inPos,
                                                         initBufferSize, client,
                                                         td->buffer->getBuffer (),
                                                         len, &nbr, timeout);

          if (contentLength == 0 && nbr == 0)
            break;

          if (length)
            length -= nbr;

          HttpDataHandler::appendDataToHTTPChannel (td, td->buffer->getBuffer (),
                                                    nbr, &(td->outputData), out,
                                                    td->appendOutputs, useChunks);
          written += nbr;

          if (contentLength && length == 0)
            break;
        }
    }
  if (useChunks)
    out->getStream ()->write ("0\r\n\r\n", 5, &nbw);

  return HttpDataHandler::RET_OK;
}

int Proxy::load ()
{
  const char *data
    = Server::getInstance ()->getData ("http.proxy.connections.keepalive",
                                       "64");
  maxKeepAliveConnections = atoi (data);
}

ConnectionPtr Proxy::getConnection (const char *host, u_short port)
{
  ConnectionPtr ret = NULL;

  connectionsLock.lock ();
  try
    {
      vector<ConnectionRecord>::iterator it = connections.begin ();
      for (; it != connections.end (); it++)
        {
          if ((*it).port == port && (*it).host.compare (host) == 0)
            {
              ret = (*it).connection;
              connections.erase (it);
              break;
            }
        }
      connectionsLock.unlock ();
      if (ret)
        return ret;
    }
  catch (...)
    {
      connectionsLock.unlock ();
      throw;
    }

  ret = new Connection ();
  ret->socket = new Socket ();
  try
    {
      ret->socket->socket (AF_INET, SOCK_STREAM, 0);
      ret->socket->connect (host, port);
    }
  catch (...)
    {
      delete ret;
      throw;
    }

  return ret;
}

void Proxy::proxySchedulerHandler (void *p, Connection *c, int event)
{
  Proxy *proxy = (Proxy *) p;
  proxy->removeConnection (c);
}

void Proxy::removeConnection (Connection *c)
{
  connectionsLock.lock ();
  try
    {
      vector<ConnectionRecord>::iterator it = connections.begin ();
      for (; it != connections.end (); it++)
        {
          /* Remove only if it is found in the buffer.  Otherwise it is
             used at this time and we can't remove it.  */
          if ((*it).connection == c)
            {
              connections.erase (it);
              delete c;
              break;
            }
        }

      connectionsLock.unlock ();
    }
  catch (...)
    {
      connectionsLock.unlock ();
    }
}

void Proxy::addConnection (ConnectionPtr con, const char *host, u_short port)
{
  if (connections.size () >= maxKeepAliveConnections)
    {
      delete con;
      return;
    }

  con->setSchedulerHandler (Proxy::proxySchedulerHandler, this);

  ConnectionsScheduler *cs  = Server::getInstance ()->getConnectionsScheduler ();
  cs->addNewWaitingConnection (con);

  ConnectionRecord cr;
  cr.host = host;
  cr.port = port;
  cr.connection = con;

  connections.push_back (cr);
}
