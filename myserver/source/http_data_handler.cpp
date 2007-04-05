/*
MyServer
Copyright (C) 2005, 2007 The MyServer Team
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "../include/http_headers.h"
#include "../include/http.h"
#include "../include/http_data_handler.h"

/*!
 *Send a file to the client using the HTTP protocol.
 */
int HttpDataHandler::send(HttpThreadContext*/* td*/, ConnectionPtr /*s*/, 
                          const char* /*filenamePath*/, const char* /*exec*/,
                          int /*onlyHeader*/)
{
  return 0;
}

/*!
 *Constructor for the class.
 */
HttpDataHandler::HttpDataHandler()
{

}

/*!
 *Destroy the object.
 */
HttpDataHandler::~HttpDataHandler()
{

}

/*!
 *Load the static elements.
 */
int HttpDataHandler::load(XmlParser* /*confFile*/)
{
  return 0;
}

/*!
 *Unload the static elements.
 */
int HttpDataHandler::unload()
{
  return 0;
}

/*!
 *Send data over the HTTP channel.
 *Return zero on success.
 *\param td The HTTP thread context.
 *\param buffer Data to send.
 *\param size Size of the buffer.
 *\param appendFile The file where append if in append mode.
 *\param chain Where send data if not append.
 *\param append Append to the file?
 *\param useChunks Can we use HTTP chunks to send data?
 */
int HttpDataHandler::appendDataToHTTPChannel(HttpThreadContext* td, 
																						 char* buffer, u_long size,
																						 File* appendFile, 
																						 FiltersChain* chain,
																						 bool append, 
																						 bool useChunks)
{
	u_long nbw;
	if(append)
	{
	  return appendFile->writeToFile(buffer, size, &nbw);
	}
	else
	{
		if(useChunks)
		{
			ostringstream chunkHeader;
			chunkHeader << hex << size << "\r\n"; 
			if(chain->write(chunkHeader.str().c_str(), 
											chunkHeader.str().length(), &nbw))
				return 1;
		}

		if(chain->write(buffer, size, &nbw))
			return 1;

		return chain->write("\r\n", 2, &nbw);

	}
	return 1;
}
