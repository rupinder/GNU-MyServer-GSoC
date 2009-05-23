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
#include <include/protocol/http/http_data_handler.h>
#include <include/protocol/http/http_headers.h>
#include <include/filter/filters_chain.h>

#include <sstream>

int Proxy::timeout = MYSERVER_SEC (10);

/*
 *Forward the HTTP requesto to another server.
 *
 *\param td The HTTP thread context.
 *\param s A pointer to the connection structure.
 *\param scriptpath Not used.
 *\param exec The remote server Url.
 *\param execute Not used.
 *\param onlyHeader Specify if send only the HTTP header.
 */
int Proxy::send (HttpThreadContext *td, ConnectionPtr s,
                 const char* scriptpath,
                 const char* exec,
                 int execute,
                 int onlyHeader)
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

  if (destUrl.getProtocol ().compare ("HTTP"))
    {
      ostringstream msg;
      msg << "Proxy: " << destUrl.getProtocol () << " is not known.";
      td->connection->host->warningsLogWrite (msg.str ().c_str ());
      return 0;
    }

  /*Use HTTP/1.0 until we accept chunks from clients.  */
  req.ver.assign ("HTTP/1.0");
  req.cmd.assign ("GET");
  req.uri.assign ("/");
  req.uri.append (destUrl.getResource ());
  req.uri.append (td->pathInfo);
  req.setValue ("Connection", "Close");

  ostringstream host;
  host << destUrl.getHost ();
  if (destUrl.getPort () != 80 )
    host << ":" << destUrl.getPort ();

  req.setValue ("Host", host.str ().c_str ());

	HttpHeaders::buildHTTPRequestHeader (td->secondaryBuffer->getBuffer (),
                                       &req);

  if (sock.connect (destUrl.getHost ().c_str (), destUrl.getPort ()))
    return td->http->raiseHTTPError (500);

  if (td->request.uriOptsPtr &&
      td->inputData.fastCopyToSocket (&sock, 0, td->secondaryBuffer, &nbw))
    {
      sock.close ();
      return td->http->raiseHTTPError (500);
    }

  if (sock.write (td->secondaryBuffer->getBuffer (),
                  strlen (td->secondaryBuffer->getBuffer ()), &nbw))
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
                         timeout);

      if (ret == 0)
        return td->http->raiseHTTPError (500);

      read += ret;

      ret = HttpHeaders::buildHTTPResponseHeaderStruct (td->secondaryBuffer->getBuffer (),
                                                        &td->response,
                                                        &headerLength);
    }
  while (ret == -1);

  checkDataChunks (td, &keepalive, &useChunks);


  HttpHeaders::buildHTTPResponseHeader (td->buffer->getBuffer (),
                                        &td->response);

  if (out.getStream ()->write (td->buffer->getBuffer (),
                               strlen (td->buffer->getBuffer ()),
                               &nbw))
    return 0;

  if (onlyHeader)
    return keepalive;

  if (read - headerLength)
    {
      if (HttpDataHandler::appendDataToHTTPChannel(td,
                                                   td->secondaryBuffer->getBuffer() +
                                                     headerLength,
                                                   read - headerLength,
                                                   &(td->outputData),
                                                   &out,
                                                   td->appendOutputs,
                                                   useChunks))
        return 0;

      td->sentData += read - headerLength;
    }

  while (ret = client.recv (td->secondaryBuffer->getBuffer (),
                            td->secondaryBuffer->getRealLength (),
                            0,
                            timeout))
    {

      if (ret == -1)
        break;

      if (HttpDataHandler::appendDataToHTTPChannel(td,
                                                   td->secondaryBuffer->getBuffer(),
                                                   ret,
                                                   &(td->outputData),
                                                   &out,
                                                   td->appendOutputs,
                                                   useChunks))
        return 0;

      td->sentData += ret;
    }


  if(useChunks && out.write ("0\r\n\r\n", 5, &nbw))
    {
      return 0;
    }

  return keepalive;
}

/*!
 *Set the CGI timeout for the new processes.
 *\param nt The new timeout value.
 */
void Proxy::setTimeout(int nt)
{
   timeout = nt;
}

/*!
 *Get the timeout value for HTTP requests.
 */
int Proxy::getTimeout()
{
  return timeout;
}
