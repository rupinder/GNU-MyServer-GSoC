/*
MyServer
Copyright (C) 2002-2008 The MyServer Team
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


#include "../include/http.h"
#include "../include/http_headers.h"
#include "../include/server.h"
#include "../include/file.h"
#include "../include/files_utility.h"
#include "../include/socket.h"
#include "../include/utility.h"
#include "../include/stringutils.h"
#include "../include/securestr.h"

#include "../include/http_data_read.h"

#include <string>
#include <ostream>

using namespace std;

extern "C"
{
#ifdef WIN32
#include <direct.h>
#include <errno.h>
#endif

#ifdef NOT_WIN
#include <string.h>
#include <errno.h>
#endif
}

#ifdef NOT_WIN
#include "../include/find_data.h"
#endif

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
int HttpDataRead::readContiguousPrimitivePostData(char* inBuffer,
                                                  u_long *inBufferPos,
                                                  u_long inBufferSize,
                                                  Socket *inSocket,
                                                  char* outBuffer,
                                                  u_long outBufferSize,
                                                  u_long* nbr,
                                                  u_long timeout)
{
  int ret;
  u_long nbtrSocket = 0;
  *nbr = 0;
  if(inBufferSize - *inBufferPos)
  {
    *nbr = min(outBufferSize, inBufferSize - *inBufferPos);
    memcpy(outBuffer, inBuffer + *inBufferPos, *nbr);
    *inBufferPos += *nbr;
  }

  /*
   * No other space in the out buffer, return from the function with success.
   */
  if(outBufferSize == *nbr)
    return 0;

  nbtrSocket = inSocket->bytesToRead();

  if(!nbtrSocket)
    return 0;

  ret = inSocket->recv(outBuffer + *nbr,  min(nbtrSocket, outBufferSize - *nbr), 0, timeout);

  if(ret == -1)
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
 *\return Return 0 on success.
 *\return -1 on internal error.
 *\return Any other value is the HTTP error code.
 */
