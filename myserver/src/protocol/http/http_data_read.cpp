/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
  Free Software Foundation, Inc.
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
#include <include/server/server.h>
#include <include/base/file/file.h>
#include <include/base/file/files_utility.h>
#include <include/base/socket/socket.h>
#include <include/base/utility.h>
#include <include/base/string/stringutils.h>
#include <include/base/string/securestr.h>
#include <include/protocol/http/http_data_read.h>

#include <string>
#include <ostream>

using namespace std;

/*!
 *Read primitive post data as it is sent by the client without apply any filter
 *in a contiguous manner, first read from the memory buffer and after from the
 *socket.
 *\param inBuffer Memory buffer with first part of POST data.
 *\param inBufferPos inBuffer size, this value is modified by the function,
 *has to be 0 on first call.
 *\param inBufferSize inBuffer size.
 *\param inSocket Connection socket to read from.
 *\param outBuffer Out buffer where write.
 *\param outBufferSize outBuffer size.
 *\param nbr Number of bytes read.
 *\param timeout Timeout value to use on the socket.
 *\return Return 0 on success.
 */
int HttpDataRead::readContiguousPrimitivePostData (const char* inBuffer,
                                                  u_long *inBufferPos,
                                                  u_long inBufferSize,
                                                  Socket *inSocket,
                                                  char* outBuffer,
                                                  u_long outBufferSize,
                                                  u_long* nbr,
                                                  u_long timeout)
{
  int ret;
  *nbr = 0;
  if (inBufferSize - *inBufferPos)
  {
    *nbr = min (outBufferSize, inBufferSize - *inBufferPos);
    memcpy (outBuffer, inBuffer + *inBufferPos, *nbr);
    *inBufferPos += *nbr;
  }

  /*
   * No other space in the out buffer, return from the function with success.
   */
  if (outBufferSize == *nbr)
    return 0;

  ret = inSocket->recv (outBuffer + *nbr,  outBufferSize - *nbr, 0, timeout);

  if (ret == -1)
    return -1;

  *nbr += ret;

  return 0;
}

/*!
 *Read post data using the chunked transfer encoding.
 *This function uses the same arguments of readContiguousPrimitivePostData with
 *the additional destination file.
 *\param inBuffer Memory buffer with first part of POST data.
 *\param inBufferPos inBuffer size, this value is modified by the function,
 *has to be 0 on first call.
 *\param inBufferSize inBuffer size.
 *\param inSocket Connection socket to read from.
 *\param outBuffer Out buffer where write.
 *\param outBufferSize outBuffer size.
 *\param outNbr Number of bytes read.
 *\param timeout Timeout value to use on the socket.
 *\param out Output file, may be  a NULL pointer.
 *\param maxChunks The maximum number of chunks to read.
 *\return Return 0 on success.
 *\return -1 on internal error.
 *\return Any other value is the HTTP error code.
 */
int HttpDataRead::readChunkedPostData (const char* inBuffer,
                                      u_long *inBufferPos,
                                      u_long inBufferSize,
                                      Socket *inSocket,
                                      char* outBuffer,
                                      u_long outBufferSize,
                                      u_long* outNbr,
                                      u_long timeout,
                                      Stream* out,
                                      long maxChunks)
{
  u_long nbr;
  *outNbr = 0;

  for (int n = 0; maxChunks == 0 || n < maxChunks; n++)
  {
    u_long chunkNbr;
    u_long dataToRead;
    u_long nbw;
    u_long bufferlen;
    char buffer[20];
    char c;
    bufferlen = 0;
    buffer[0] = '\0';
    for (;;)
    {
      if (readContiguousPrimitivePostData (inBuffer,
                                         inBufferPos,
                                         inBufferSize,
                                         inSocket,
                                         &c,
                                         1,
                                         &nbr,
                                         timeout))
        return -1;

      if (nbr != 1)
        return -1;

      if ((c != '\r') && (bufferlen < 19))
      {
        buffer[bufferlen++] = c;
        buffer[bufferlen] = '\0';
      }
      else
        break;
    }

    /* Read the \n character too. */
    if (readContiguousPrimitivePostData (inBuffer,
                                       inBufferPos,
                                       inBufferSize,
                                       inSocket,
                                       &c,
                                       1,
                                       &nbr,
                                       timeout))
       return -1;

    dataToRead = (u_long) hexToInt (buffer);

    /*! The last chunk length is 0.  */
    if (dataToRead == 0)
      break;

    chunkNbr = 0;

    while (chunkNbr < dataToRead)
    {
      u_long rs = min (outBufferSize , dataToRead - chunkNbr);

      if (readContiguousPrimitivePostData (inBuffer,
                                         inBufferPos,
                                         inBufferSize,
                                         inSocket,
                                         outBuffer,
                                         rs,
                                         &nbr,
                                         timeout))
      {
        return -1;
      }

      if (nbr == 0)
        return -1;

      chunkNbr += nbr;

      if (out && out->write (outBuffer, nbr, &nbw))
        return -1;

      if (nbw != nbr)
        return -1;

      if (out)
        *outNbr += nbw;
      else
        *outNbr += nbr;

      /* Read final chunk \r\n.  */
      if (readContiguousPrimitivePostData (inBuffer,
                                         inBufferPos,
                                         inBufferSize,
                                         inSocket,
                                         outBuffer,
                                         2,
                                         &nbr,
                                         timeout))
      {
        return -1;
      }

    }

  }
  return 0;
}

