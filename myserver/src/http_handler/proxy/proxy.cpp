/*
MyServer
Copyright (C) 2009 Free Software Foundation, Inc.
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
 *Forward the HTTP requesto to another server.
 *
 *\param td The HTTP thread context.
 *\param scriptpath Not used.
 *\param exec The remote server Url.
 *\param execute Not used.
 *\param onlyHeader Specify if send only the HTTP header.
 */
int Proxy::send (HttpThreadContext *td,
                 const char* scriptpath,
                 const char* exec,
                 bool execute,
                 bool onlyHeader)
{
  Url destUrl (exec, 80);
  Socket sock;
  FiltersChain chain;
  HttpRequestHeader req;
  u_long nbw;

  for (HashMap<string, HttpRequestHeader::Entry*>::Iterator it = td->request.begin ();
       it != td->request.end ();
       it++)
    {
      HttpRequestHeader::Entry *e = *it;
      req.setValue (e->name->c_str (), e->value->c_str ());
    }

  if (destUrl.getProtocol ().compare ("http") && destUrl.getProtocol ().compare ("HTTP"))
    {
      td->connection->host->warningsLogWrite ("Proxy: %s is not a known protocol", destUrl.getProtocol ());
      return 0;
    }

  req.ver.assign ("HTTP/1.1");
  req.cmd.assign (td->request.cmd);
  req.uri.assign ("/");
  req.uri.append (destUrl.getResource ());
  req.uri.append (td->pathInfo);
  req.setValue ("Connection", "Close");

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

	u_long hdrLen = HttpHeaders::buildHTTPRequestHeader (td->secondaryBuffer->getBuffer (),
                                                       &req);

  if (sock.connect (destUrl.getHost ().c_str (), destUrl.getPort ()))
    return td->http->raiseHTTPError (500);

  if (sock.write (td->secondaryBuffer->getBuffer (), hdrLen, &nbw))
    {
      sock.close ();
      return td->http->raiseHTTPError (500);
    }

 if (td->request.uriOptsPtr &&
      td->inputData.fastCopyToSocket (&sock, 0, td->secondaryBuffer, &nbw))
    {
      sock.close ();
      return td->http->raiseHTTPError (500);
    }

  chain.setProtocol(td->http);
  chain.setProtocolData(td);
  chain.setStream(td->connection->socket);

  if(td->mime && Server::getInstance()->getFiltersFactory()->chain(&chain,
                                                                   td->mime->filters,
                                                                   td->connection->socket,
                                                                   &nbw,
                                                                   1))
    {
      sock.close ();
      return td->http->raiseHTTPError (500);
    }

  int ret = flushToClient (td, sock, chain, onlyHeader);

  chain.clearAllFilters();
  sock.close ();
  req.free ();

  return ret;
}

/*!
 *Flush the server reply to the client.
 */
int Proxy::flushToClient (HttpThreadContext* td, Socket& client,
                          FiltersChain &out, int onlyHeader)
{
  u_long read = 0;
  u_long headerLength;
  int ret;
  u_long nbw;
  bool useChunks = false;
  bool keepalive = false;

  do
    {
      ret = client.recv (td->secondaryBuffer->getBuffer () + read,
                         td->secondaryBuffer->getRealLength () - read,
                         0,
                         td->http->getTimeout ());

      if (ret == 0)
        return td->http->raiseHTTPError (500);

      read += ret;

      ret = HttpHeaders::buildHTTPResponseHeaderStruct (td->secondaryBuffer->getBuffer (),
                                                        &td->response,
                                                        &headerLength);
    }
  while (ret == -1);

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
  tmp = td->response.getValue ("Transfer-Encoding", NULL);
  if (tmp)
    {
      hasTransferEncoding = false;
      transferEncoding.assign (*tmp);
    }
  

  if (useChunks)
    td->response.setValue ("Transfer-Encoding", "chunked");


  u_long hdrLen = HttpHeaders::buildHTTPResponseHeader (td->buffer->getBuffer (),
                                                        &td->response);

  if (out.getStream ()->write (td->buffer->getBuffer (),
                               hdrLen,
                               &nbw))
    return 0;

  if (onlyHeader)
    return keepalive;


  ret = readPayLoad (td,
                     &td->response,
                     &out,
                     &client,
                     td->secondaryBuffer->getBuffer() + headerLength,
                     read - headerLength,
                     td->http->getTimeout (),
                     useChunks,
                     keepalive,
                     hasTransferEncoding ? &transferEncoding : NULL);

  if (ret != -1)
    td->sentData += ret;

  return ret == -1 ? 0 : keepalive;
}

/*!
 *Forward the message payload to the client.
 *
 *\param td The current HTTP thread context.
 *\param res Response obtained by the server.
 *\param out The client chain.
 *\param initBuffer Initial read data.
 *\param initBufferSize Size of initial data.
 *\param timeout Connection timeout.
 *\param useChunks Use chunked transfer encoding
 *with the client.
 *\param keepalive The connection is keep-alive.
 *\param serverTransferEncoding Transfer-Encoding
 *used by the server.
 *
 *\return -1 on error.
 *\return Otherwise the number of bytes transmitted.
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
  if(serverTransferEncoding && serverTransferEncoding->compare("chunked"))
    return -1;

  if (res->contentLength.length ())
    {
      contentLength = atol (res->contentLength.c_str ());
      if (contentLength < 0)
        return -1;
    }

  length = contentLength;

  bufferDataSize = (td->nBytesToRead < td->buffer->getRealLength() - 1
                    ? td->nBytesToRead
                    : td->buffer->getRealLength() - 1 ) - td->nHeaderChars;

  /* If it is specified a transfer encoding read data using it.  */
  if(serverTransferEncoding)
  {
    if(!serverTransferEncoding->compare("chunked"))
    {
      for (;;)
        {
          if (HttpDataRead::readChunkedPostData (initBuffer,
                                                 &inPos,
                                                 initBufferSize,
                                                 client,
                                                 td->buffer->getBuffer(),
                                                 td->buffer->getRealLength() - 1,
                                                 &nbr,
                                                 timeout,
                                                 NULL,
                                                 1))
            return -1;

          if (!nbr)
            break;

          if (HttpDataHandler::appendDataToHTTPChannel (td,
                                                        td->buffer->getBuffer(),
                                                        nbr,
                                                        &(td->outputData),
                                                        out,
                                                        td->appendOutputs,
                                                        useChunks))
            return -1;

          written += nbr;
        }
    }
  }
  /* If it is not specified an encoding, read the data as it is.  */
  else for(;;)
  {

    u_long len = td->buffer->getRealLength() - 1;

    if (contentLength && length < len)
      len = length;

    if (len == 0)
      break;

    if(HttpDataRead::readContiguousPrimitivePostData (initBuffer,
                                                      &inPos,
                                                      initBufferSize,
                                                      client,
                                                      td->buffer->getBuffer(),
                                                      len,
                                                      &nbr,
                                                      timeout))
    {
      return -1;
    }

    if (contentLength == 0 && nbr == 0)
      break;

    if (length)
      length -= nbr;

    if (nbr && HttpDataHandler::appendDataToHTTPChannel (td,
                                                         td->buffer->getBuffer (),
                                                         nbr,
                                                         &(td->outputData),
                                                         out,
                                                         td->appendOutputs,
                                                         useChunks))
      return -1;

    written += nbr;

    if(contentLength && length == 0)
      break;
  }

  if(useChunks && out->getStream ()->write ("0\r\n\r\n", 5, &nbw))
    return -1;

  return written;
}
