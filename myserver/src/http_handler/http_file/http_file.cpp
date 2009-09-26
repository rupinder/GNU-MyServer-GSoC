/*
MyServer
Copyright (C) 2005, 2006, 2007, 2008, 2009 Free Software Foundation, Inc.
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


#include <include/protocol/http/http.h>
#include <include/protocol/http/http_headers.h>
#include <include/http_handler/http_file/http_file.h>
#include <include/filter/gzip/gzip.h>
#include <include/server/server.h>
#include <include/filter/filters_chain.h>
#include <include/filter/memory_stream.h>

#include <sstream>
#include <algorithm>
using namespace std;

extern "C"
{
#ifdef WIN32
# include <direct.h>
# include <errno.h>
#else
# include <string.h>
# include <errno.h>
#endif
}

/*!
 * Main function to handle the HTTP PUT command.
 */
int HttpFile::putFile (HttpThreadContext* td,
                       string& filename)
{
  u_long firstByte = td->request.rangeByteBegin;
  int keepalive = 0;
  int ret;

  try
  {
    HttpHeaders::buildDefaultHTTPResponseHeader(&td->response);

    if(td->request.isKeepAlive())
      {
        td->response.connection.assign("keep-alive");
        keepalive = 1;
      }

    if(!(td->permissions & MYSERVER_PERMISSION_WRITE))
      return td->http->sendAuth();

    if(FilesUtility::fileExists (td->filenamePath.c_str()))
      {
        File file;
        if (file.openFile (td->filenamePath.c_str(), File::OPEN_IF_EXISTS |
                           File::WRITE))
          return td->http->raiseHTTPError (500);

        file.seek (firstByte);

        for(;;)
          {
            u_long nbr = 0, nbw = 0;
            if(td->inputData.read (td->buffer->getBuffer(),
                                   td->buffer->getRealLength(), &nbr))
              {
                file.close ();
                return td->http->raiseHTTPError(500);
              }

            if(nbr)
              {
                if (file.writeToFile (td->buffer->getBuffer (), nbr, &nbw))
                  {
                    file.close ();
                  return td->http->raiseHTTPError (500);
                  }
              }
            else
              break;

          if (nbw != nbr)
            {
              file.close ();
              return td->http->raiseHTTPError (500);
            }
        }
        file.close ();

        td->http->raiseHTTPError (200);
        return keepalive;
      }
    else
      {
        /* The file doesn't exist.  */
        File file;
        if(file.openFile(td->filenamePath.c_str(),
                         File::FILE_CREATE_ALWAYS |
                         File::WRITE))
          {
            /* Internal server error. */
            return td->http->raiseHTTPError(500);
          }

        for (;;)
          {
            u_long nbr = 0, nbw = 0;
            if (td->inputData.read (td->buffer->getBuffer (),
                                    td->buffer->getRealLength (), &nbr))
              {
                file.close ();
                return td->http->raiseHTTPError (500);
              }
            if(nbr)
              {
                if (file.writeToFile (td->buffer->getBuffer (), nbr, &nbw))
                  {
                    file.close ();
                    return td->http->raiseHTTPError (500);
                  }
              }
            else
              break;
            if( nbw != nbr )
              {
                file.close ();
                return td->http->raiseHTTPError (500);
              }
          }
        file.close ();

        td->http->raiseHTTPError (201);
        return 1;
      }
  }
  catch (...)
    {
      return td->http->raiseHTTPError (500);
    };
}

/*!
 * Delete the resource identified by filename.
 */
int HttpFile::deleteFile (HttpThreadContext* td,
                          string& filename)
{
  int permissions = -1;
  string directory;
  string file;
  int ret;
  try
  {
    HttpHeaders::buildDefaultHTTPResponseHeader (&td->response);

    if (!(td->permissions & MYSERVER_PERMISSION_DELETE))
      return td->http->sendAuth ();

    if (FilesUtility::fileExists (td->filenamePath))
      {
        FilesUtility::deleteFile (td->filenamePath.c_str ());
        return td->http->raiseHTTPError (202);
      }
    else
      return td->http->raiseHTTPError (204);
  }
  catch(...)
    {
      return td->http->raiseHTTPError (500);
    };

}

/*!
 * Generate the E-Tag header given the resource atime and size.
 *
 * \param etag output the etag to this string.
 * \param atime Resource last modified time.
 * \param size Resource size.
 */
