/*
  MyServer
  Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010 Free Software
  Foundation, Inc.
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
#include <include/protocol/http/http.h>
#include <include/protocol/http/http_headers.h>
#include <include/http_handler/http_file/http_file.h>
#include <include/server/server.h>
#include <include/filter/filters_chain.h>
#include <include/filter/memory_stream.h>

#include <sstream>
#include <algorithm>
using namespace std;

#ifdef WIN32
# include <direct.h>
# include <errno.h>
#else
# include <string.h>
# include <errno.h>
#endif

/*!
  Main function to handle the HTTP PUT command.
 */
int HttpFile::putFile (HttpThreadContext* td, string& filename)
{
  u_long firstByte = td->request.rangeByteBegin;
  int successCode = 500;
  File file;
  int symFlags = td->http->areSymlinksAllowed () ? 0
    : File::NO_FOLLOW_SYMLINK;

  try
  {
    if (td->request.isKeepAlive ())
      td->response.setValue ("connection", "keep-alive");

    if (!(td->permissions & MYSERVER_PERMISSION_WRITE))
      return td->http->sendAuth ();

    if (FilesUtility::nodeExists (td->filenamePath.c_str ()))
      {
        try
          {
            file.openFile (td->filenamePath.c_str (), File::OPEN_IF_EXISTS
                           | File::WRITE | symFlags);
          }
        catch (exception & e)
          {
            td->connection->host->warningsLogWrite
              (_E ("HttpFile: error accessing file %s"),
               td->filenamePath.c_str (), &e);
            return td->http->raiseHTTPError (500);
          }

        successCode = 200;
      }
    else
      {
        /* The file doesn't exist.  */
        try
          {
            file.openFile (td->filenamePath.c_str (), File::FILE_OPEN_ALWAYS
                           | File::WRITE | symFlags);
          }
        catch (exception & e)
          {
            td->connection->host->warningsLogWrite
              (_E ("HttpFile: error accessing file %s"),
               td->filenamePath.c_str (), &e);
            return td->http->raiseHTTPError (500);
          }

        successCode = 201;
      }

    if (firstByte)
      file.seek (firstByte);

    for (;;)
      {
        size_t nbr = 0, nbw = 0;
        if (td->inputData.read (td->buffer->getBuffer (),
                                td->buffer->getRealLength (), &nbr))
          {
            file.close ();
            return td->http->raiseHTTPError (500);
          }

        if (! nbr)
          break;
        else
          {
            if (file.writeToFile (td->buffer->getBuffer (), nbr, &nbw))
              {
                file.close ();
                return td->http->raiseHTTPError (500);
              }
          }

        if (nbw != nbr)
          {
            file.close ();
            return td->http->raiseHTTPError (500);
          }
      }

    file.close ();

    td->http->raiseHTTPError (successCode);
    return HttpDataHandler::RET_OK;

  }
  catch (exception & e)
    {
      td->connection->host->warningsLogWrite (_E ("HttpFile: internal error"),
                                              &e);
      return td->http->raiseHTTPError (500);
    };
}

/*!
  Delete the resource identified by filename.
 */
int HttpFile::deleteFile (HttpThreadContext* td,
                          string& filename)
{
  string directory;
  string file;
  try
    {
      if (! (td->permissions & MYSERVER_PERMISSION_DELETE))
        return td->http->sendAuth ();

      if (FilesUtility::nodeExists (td->filenamePath))
        {
          FilesUtility::deleteFile (td->filenamePath.c_str ());
          return td->http->raiseHTTPError (202);
        }
      else
        return td->http->raiseHTTPError (204);
    }
  catch (exception & e)
    {
      td->connection->host->warningsLogWrite
        (_E ("HttpFile: cannot delete file %s"),
         td->filenamePath.c_str (), &e);
      return td->http->raiseHTTPError (500);
    };
}

/*!
  Generate the E-Tag header given the resource atime and size.

  \param etag output the etag to this string.
  \param atime Resource last modified time.
  \param size Resource size.
 */
void HttpFile::generateEtag (string & etag, u_long mtime, u_long fsize)
{
#define ROL(x, y) ((x << y) | (x >> (32 - y)))
  /* Do a bit rotation to have less significative bits in the middle.  */
  u_long x = ROL (mtime, 16) | fsize;
  char buf[16];
  sprintf (buf, "%lu", x);
  etag.assign (buf);
}


/*!
  Send a file to the client using the HTTP protocol.
  \param td The current HTTP thread context.
  \param filenamePath The path of the static file to send.
  \param exec Not used.
  \param execute Not used.
  \param onlyHeader Specify if send only the HTTP header.
  */
