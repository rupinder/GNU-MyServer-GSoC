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
int HttpFile::send(HttpThreadContext* td, ConnectionPtr s, char *filenamePath, 
                   char* /*exec*/,int only_header)
{
	/*!
   *With this routine we send a file through the HTTP protocol.
   *Open the file and save its handle.
   */
	int ret;
	/*! 
   *Will we use GZIP compression to send data?
   */
	int use_gzip=0;
  u_long filesize=0;
	File h;
	u_long bytes_to_send;
  u_long firstByte = td->request.RANGEBYTEBEGIN; 
  u_long lastByte = td->request.RANGEBYTEEND;
  int keepalive;


 	/*! gzip compression object.  */
	Gzip gzip;
	/*! Is the GZIP header still added to the buffer?  */
	u_long gzipheaderadded=0;
	
	/*! Number of bytes created by the zip compressor by loop.  */
	u_long gzip_dataused=0;
	u_long dataSent=0;

	ret  = h.openFile(filenamePath, FILE_OPEN_IFEXISTS | 
                   FILE_OPEN_READ);
	if(ret)
	{	
		return 0;
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
		lastByte = ((u_long)lastByte < bytes_to_send) ? lastByte : bytes_to_send;
	}

  /*! 
   *Use GZIP compression to send files bigger than GZIP threshold.  
   */
  if( ((Http*)td->lhttp)->getGzipThreshold() && 
      (bytes_to_send > ((Http*)td->lhttp)->getGzipThreshold() ))
	{
    use_gzip=1;
  }
	keepalive = !lstrcmpi(td->request.CONNECTION.c_str(),"Keep-Alive");

#ifndef DO_NOT_USE_GZIP
	/*! 
   *Be sure that the client accept GZIP compressed data.  
   */
	if(use_gzip)
		use_gzip &= (td->request.ACCEPTENC.find("gzip") != string::npos);
#else
	/*! 
   *If compiled without GZIP support force the server to don't use it.  
   */
	use_gzip=0;
#endif	
	if(td->appendOutputs)
		use_gzip=0;

	/*! 
   *bytes_to_send is the interval between the first and the last byte.  
   */
	bytes_to_send=lastByte-firstByte;

	/*! 
   *If failed to set the file pointer returns an internal server error.  
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
    use_gzip = 0;
	}

  /*! Specify the content length with keep-alive connections. */
	if(keepalive)
  {
    ostringstream buffer;
    buffer << (u_int)bytes_to_send;
    td->response.CONTENT_LENGTH.assign(buffer.str());
  }	
  else
		td->response.CONNECTION.assign("close");
	
	if(use_gzip)
	{
		/*! Do not use chunked transfer with old HTTP/1.0 clients.  */
		if(keepalive)
			td->response.TRANSFER_ENCODING.assign("chunked");
		td->response.CONTENT_ENCODING.assign("gzip");
	}

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
	if(only_header)
	{
		h.closeFile();
		return 1;
	}
	if(use_gzip)
		gzip.initialize(td->buffer2->GetBuffer(), 
                    td->buffer2->GetRealLength(), 
                    td->buffer->GetBuffer(), 
                    td->buffer->GetRealLength());
	for(;;)
	{
		u_long nbr;

		if(use_gzip)
		{
			gzip_dataused=0;
			u_long datatoread=(bytes_to_send < td->buffer2->GetRealLength()/2) 
        ? bytes_to_send : td->buffer2->GetRealLength()/2 ;
			/*! Read from the file the bytes to send.  */
			if(h.readFromFile(td->buffer2->GetBuffer(), datatoread, &nbr))
			{
        ostringstream buffer;
				h.closeFile();
        buffer << (int)dataSent;
        td->response.CONTENT_LENGTH.assign(buffer.str());
				return 0;
			}
      
			if(nbr)
			{
				if(gzipheaderadded==0)
				{
					gzip_dataused+=gzip.getHEADER(td->buffer->GetBuffer(), 
                                        td->buffer->GetLength());
					gzipheaderadded=1;
				}
				gzip_dataused+=gzip.compress(td->buffer2->GetBuffer(), 
                       nbr, &((td->buffer->GetBuffer())[gzip_dataused]),
                              td->buffer->GetRealLength()-gzip_dataused);
			}
			else
			{
				gzip_dataused=gzip.flush(td->buffer->GetBuffer(), 
                                      td->buffer->GetLength());
				gzip.free(td->buffer2->GetBuffer(), nbr, 
                  td->buffer->GetBuffer(), 
                  td->buffer->GetRealLength());
			}
			if(keepalive)
			{
        ostringstream buffer;
        buffer << hex << gzip_dataused << "\r\n";
				ret = s->socket.send(buffer.str().c_str(), buffer.str().length(), 0);
				if(ret == SOCKET_ERROR)
					break;
			}
			if(gzip_dataused)
			{
				ret=s->socket.send(td->buffer->GetBuffer(), gzip_dataused, 0);
				if(ret == SOCKET_ERROR)
					break;
				dataSent+=ret;
			}
			if(keepalive)
			{
				ret=s->socket.send("\r\n", 2, 0);
				if(ret == SOCKET_ERROR)
					break;
			}
      
		}
		else
		{
			/*! Read from the file the bytes to send. */
      ret = h.readFromFile(td->buffer->GetBuffer(),
                           (bytes_to_send<td->buffer->GetRealLength()? 
                            bytes_to_send :td->buffer->GetRealLength()  ), &nbr);
			if(ret)
			{
        ostringstream buffer;
				h.closeFile();
        buffer << dataSent;
        td->response.CONTENT_LENGTH.assign(buffer.str().c_str());
				return 0;
			}
      bytes_to_send-=nbr;
			/*! If there are bytes to send, send them. */
			if(nbr)
			{
				if(!td->appendOutputs)
				{
					ret=(u_long)s->socket.send(td->buffer->GetBuffer(), nbr, 0);
					if(ret==SOCKET_ERROR)
					{
            ostringstream buffer;
						h.closeFile();
            buffer << (int)dataSent;
            td->response.CONTENT_LENGTH.assign(buffer.str().c_str());
						return 0;
					}
          dataSent+=ret;
				}
				else
				{
          u_long nbw;
					ret = td->outputData.writeToFile(td->buffer->GetBuffer(), 
                                           nbr, &nbw);
				  if(ret)
					{
						h.closeFile();
            td->response.CONTENT_LENGTH.assign("0");
						return 0;
					}
          dataSent+=nbw;
					
				}
			}
    }
		/*! 
     *When the bytes number read from the file is zero, 
     *stop to send the file.  
     */
		if(nbr==0)
		{
			if(keepalive && use_gzip )
			{
				ret=s->socket.send("0\r\n\r\n", 5, 0);
				if(ret==SOCKET_ERROR)
				{
          ostringstream buffer;
					h.closeFile();
          buffer << dataSent;
          td->response.CONTENT_LENGTH.assign(buffer.str());
					return 0;
				}
			}
			break;
		}
	}/*End for loop. */
	h.closeFile();
  {
    ostringstream buffer;
    buffer << dataSent;
    td->response.CONTENT_LENGTH.assign(buffer.str());
  }
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
int HttpFile::load()
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