/*!
 *Read POST data from the active connection.
 *\param td The Active thread context.
 *\param httpRetCode The HTTP error to report to the client.
 *\return Return 0 on success.
 *\return Return -1 on irreversible error and
 *        the connection should be removed immediately.
 *\return Any other value is a protocol error specified in HTTPRETCODE.
 */
int HttpDataRead::readPostData (HttpThreadContext* td, int* httpRetCode)
{
  int contentLength = 0;
  bool contentLengthSpecified = false;
  u_long nbw = 0;
  u_long bufferDataSize = 0;


  u_long timeout = MYSERVER_SEC (10);
  u_long inPos = 0;
  u_long nbr;
  u_long length;

  HttpRequestHeader::Entry *contentType =
    td->request.other.get ("Content-type");

  HttpRequestHeader::Entry *encoding =
    td->request.other.get ("Transfer-encoding");

  /* Specify a type if it not specified by the client.  */
  if (contentType == 0)
  {
    contentType = new HttpRequestHeader::Entry ();
    contentType->name->assign ("Content-type");
    contentType->value->assign ("application/x-www-form-urlencoded");
  }
  else if (contentType->value->length () == 0)
  {
    contentType->value->assign ("application/x-www-form-urlencoded");
  }

  td->request.uriOptsPtr = &(td->buffer->getBuffer ())[td->nHeaderChars];
  td->buffer->getBuffer ()[td->nBytesToRead < td->buffer->getRealLength () - 1
                   ? td->nBytesToRead : td->buffer->getRealLength ()-1] = '\0';

  if (td->request.contentLength.length ())
  {
    contentLength = atoi (td->request.contentLength.c_str ());
    contentLengthSpecified = true;
    if (contentLength < 0)
    {
      *httpRetCode = 400;
      return 1;
    }
  }

  /*!
   *If the connection is Keep-Alive be sure that the client specify the
   *HTTP CONTENT-LENGTH field.
   *If a CONTENT-ENCODING is specified the CONTENT-LENGTH is not
   *always needed.
   */
  if (!contentLengthSpecified && td->request.isKeepAlive ())
  {
    HttpRequestHeader::Entry *content =
      td->request.other.get ("Content-Encoding");

    if (content && (content->value->length () == '\0')
           && (td->request.contentLength.length () == 0))
    {
      *httpRetCode = 400;
      return 1;
    }
  }

  /*!
   *Create the file that contains the posted data.
   *This data is the stdin file in the CGI.
   */
  if (td->inputData.openFile (td->inputDataPath, File::FILE_CREATE_ALWAYS |
                            File::READ |
                            File::WRITE))
  {
    *httpRetCode = 500;
    return 1;
  }

  length = contentLength;

  bufferDataSize = (td->nBytesToRead < td->buffer->getRealLength () - 1
                    ? td->nBytesToRead
                    : td->buffer->getRealLength () - 1 ) - td->nHeaderChars;

  /* If it is specified a transfer encoding read data using it.  */
  if (encoding)
  {
    if (!encoding->value->compare ("chunked"))
    {
      int ret = readChunkedPostData (td->request.uriOptsPtr,
                                     &inPos,
                                     bufferDataSize,
                                     td->connection->socket,
                                     td->auxiliaryBuffer->getBuffer (),
                                     td->auxiliaryBuffer->getRealLength () - 1,
                                     &nbr,
                                     timeout,
                                     &(td->inputData),
                                     0);

      if (ret == -1)
      {
        td->inputDataPath.assign ("");
        td->outputDataPath.assign ("");
        td->inputData.close ();
        return -1;
      }
      else if (ret)
      {
        *httpRetCode = ret;
        return 1;
      }

    }
    else
    {
      *httpRetCode = 501;
      return 1;
    }
  }
  else
    {
      /* If it is not specified an encoding, read the data as it is.  */
      if (!contentLengthSpecified)
        {
          *httpRetCode = 400;
          return 1;
        }

      for (;;)
        {

          /* Do not try to read more than what we expect.  */
          u_long dimBuffer = std::min (td->auxiliaryBuffer->getRealLength () - 1ul,
                                       length);

          if (readContiguousPrimitivePostData (td->request.uriOptsPtr,
                                               &inPos,
                                               bufferDataSize,
                                               td->connection->socket,
                                               td->auxiliaryBuffer->getBuffer (),
                                               dimBuffer,
                                               &nbr,
                                               timeout))
            {
              td->inputData.close ();
              FilesUtility::deleteFile (td->inputDataPath);
              *httpRetCode = 400;
              return 1;
            }

          if (nbr <= length)
            length -= nbr;
          else
            {
              td->inputData.close ();
              FilesUtility::deleteFile (td->inputDataPath);
              *httpRetCode = 400;
              return 1;
            }

          td->auxiliaryBuffer->getBuffer ()[nbr] = '\0';

          if (nbr && td->inputData.writeToFile (td->auxiliaryBuffer->getBuffer (),
                                                nbr, &nbw))
            {
              td->inputDataPath.assign ("");
              td->outputDataPath.assign ("");
              td->inputData.close ();
              return -1;
            }

          if (!length)
            break;
        }
    }

  td->inputData.seek (0);
  return 0;
}