void HttpFile::generateEtag (string & etag, u_long mtime, u_long fsize)
{
#define ROL(x, y) ((x << y) | (x >> (32 - y)))
  /* Do a bit rotation to have less significative bits in the middle.  */
  u_long x = ROL(mtime, 16) | fsize;

  char buf[16];
  sprintf (buf, "%u", x);
  etag.assign (buf);
}


/*!
 * Send a file to the client using the HTTP protocol.
 * \param td The current HTTP thread context.
 * \param filenamePath The path of the static file to send.
 * \param exec Not used.
 * \param execute Not used.
 * \param onlyHeader Specify if send only the HTTP header.
  */
int HttpFile::send (HttpThreadContext* td, const char *filenamePath,
                    const char* exec, bool execute, bool onlyHeader)
{
  /*
   * With this routine we send a file through the HTTP protocol.
   * Open the file and save its handle.
   */
  int ret;

  /*
   * Will we use GZIP compression to send data?
   */
  bool useGzip = false;
  u_long filesize = 0;
  File *file = 0;
  u_long bytesToSend;
  u_long firstByte = td->request.rangeByteBegin;
  u_long lastByte = td->request.rangeByteEnd;
  bool keepalive = false;
  bool useChunks = false;
  bool useModifiers = false;
  MemoryStream memStream (td->secondaryBuffer);
  FiltersChain chain;
  u_long nbw;
  u_long nbr;
  time_t lastMT;
  string tmpTime;
  u_long dataSent = 0;

  try
  {

    if (!td->request.cmd.compare ("PUT"))
      return putFile (td, td->filenamePath);

    if (!td->request.cmd.compare ("DELETE"))
      return deleteFile (td, td->filenamePath);

    if (!(td->permissions & MYSERVER_PERMISSION_READ))
      return td->http->sendAuth ();

    if (!FilesUtility::fileExists (filenamePath))
      return td->http->raiseHTTPError(404);

    lastMT = FilesUtility::getLastModTime (td->filenamePath.c_str());

    if (lastMT == -1)
      return td->http->raiseHTTPError(500);

    getRFC822GMTTime (lastMT, tmpTime, HTTP_RESPONSE_LAST_MODIFIED_DIM);
    td->response.setValue ("Last-Modified", tmpTime.c_str ());

    HttpRequestHeader::Entry *ifModifiedSince =
      td->request.other.get ("Last-Modified");

    if (ifModifiedSince && ifModifiedSince->value->length () &&
        !ifModifiedSince->value->compare (tmpTime))
      return td->http->sendHTTPNonModified ();

    file = Server::getInstance ()->getCachedFiles ()->open (filenamePath);
    if (file == 0)
      return td->http->raiseHTTPError (500);

    /*
     * Check how many bytes are ready to be send.
     */
    filesize = file->getFileSize ();

    string etag;
    generateEtag (etag, lastMT, filesize);

    HttpRequestHeader::Entry *etagHeader = td->request.other.get ("ETag");
    if (etagHeader &&  !etagHeader->value->compare (etag))
      return td->http->sendHTTPNonModified ();
    else
      {
      HttpResponseHeader::Entry *e = td->response.other.get ("Etag");
      if (e)
        e->value->assign (etag);
      else
        {
          e = new HttpResponseHeader::Entry ();
          e->name->assign ("Etag");
          e->value->assign (etag);
          td->response.other.put (*(e->name), e);
        }
      }

    bytesToSend = filesize;
    if (lastByte == 0)
      lastByte = bytesToSend;
    else
      {
        /*
         * If the client use ranges set the right value
         * for the last byte number.
         */
        lastByte = std::min (lastByte + 1, bytesToSend);
      }

    /*
     * bytesToSend is the interval between the first and the last byte.
     */
    bytesToSend = lastByte - firstByte;

    /*
     * If fail to set the file pointer returns an internal server error.
     */
    ret = file->seek (firstByte);
    if (ret)
      {
        file->close ();
        delete file;
        return td->http->raiseHTTPError (500);
      }

    /*
     * Use GZIP compression to send files bigger than GZIP threshold.
     */
    const char *val = td->securityToken.getData ("gzip.threshold",
                                                       MYSERVER_SECURITY_CONF
                                                       | MYSERVER_VHOST_CONF
                                                       | MYSERVER_MIME_CONF
                                                       | MYSERVER_SERVER_CONF, "0");

    useGzip = false;
    if (val)
      {
        u_long gzipThreshold = atoi (val);
        if (bytesToSend >= gzipThreshold)
          useGzip = true;
      }

    keepalive = td->request.isKeepAlive ();

#ifdef HAVE_ZLIB
    /*
     * Be sure that the client accept GZIP compressed data.
     */
    if (useGzip)
      {
        HttpRequestHeader::Entry* e = td->request.other.get ("Accept-Encoding");
        if (e)
          useGzip &= (e->value->find("gzip") != string::npos);
        else
          useGzip = false;
      }
#else
    /* If compiled without GZIP support force the server to don't use it.  */
    useGzip = false;
#endif
    if (td->appendOutputs)
      useGzip = false;

    td->buffer->setLength (0);

    /* If a Range was requested send 206 and not 200 for success.  */
    if (td->request.rangeByteBegin || td->request.rangeByteEnd)
      {
        HttpResponseHeader::Entry *e;
        ostringstream buffer;
        td->response.httpStatus = 206;
        buffer << "bytes "<< (u_long)firstByte << "-"
               << (u_long) (lastByte - 1) << "/" << (u_long)filesize;

        e = td->response.other.get ("Content-Range");
        if (e)
          e->value->assign (buffer.str ());
      else
        {
          e = new HttpResponseHeader::Entry ();
          e->name->assign ("Content-Range");
          e->value->assign (buffer.str());
          td->response.other.put (*(e->name), e);
        }

      e = td->response.other.get ("Transfer-Encoding");
      if (e)
        e->value->assign ("chunked");
      else
        {
          e = new HttpResponseHeader::Entry();
          e->name->assign ("Transfer-Encoding");
          e->value->assign ("chunked");
          td->response.other.put (*(e->name), e);
        }

      useGzip = false;
    }
    chain.setProtocol (td->http);
    chain.setProtocolData (td);
    chain.setStream (&memStream);
    if (td->mime)
      {
        if (td->mime &&
            Server::getInstance ()->getFiltersFactory ()->chain (&chain,
                                                                 td->mime->filters,
                                                                 &memStream,
                                                                 &nbw))
          {
            file->close ();
            delete file;
            chain.clearAllFilters ();
            return 0;
          }
      memStream.refresh();
      dataSent += nbw;
    }

    if (useGzip && !chain.isFilterPresent ("gzip"))
      {
        Filter* gzipFilter =
          Server::getInstance()->getFiltersFactory()->getFilter ("gzip");
        u_long nbw;
        if (!gzipFilter)
          {
            file->close ();
            delete file;
            chain.clearAllFilters ();
            return 0;
          }
        if (chain.addFilter (gzipFilter, &nbw))
          {
            delete gzipFilter;
            file->close ();
            delete file;
            chain.clearAllFilters ();
            return 0;
          }
        dataSent += nbw;
      }

    useModifiers = chain.hasModifiersFilters ();

    if (!useModifiers)
      {
        ostringstream buffer;
        buffer << (u_int)bytesToSend;
        td->response.contentLength.assign (buffer.str ());
      }

    if (keepalive)
      td->response.connection.assign("keep-alive");
    else
      td->response.connection.assign("close");

    if (useModifiers)
      {
        string s;
        HttpResponseHeader::Entry *e;
        chain.getName (s);
        e = td->response.other.get ("Content-Encoding");

        if (e)
          e->value->assign (s);
        else
          {
            e = new HttpResponseHeader::Entry ();
            e->name->assign ("Content-Encoding");
            e->value->assign (s);
            td->response.other.put (*(e->name), e);
          }
        /* Do not use chunked transfer with old HTTP/1.0 clients.  */
        if (keepalive)
          {
            HttpResponseHeader::Entry *e;
            e = td->response.other.get ("Transfer-Encoding");
            if(e)
              e->value->assign ("chunked");
            else
              {
                e = new HttpResponseHeader::Entry ();
                e->name->assign ("Transfer-Encoding");
                e->value->assign ("chunked");
                td->response.other.put (*(e->name), e);
              }
            useChunks = true;
          }
      }

    u_long hdrLen = HttpHeaders::buildHTTPResponseHeader (td->buffer->getBuffer(),
                                                          &td->response);

    td->buffer->setLength (hdrLen);

    if (!td->appendOutputs)
      {
        /* Send the HTTP header.  */
        if (td->connection->socket->send (td->buffer->getBuffer (),
                                          (u_long) td->buffer->getLength (),
                                          0) == SOCKET_ERROR)
          {
            file->close ();
            delete file;
            chain.clearAllFilters ();
            return 1;
          }
      }

    /*
     * If is requested only the header exit from the function;
     * used by the HEAD request.
     */
    if (onlyHeader)
      {
        file->close ();
        delete file;
        chain.clearAllFilters ();
        return 0;
      }

    /*
     * Check if there are all the conditions to use a direct copy from the
     * file to the socket.
     */
    if (!useChunks && chain.isEmpty () && !td->appendOutputs
        && !(td->http->getProtocolOptions() & PROTOCOL_USES_SSL))
      {
        u_long nbw = 0;
        int ret = file->fastCopyToSocket (td->connection->socket, firstByte,
                                          td->buffer, &nbw);

        file->close ();
        delete file;

        chain.clearAllFilters ();

        td->sentData += nbw;

        return ret;
      }

    if (td->appendOutputs)
      chain.setStream (&(td->outputData));
    else
      chain.setStream (td->connection->socket);

    /*
     * Flush initial data.  This is data that filters could have added
     * and we have to send before the file itself, for example the gzip
     * filter add a header to file.
     */
    if (memStream.availableToRead ())
      {
        if (memStream.read (td->buffer->getBuffer(),
                              td->buffer->getRealLength(),
                            &nbr))
          {
            file->close ();
            delete file;
            chain.clearAllFilters ();
            return 0;
          }

        memStream.refresh ();

        if (nbr)
          {
            if (HttpDataHandler::appendDataToHTTPChannel(td,
                                   td->buffer->getBuffer(),
                                   nbr, &(td->outputData),
                                   &chain, td->appendOutputs,
                                   useChunks))
              {
                file->close ();
                delete file;
                chain.clearAllFilters ();
                return 1;
                dataSent += nbw;
              }
          } /* nbr.  */
      } /* memStream.availableToRead().  */

    /* Flush the rest of the file.  */
    for (;;)
      {
        u_long nbr;
        u_long nbw;

        /* Check if there are other bytes to send.  */
        if (bytesToSend)
          {
            /* Read from the file the bytes to send.  */
            if (ret = file->read (td->buffer->getBuffer(),
                  std::min (static_cast<u_long> (bytesToSend),
                  static_cast<u_long> (td->buffer->getRealLength() / 2)),
                  &nbr))
              break;

            if (nbr == 0)
              {
                bytesToSend = 0;
                continue;
              }

            bytesToSend -= nbr;

            if (ret = appendDataToHTTPChannel (td, td->buffer->getBuffer (),
                     nbr, &(td->outputData), &chain, td->appendOutputs,
                     useChunks, td->buffer->getRealLength (), &memStream))
              break;

            dataSent += nbr;
          }
        else /* if(bytesToSend) */
          {
            /* If we don't use chunks we can flush directly.  */
            if (!useChunks)
              {
                ret = chain.flush (&nbw);
                break;
              }
            else
              {
                /*
                 * Replace the final stream before the flush and write to a
                 * memory buffer, after all the data is flushed from the
                 * chain we can replace the stream with the original one and
                 * write there the HTTP data chunk.
                 */
                Stream* tmpStream = chain.getStream ();
                chain.setStream (&memStream);
                memStream.refresh ();
                if (ret = chain.flush (&nbw))
                  break;

                chain.setStream (tmpStream);
                if (ret = memStream.read (td->buffer->getBuffer (),
                    td->buffer->getRealLength(), &nbr))
                  break;

                if (ret = HttpDataHandler::appendDataToHTTPChannel (td,
                    td->buffer->getBuffer (), nbr, &(td->outputData),
                    &chain, td->appendOutputs, useChunks))
                  break;

                ret = HttpDataHandler::appendDataToHTTPChannel (td, 0, 0,
                                         &(td->outputData), &chain,
                                         td->appendOutputs,useChunks);
                break;
              }
          }
        memStream.refresh ();
    }/* End for loop.  */

    file->close ();
    delete file;
  }
  catch (...)
    {
      file->close ();
      delete file;
      td->connection->host->warningsLogWrite (_("HttpFile: internal error"));
      chain.clearAllFilters ();
      return td->http->raiseHTTPError (500);
    }

  /* For logging activity.  */
  td->sentData += dataSent;

  chain.clearAllFilters ();
  return !ret;
}

/*!
 * Constructor for the class.
 */
HttpFile::HttpFile ()
{

}

/*!
 * Destroy the object.
 */
HttpFile::~HttpFile ()
{

}

/*!
 * Load the static elements.
 * \param confFile Not used.
 */
int HttpFile::load ()
{
  return 0;
}

/*!
 * Unload the static elements.
 */
int HttpFile::unLoad ()
{
  return 0;
}