int HttpFile::send (HttpThreadContext* td, const char *filenamePath,
                    const char* exec, bool execute, bool onlyHeader)
{
  /*
    With this routine we send a file through the HTTP protocol.
    Open the file and save its handle.
   */
  size_t filesize = 0;
  File *file = NULL;
  size_t bytesToSend;
  size_t firstByte = td->request.rangeByteBegin;
  size_t lastByte = td->request.rangeByteEnd;
  bool keepalive = false;
  bool useChunks = false;
  bool useModifiers = false;
  MemoryStream memStream (td->auxiliaryBuffer);
  FiltersChain chain;
  size_t nbw;
  size_t nbr;
  time_t lastMT;
  string tmpTime;
  u_long dataSent = 0;

  try
    {

      if (! td->request.cmd.compare ("PUT"))
        return putFile (td, td->filenamePath);

      if (! td->request.cmd.compare ("DELETE"))
        return deleteFile (td, td->filenamePath);

      if (! (td->permissions & MYSERVER_PERMISSION_READ))
        return td->http->sendAuth ();

      if (! FilesUtility::nodeExists (filenamePath))
        return td->http->raiseHTTPError (404);

      lastMT = FilesUtility::getLastModTime (td->filenamePath.c_str ());

      if (lastMT == -1)
        return td->http->raiseHTTPError (500);

      getRFC822GMTTime (lastMT, tmpTime, 32);
      td->response.setValue ("last-modified", tmpTime.c_str ());

      HttpRequestHeader::Entry *ifModifiedSince =
        td->request.other.get ("last-modified");

      if (ifModifiedSince && ifModifiedSince->value.length () &&
          !ifModifiedSince->value.compare (tmpTime))
        return td->http->sendHTTPNonModified ();

      try
        {
          int symFlags = td->http->areSymlinksAllowed () ? 0
            : File::NO_FOLLOW_SYMLINK;
          file = Server::getInstance ()->getCachedFiles ()->open (filenamePath,
                                                                  symFlags);
          if (! file)
            return td->http->raiseHTTPError (500);
        }
      catch (exception & e)
        {
          td->connection->host->warningsLogWrite
            (_E ("HttpFile: error accessing file %s"),
             td->filenamePath.c_str (), &e);
          return td->http->raiseHTTPError (500);
        }

    /*
      Check how many bytes are ready to be send.
     */
    filesize = file->getFileSize ();

    string etag;
    generateEtag (etag, lastMT, filesize);

    HttpRequestHeader::Entry *etagHeader = td->request.other.get ("etag");
    if (etagHeader && !etagHeader->value.compare (etag))
      return td->http->sendHTTPNonModified ();
    else
      {
        HttpResponseHeader::Entry *e = td->response.other.get ("etag");
        if (e)
          e->value.assign (etag);
        else
          {
            e = new HttpResponseHeader::Entry ();
            e->name.assign ("etag");
            e->value.assign (etag);
            td->response.other.put (e->name, e);
          }
      }

    if (lastByte == 0)
      lastByte = filesize;
    else
      lastByte = std::min (lastByte + 1, filesize);

    bytesToSend = lastByte - firstByte;

    keepalive = td->request.isKeepAlive ();

    td->buffer->setLength (0);

    /* If a Range was requested send 206 and not 200 for success.  */
    if (td->request.rangeByteBegin || td->request.rangeByteEnd)
      {
        HttpResponseHeader::Entry *e;
        ostringstream buffer;
        td->response.httpStatus = 206;
        buffer << "bytes "<< (u_long) firstByte << "-"
               << (u_long) (lastByte - 1) << "/" << (u_long) filesize;

        e = td->response.other.get ("content-range");
        if (e)
          e->value.assign (buffer.str ());
        else
          {
            e = new HttpResponseHeader::Entry ();
            e->name.assign ("content-range");
            e->value.assign (buffer.str ());
            td->response.other.put (e->name, e);
          }
      }
    chain.setStream (&memStream);
    if (td->mime)
      {
        HttpRequestHeader::Entry* e = td->request.other.get ("accept-encoding");
        if (td->mime)
          Server::getInstance ()->getFiltersFactory ()->chain (&chain,
                                                               td->mime->filters,
                                                               &memStream,
                                                               &nbw, 0,
                                                               e ? &e->value : NULL);
        memStream.refresh ();
        dataSent += nbw;
      }

    useModifiers = chain.hasModifiersFilters ();
    if (!useModifiers)
      {
        ostringstream buffer;
        buffer << (u_int) bytesToSend;
        td->response.contentLength.assign (buffer.str ());
      }

    if (keepalive)
      td->response.setValue ("connection", "keep-alive");
    else
      td->response.setValue ("connection", "close");

    if (useModifiers)
      {
        string s;
        HttpResponseHeader::Entry *e;
        chain.getName (s);
        e = td->response.other.get ("content-encoding");

        if (e)
          e->value.assign (s);
        else
          {
            e = new HttpResponseHeader::Entry ();
            e->name.assign ("content-encoding");
            e->value.assign (s);
            td->response.other.put (e->name, e);
          }
        /* Do not use chunked transfer with old HTTP/1.0 clients.  */
        if (keepalive)
          useChunks = true;
      }

    if (useChunks)
      {
        HttpResponseHeader::Entry *e;
        e = td->response.other.get ("transfer-encoding");
        if (e)
          e->value.assign ("chunked");
        else
          {
            e = new HttpResponseHeader::Entry ();
            e->name.assign ("transfer-encoding");
            e->value.assign ("chunked");
            td->response.other.put (e->name, e);
          }
      }
    else
      {	HttpResponseHeader::Entry *e;
        e = td->response.other.remove ("transfer-encoding");
        if (e)
          delete e;
      }

    HttpHeaders::sendHeader (td->response, *td->connection->socket,
                             *td->buffer, td);

    /*
      If is requested only the header exit from the function;
      used by the HEAD request.
    */
    if (onlyHeader)
      {
        file->close ();
        delete file;
        chain.clearAllFilters ();
        return HttpDataHandler::RET_OK;
      }

    /*
      Check if there are all the conditions to use a direct copy from the
      file to the socket.
    */
    if (!useChunks && chain.isEmpty () && !td->appendOutputs
        && !(td->http->getProtocolOptions () & Protocol::SSL))
      {
        size_t nbw = 0;
        file->fastCopyToSocket (td->connection->socket, firstByte,
                                td->buffer, &nbw);

        file->close ();
        delete file;

        chain.clearAllFilters ();

        td->sentData += nbw;

        return HttpDataHandler::RET_OK;
      }

    file->seek (firstByte);

    if (td->appendOutputs)
      chain.setStream (&(td->outputData));
    else
      chain.setStream (td->connection->socket);

    /*
      Flush initial data.  This is data that filters could have added
      and we have to send before the file itself, for example the gzip
      filter add a header to file.
    */
    if (memStream.availableToRead ())
      {
        memStream.read (td->buffer->getBuffer (),
                        td->buffer->getRealLength (), &nbr);

        memStream.refresh ();
        if (nbr)
          HttpDataHandler::appendDataToHTTPChannel (td,
                                                    td->buffer->getBuffer (),
                                                    nbr, &(td->outputData),
                                                    &chain, td->appendOutputs,
                                                    useChunks);
      } /* memStream.availableToRead ().  */

    /* Flush the rest of the file.  */
    for (;;)
      {
        size_t nbr;
        size_t nbw;

        /* Check if there are other bytes to send.  */
        if (bytesToSend)
          {
            /* Read from the file the bytes to send.  */
            size_t size = std::min (bytesToSend,
                                    td->buffer->getRealLength () / 2);

            file->read (td->buffer->getBuffer (), size, &nbr);
            if (nbr == 0)
              {
                bytesToSend = 0;
                continue;
              }

            bytesToSend -= nbr;

            appendDataToHTTPChannel (td, td->buffer->getBuffer (),
                           nbr, &(td->outputData), &chain, td->appendOutputs,
                               useChunks, td->buffer->getRealLength (), &memStream);
            dataSent += nbr;
          }
        else /* if (bytesToSend) */
          {
            /* If we don't use chunks we can flush directly.  */
            if (!useChunks)
              {
                chain.flush (&nbw);
                break;
              }
            else
              {
                /*
                  Replace the final stream before the flush and write to a
                  memory buffer, after all the data is flushed from the
                  chain we can replace the stream with the original one and
                  write there the HTTP data chunk.
                */
                Stream* tmpStream = chain.getStream ();
                chain.setStream (&memStream);
                memStream.refresh ();
                chain.flush (&nbw);

                chain.setStream (tmpStream);
                memStream.read (td->buffer->getBuffer (),
                                td->buffer->getRealLength (), &nbr);

                HttpDataHandler::appendDataToHTTPChannel (td,
                                                       td->buffer->getBuffer (),
                                                       nbr, &(td->outputData),
                                                       &chain, td->appendOutputs,
                                                          useChunks);
                break;
              }
          }
        memStream.refresh ();
      }/* End for loop.  */

    file->close ();
    delete file;
  }
  catch (exception & e)
    {
      if (file)
        {
          file->close ();
          delete file;
        }
      chain.clearAllFilters ();
      td->connection->host->warningsLogWrite (_E ("HttpFile: internal error"),
                                              &e);
      return td->http->raiseHTTPError (500);
    }

  /* For logging activity.  */
  td->sentData += dataSent;
  chain.clearAllFilters ();

  return HttpDataHandler::RET_OK;
}

/*!
  Constructor for the class.
*/
HttpFile::HttpFile ()
{

}

/*!
  Destroy the object.
*/
HttpFile::~HttpFile ()
{

}

/*!
  Load the static elements.
  \param confFile Not used.
*/
int HttpFile::load ()
{
  return 0;
}

/*!
  Unload the static elements.
*/
int HttpFile::unLoad ()
{
  return 0;
}
