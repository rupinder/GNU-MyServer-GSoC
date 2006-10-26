/*
MyServer
Copyright (C) 2002, 2003, 2004 The MyServer Team
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

#ifndef CLIENTSTHREAD_H
#define CLIENTSTHREAD_H
#include "../stdafx.h"
#include "../include/utility.h"
#include "../include/connection.h"
#include "../include/security.h"
#include "../include/http.h"
#include "../include/mem_buff.h"
#include "../include/https.h"
#include "../include/control_protocol.h"

class  ClientsThread
{
	friend class Server;

#ifdef WIN32
	friend  unsigned int __stdcall startClientsThread(void* pParam);
#endif
#ifdef HAVE_PTHREAD
	friend  void* startClientsThread(void* pParam);
#endif
private:
  int toDestroy;
  int timeout;
	int initialized;
  int staticThread;
	u_long id;
	int err;
  int parsing;
	int threadIsStopped;
	int threadIsRunning;
	u_long buffersize;
	u_long buffersize2;
	int isRunning();
	int isStopped();
	Http *httpParser;
	Https *httpsParser;
  ControlProtocol  *controlProtocolParser;
	MemBuf buffer;
	MemBuf buffer2;
	int controlConnections();
	u_long nBytesToRead;
public:
	enum RETURN_CODE
	{
		/*!
		 *Delete the current connection from the connections pool.  
		 */
		DELETE_CONNECTION = 0,
		/*!
		 *Keep the connection in the connections pool waiting for new data.  
		 */
		KEEP_CONNECTION = 1,
		/*!
		 *The request present in the connection buffer is not complete, keep
		 *data in the buffer and append to it.  
		 */
		INCOMPLETE_REQUEST = 2,
		/*!
		*The request present in the buffer is not complete, append to the buffer
		*and check before new data is present.
		*/
		INCOMPLETE_REQUEST_NO_WAIT = 3
	};
  ClientsThread *next;
	MemBuf *getBuffer();
	MemBuf *getBuffer2();
	const static u_long ID_OFFSET = 200;
	ClientsThread();
	~ClientsThread();
	void stop();
	void clean();	
  int getTimeout();
  void setTimeout(int);
  int isToDestroy();
  void setToDestroy(int);
  int isStatic();
  int isParsing();
  void setStatic(int);
};

#ifdef WIN32
unsigned int __stdcall startClientsThread(void* pParam); 
#endif

#ifdef HAVE_PTHREAD
void* startClientsThread(void* pParam);
#endif

#endif
