/*
  MyServer
  Copyright (C) 2005, 2007, 2008, 2009 Free Software Foundation, Inc.
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

#include <include/protocol/http/http_headers.h>
#include <include/protocol/http/http.h>
#include <include/protocol/http/http_data_handler.h>

/*!
 * Send a file to the client using the HTTP protocol.
 */
int
HttpDataHandler::send (HttpThreadContext* td,
                       const char* /*filenamePath*/, const char* /*exec*/,
                       bool /*execute*/, bool /*onlyHeader*/)
{
  td->connection->host->warningsLogWrite (_("HttpDataHandler: using the base interface!"));
  return td->http->raiseHTTPError (500);
}

/*!
 * Constructor for the class.
 */
HttpDataHandler::HttpDataHandler () { }

/*!
 * Destroy the object.
 */
HttpDataHandler::~HttpDataHandler () { }

/*!
 * Load the static elements.
 */
int HttpDataHandler::load ()
{
  return 0;
}

/*!
 * Unload the static elements.
 */
int HttpDataHandler::unLoad ()
{
  return 0;
}

/*!
 * Send data over the HTTP channel.  This method considers modifier filters.
 * in the filters chain.
 * \param td The HTTP thread context.
 * \param buffer Data to send.
 * \param size Size of the buffer.
 * \param appendFile The file where append if in append mode.
 * \param chain Where send data if not append.
 * \param append Append to the file?
 * \param useChunks Can we use HTTP chunks to send data?
 * \param realBufferSize The real dimension of the buffer that can be
 * used by this method.
 * \param tmpStream A support on memory read/write stream used
 * internally by the function.
 */
int HttpDataHandler::appendDataToHTTPChannel (HttpThreadContext* td,
                                              char* buffer,
                                              u_long size,
                                              File* appendFile,
                                              FiltersChain* chain,
                                              bool append,
                                              bool useChunks,
                                              u_long realBufferSize,
                                              MemoryStream *tmpStream)
{
  u_long nbr, nbw;
  Stream *oldStream = chain->getStream ();

  if (!chain->hasModifiersFilters ())
    return appendDataToHTTPChannel (td, buffer, size, appendFile, chain, append,
                                    useChunks);

  /*
   * This function can't append directly to the chain because we can't
   * know in advance the data chunk size.  Therefore we replace the
   * final stream with a memory buffer and write there the final data
   * chunk content, finally we read from it and send directly to the
   * original stream.
   */
  chain->setStream (tmpStream);

  if (chain->write (buffer, size, &nbw))
    {
      td->connection->host->warningsLogWrite (_("Http: internal error"));
      return 1;
    }


  if (tmpStream->read (buffer, realBufferSize, &nbr))
    {
      td->connection->host->warningsLogWrite (_("Http: internal error"));
      return 1;
    }

  chain->setStream (oldStream);

  /*
   *Use of chain->getStream () is needed to write directly on the
   *final stream.
   */
  return appendDataToHTTPChannel (td, buffer, nbr, appendFile, chain, append,
                                  useChunks);
}

/*!
 * Send raw data over the HTTP channel.  It doesn't consider modifier filters.
 * Return zero on success.
 * \param td The HTTP thread context.
 * \param buffer Data to send.
 * \param size Size of the buffer.
 * \param appendFile The file where append if in append mode.
 * \param chain Where send data if not append.
 * \param append Append to the file?
 * \param useChunks Can we use HTTP chunks to send data?
 */
int
HttpDataHandler::appendDataToHTTPChannel (HttpThreadContext* td, char* buffer,
                           u_long size, File* appendFile, FiltersChain* chain,
                                          bool append, bool useChunks)
{
  u_long nbw;

  if (chain->hasModifiersFilters ())
    {
      td->connection->host->warningsLogWrite (_("Http: internal error"));
      return 1;
    }

  if (append)
    return appendFile->writeToFile (buffer, size, &nbw);
  else
    {
      if (useChunks)
        {
          ostringstream chunkHeader;
          u_long flushNbw = 0;
          chunkHeader << hex << size << "\r\n";

          if (chain->flush (&flushNbw))
            {
              td->connection->host->warningsLogWrite (_("Http: internal error"));
              return 1;
            }

          if (chain->getStream ()->write (chunkHeader.str ().c_str (),
                                          chunkHeader.str ().length (), &nbw))
            {
              td->connection->host->warningsLogWrite (_("Http: internal error"));
              return 1;
            }
        }

      if (size && chain->write (buffer, size, &nbw))
        {
          td->connection->host->warningsLogWrite (_("Http: internal error"));
          return 1;
        }

      if (useChunks && chain->getStream ()->write ("\r\n", 2, &nbw))
        {
          td->connection->host->warningsLogWrite (_("Http: internal error"));
          return 1;
        }

      return 0;
    }
  return 1;
}

/*!
 * Check if the server can use the chunked transfer encoding and if the client
 * supports keep-alive connections.
 */
void
HttpDataHandler::checkDataChunks (HttpThreadContext* td, bool* keepalive,
                                  bool* useChunks)
{
  *keepalive = td->request.isKeepAlive ();
  *useChunks = false;

  /* Do not use chunked transfer with old HTTP/1.0 clients.  */
  if (*keepalive)
    {
      HttpResponseHeader::Entry *e;
      e = td->response.other.get ("Transfer-Encoding");
      if (e)
        e->value->assign ("chunked");
      else
        {
          e = new HttpResponseHeader::Entry ();
          e->name->assign ("Transfer-Encoding");
          e->value->assign ("chunked");
          td->response.other.put (*(e->name), e);
        }
      *useChunks = true;
    }
}
