/*
*MyServer
*Copyright (C) 2005 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "../include/http.h"
#include "../include/http_headers.h"
#include "../include/http_file.h"
#include "../include/gzip.h"
#include "../include/cserver.h"
#include "../include/filters_chain.h"
#include "../include/memory_stream.h"
#include <sstream>

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

/*!
 *Send a file to the client using the HTTP protocol.
 */
int HttpFile::send(HttpThreadContext* td, ConnectionPtr s, const char *filenamePath, 
                   const char* /*exec*/,int onlyHeader)
{
	/*!
   *With this routine we send a file through the HTTP protocol.
   *Open the file and save its handle.
   */
	int ret;
	/*! 
   *Will we use GZIP compression to send data?
   */
	int useGzip=0;
  u_long filesize=0;
	File h;
	u_long bytes_to_send;
  u_long firstByte = td->request.RANGEBYTEBEGIN; 
  u_long lastByte = td->request.RANGEBYTEEND;
  int keepalive;
  MemoryStream memStream(td->buffer2);
  FiltersChain chain;
	
	/*! Number of bytes created by the zip compressor by loop.  */
	u_long GzipDataused=0;
	int dataSent=0;

  try
  {
    ret  = h.openFile(filenamePath, FILE_OPEN_IFEXISTS | 
                      FILE_OPEN_READ);
    if(ret)
    {	
      return ((Http*)td->lhttp)->raiseHTTPError(td, s, e_500);
    }
    /*! 
     *Read how many bytes are waiting to be send.  
     */
    filesize=h.getFileSize();
    bytes_to_send=filesize;
    if(lastByte == 0)
    {
      lastByte = bytes_to_send;
    }
    else
    {
      /*! 
       *If the client use ranges set the right value 
       *for the last byte number.  
       */
      lastByte = ((u_long)lastByte+1 < bytes_to_send) ? lastByte+1 : bytes_to_send;
    }

    /*! 
     *Use GZIP compression to send files bigger than GZIP threshold.  
     */
    if( ((Http*)td->lhttp)->getGzipThreshold() && 
        (bytes_to_send > ((Http*)td->lhttp)->getGzipThreshold() ))
    {
      useGzip=1;
    }
    keepalive = !lstrcmpi(td->request.CONNECTION.c_str(),"Keep-Alive");

#ifndef DO_NOT_USEGZIP
    /*! 
     *Be sure that the client accept GZIP compressed data.  
     */
    if(useGzip)
      useGzip &= (td->request.ACCEPTENC.find("gzip") != string::npos);
#else
    /*! 
     *If compiled without GZIP support force the server to don't use it.  
     */
    useGzip=0;
#endif	
    if(td->appendOutputs)
      useGzip=0;

    /*! 
     *bytes_to_send is the interval between the first and the last byte.  
     */
    bytes_to_send=lastByte-firstByte;
    
    /*!
     *If fail to set the file pointer returns an internal server error.  
     */
    ret = h.setFilePointer(firstByte);
    if(ret)
    {
      h.closeFile();
      return  ((Http*)td->lhttp)->raiseHTTPError(td, s, e_500);
    }

    td->buffer->SetLength(0);

    /*! If a Range was requested send 206 and not 200 for success.  */
    if( td->request.RANGEBYTEBEGIN ||  td->request.RANGEBYTEEND )
    {	
      ostringstream buffer;
      td->response.httpStatus = 206;
      buffer << "bytes "<< (u_long)firstByte << "-" 
             << (u_long)lastByte << "/" << (u_long)filesize ;
      td->response.CONTENT_RANGE.assign(buffer.str());
      useGzip = 0;
    }

    if(keepalive && !useGzip)
    {
      ostringstream buffer;
      buffer << (u_int)bytes_to_send;
      td->response.CONTENT_LENGTH.assign(buffer.str());
    }

    /*! Specify the connection type. */
    if(keepalive)
    {
      td->response.CONNECTION.assign("keep-alive");
    }
    else
    {
      td->response.CONNECTION.assign("close");
    }

    if(useGzip)
    {
      td->response.CONTENT_ENCODING.assign("gzip");
    }
    /*! Do not use chunked transfer with old HTTP/1.0 clients.  */
    if(keepalive)
      td->response.TRANSFER_ENCODING.assign("chunked");

    HttpHeaders::buildHTTPResponseHeader(td->buffer->GetBuffer(), 
                                         &td->response);
    td->buffer->SetLength((u_long)strlen(td->buffer->GetBuffer()));
    if(!td->appendOutputs)
    {
      /*! Send the HTTP header.  */
      if(s->socket.send(td->buffer->GetBuffer(), 
                        (u_long)td->buffer->GetLength(), 0)== SOCKET_ERROR)
      {
        h.closeFile();
        return 0;
      }
    }

    /*! 
     *If is requested only the header exit from the function; 
     *used by the HEAD request.  
     */
    if(onlyHeader)
    {
      h.closeFile();
      return 1;
    }

    if(td->appendOutputs)
      chain.setStream(&(td->outputData));   
    else
        chain.setStream(&(s->socket));

    if(useGzip)
    {
      Filter* gzipFilter = lserver->getFiltersFactory()->getFilter("gzip");
      u_long nbw;
      if(!gzipFilter)
      {
        h.closeFile();
        chain.clearAllFilters();
        return 0;
      }

      if(chain.addFilter(gzipFilter, &nbw))
      {
        delete gzipFilter;
        h.closeFile();
        chain.clearAllFilters();
        return 0;
      }

      dataSent+=nbw;
    }

    for(;;)
    {
      u_long nbr;
      u_long breakAtTheEnd=0;
      /*! Read from the file the bytes to send. */
      ret = h.readFromFile(td->buffer->GetBuffer(),
                           (bytes_to_send<td->buffer->GetRealLength()? 
                            bytes_to_send :td->buffer->GetRealLength()), &nbr);
      if(ret)
        break;

      bytes_to_send-=nbr;

      /*! If there are bytes to send, send them. */
      if(!nbr)
      {
        ret=chain.flush(&nbr);
       
        if(ret || !keepalive)
          break;

        breakAtTheEnd=1; 
      }
      
      if(nbr)
      {
        u_long nbw;
        memStream.refresh();
        memStream.write(td->buffer->GetBuffer(), nbr, &nbw);
        if(keepalive)
        {
          ostringstream buffer;
          Stream *s;
          u_long availableToRead=memStream.availableToRead();
          buffer << hex << availableToRead << "\r\n";
          /*!TODO: remove ugly (char*) cast. */
          ret=chain.getStream()->write((char*)buffer.str().c_str(), buffer.str().length(), &nbw);
          if(ret)
            break;

          s=chain.getFirstFilter();
          if(!s)
             s=chain.getStream();
          if(!s)
            break;

          ret=memStream.read(s, availableToRead, &nbw);
          if(ret)
            break;     
          
          dataSent += nbw;


        }
        else
        {
          u_long nbw;
          ret = chain.write(td->buffer->GetBuffer(), nbr, &nbw);
          if(ret)
            break;     
          
          dataSent += nbw;       

          ret=chain.getStream()->write("\r\n", 2, &nbw);
          if(ret)
            break;   
        }
      }

      if(breakAtTheEnd)
      {
        if(keepalive)
        {
          u_long nbw;
          ret=chain.getStream()->write("0\r\n\r\n", 5, &nbw);
        }
        break;
      }

    }/*End for loop. */

    h.closeFile();
    /*! Update the Content-Length field for logging activity. */
    {
      ostringstream buffer;
      buffer << dataSent;
      td->response.CONTENT_LENGTH.assign(buffer.str());
    }
  }
  catch(bad_alloc &ba)
  {
    h.closeFile();
    ((Vhost*)(s->host))->warningslogRequestAccess(td->id);
    ((Vhost*)(s->host))->warningsLogWrite("HTTP File: Error allocating memory\r\n");
    ((Vhost*)(s->host))->warningslogTerminateAccess(td->id);
    chain.clearAllFilters();
    return ((Http*)td->lhttp)->raiseHTTPError(td, s, e_500);
  }
  catch(...)
  {
    h.closeFile();
    ((Vhost*)(s->host))->warningslogRequestAccess(td->id);
    ((Vhost*)(s->host))->warningsLogWrite("HTTP File: Internal error\r\n");
    ((Vhost*)(s->host))->warningslogTerminateAccess(td->id);
    chain.clearAllFilters();
    return ((Http*)td->lhttp)->raiseHTTPError(td, s, e_500);
  };

  chain.clearAllFilters();
	return 1;
}

/*!
 *Constructor for the class.
 */
HttpFile::HttpFile()
{

}

/*!
 *Destroy the object.
 */
HttpFile::~HttpFile()
{

}

/*!
 *Load the static elements.
 */
int HttpFile::load(XmlParser* /*confFile*/)
{
  return 0;
}

/*!
 *Unload the static elements.
 */
int HttpFile::unload()
{
  return 0;
}
