/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
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

#include "../include/connectionstruct.h"

/*!
 *Contructor for the CONNECTION class.
 */
CONNECTION::CONNECTION()
{
  thread=0;
  parsing=0;
  login[0]='\0';
  password[0]='\0';
  nTries=0;
	ipAddr[0]='\0';
  localIpAddr[0]='\0';
  port = 0;
	localPort = 0;
  timeout = 0;
  next = 0;
  host = 0;
	dataRead = 0;
  toRemove = 0;
  forceParsing = 0;
  connectionBuffer[0]='\0';
  protocolBuffer = 0;

}

/*!
 *Destroy the object.
 */
CONNECTION::~CONNECTION()
{
	socket.shutdown(SD_BOTH);
	char buffer[256];
	int buffersize=256;
  int err;
	do
	{
		err=socket.recv(buffer, buffersize, 0);
	}while((err!=-1) && err);
	socket.closesocket();

  if(protocolBuffer)
    delete [] protocolBuffer;

}