int HttpDataRead::readChunkedPostData(char* inBuffer,
                                      u_long *inBufferPos,
                                      u_long inBufferSize,
                                      Socket *inSocket,
                                      char* outBuffer,
                                      u_long outBufferSize,
                                      u_long* outNbr,
                                      u_long timeout,
                                      File* out)
{
  u_long nbr;
  *outNbr = 0;

  for(;;)
  {
    u_long chunkNbr;
    u_long dataToRead;
    u_long nbw;
    u_long bufferlen;
    char buffer[20];
    char c;
    bufferlen = 0;
    buffer[0] = '\0';
    for(;;)
    {
      if(readContiguousPrimitivePostData(inBuffer,
                                         inBufferPos,
                                         inBufferSize,
                                         inSocket,
                                         &c,
                                         1,
                                         &nbr,
                                         timeout))
        return -1;

      if(nbr != 1)
        return -1;

      if((c != '\r') && (bufferlen < 19))
      {
        buffer[bufferlen++] = c;
        buffer[bufferlen] = '\0';
      }
      else
        break;
    }

    /* Read the \n character too. */
    if(readContiguousPrimitivePostData(inBuffer,
                                       inBufferPos,
                                       inBufferSize,
                                       inSocket,
                                       &c,
                                       1,
                                       &nbr,
                                       timeout))
       return -1;

    dataToRead = (u_long)hexToInt(buffer);

    /*! The last chunk length is 0.  */
    if(dataToRead == 0)
      break;

    chunkNbr = 0;

    while(chunkNbr < dataToRead)
    {
      u_long rs = min(outBufferSize , dataToRead - chunkNbr);

      if(readContiguousPrimitivePostData(inBuffer,
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

      if(nbr == 0)
        return -1;

      chunkNbr += nbr;

      if(out->writeToFile(outBuffer, nbr, &nbw))
      {
        return -1;
      }

      if(nbw != nbr)
        return -1;

      *outNbr += nbw;

      /* Read final chunk \r\n.  */
      if(readContiguousPrimitivePostData(inBuffer,
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
int HttpDataRead::readPostData(HttpThreadContext* td, int* httpRetCode)
{
  int contentLength = -1;

  u_long nbw = 0;
  u_long bufferDataSize = 0;


  u_long timeout = MYSERVER_SEC(10);
  u_long inPos = 0;
  u_long nbr;
  u_long length;

  HttpRequestHeader::Entry *contentType =
    td->request.other.get("Content-Type");

  HttpRequestHeader::Entry *encoding =
    td->request.other.get("Transfer-Encoding");

  /* Specify a type if it not specified by the client.  */
  if(contentType == 0)
  {
    contentType = new HttpRequestHeader::Entry();
    contentType->name->assign("Content-Type");
    contentType->value->assign("application/x-www-form-urlencoded");
  }
  else if(contentType->value->length() == 0)
  {
    contentType->value->assign("application/x-www-form-urlencoded");
  }

  td->request.uriOptsPtr = &(td->buffer->getBuffer())[td->nHeaderChars];
  td->buffer->getBuffer()[td->nBytesToRead < td->buffer->getRealLength() - 1
                   ? td->nBytesToRead : td->buffer->getRealLength()-1] = '\0';

  if(td->request.contentLength.length())
  {
    contentLength = atoi(td->request.contentLength.c_str());

    if(contentLength < 0)
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
  if(!contentLength && td->request.isKeepAlive())
  {
    HttpRequestHeader::Entry *content =
      td->request.other.get("Content-Encoding");

    if(content && (content->value->length() == '\0')
           && (td->request.contentLength.length() == 0))
    {
      *httpRetCode = 400;
      return 1;
    }
  }

  /*!
   *Create the file that contains the posted data.
   *This data is the stdin file in the CGI.
   */
  if(td->inputData.openFile(td->inputDataPath, File::MYSERVER_CREATE_ALWAYS |
                            File::MYSERVER_OPEN_READ |
                            File::MYSERVER_OPEN_WRITE))
  {
    *httpRetCode = 500;
    return 1;
  }

  length = contentLength;

  bufferDataSize = (td->nBytesToRead < td->buffer->getRealLength() - 1
                    ? td->nBytesToRead
                    : td->buffer->getRealLength() - 1 ) - td->nHeaderChars;

  /* If it is specified a transfer encoding read data using it.  */
  if(encoding)
  {
    if(!encoding->value->compare("chunked"))
    {
      int ret = readChunkedPostData(td->request.uriOptsPtr,
                                    &inPos,
                                    bufferDataSize,
                                    td->connection->socket,
                                    td->buffer2->getBuffer(),
                                    td->buffer2->getRealLength() - 1,
                                    &nbr,
                                    timeout,
                                    &(td->inputData));

      if(ret == -1)
      {
        td->inputDataPath.assign("");
        td->outputDataPath.assign("");
        td->inputData.closeFile();
        return -1;
      }
      else if(ret)
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
  /* If it is not specified an encoding, read the data as it is.  */
  else for(;;)
  {

    if(readContiguousPrimitivePostData(td->request.uriOptsPtr,
                                       &inPos,
                                       bufferDataSize,
                                       td->connection->socket,
                                       td->buffer2->getBuffer(),
                                       td->buffer2->getRealLength() - 1,
                                       &nbr,
                                       timeout))
    {
      td->inputData.closeFile();
      FilesUtility::deleteFile(td->inputDataPath);
      *httpRetCode = 400;
      return 1;
    }

    if(nbr <= length)
      length -= nbr;
    else
    {
      td->inputData.closeFile();
      FilesUtility::deleteFile(td->inputDataPath);
      *httpRetCode = 400;
      return 1;
    }

    td->buffer2->getBuffer()[nbr] = '\0';

    if(nbr && td->inputData.writeToFile(td->buffer2->getBuffer(), nbr, &nbw))
    {
      td->inputDataPath.assign("");
      td->outputDataPath.assign("");
      td->inputData.closeFile();
      return -1;
    }

    if(!length)
      break;
  }

  td->inputData.setFilePointer(0);
  return 0;
}
