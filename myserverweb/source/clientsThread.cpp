/*
*MyServer
*Copyright (C) 2002 The MyServer Team
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

#include "../stdafx.h"
#include "../include/clientsThread.h"
#include "../include/cserver.h"
#include "../include/security.h"
#include "../include/Response_RequestStructs.h"
#include "../include/sockets.h"
#include "../include/stringutils.h"
#include "../include/MemBuf.h"

#ifdef NOT_WIN
extern "C" {
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include <sys/types.h>
#include <sys/wait.h>

}
#endif

// Define SD_BOTH in case it is not defined
#ifndef SD_BOTH
#define SD_BOTH 2 /*! to close tcp connection in both directions */
#endif

ClientsTHREAD::ClientsTHREAD()
{
	err=0;
	initialized=0;
}
ClientsTHREAD::~ClientsTHREAD()
{
	clean();
}
/*!
*This function starts a new thread controlled by a ClientsTHREAD class instance.
*/
#ifdef WIN32
unsigned int __stdcall startClientsTHREAD(void* pParam)
#endif
#ifdef HAVE_PTHREAD
void * startClientsTHREAD(void* pParam)
#endif
{
#ifdef NOT_WIN
	// Block SigTerm, SigInt, and SigPipe in threads
	sigset_t sigmask;
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGPIPE);
	sigaddset(&sigmask, SIGINT);
	sigaddset(&sigmask, SIGTERM);
	sigprocmask(SIG_SETMASK, &sigmask, NULL);
#endif
	u_long id=*((u_long*)pParam) - ClientsTHREAD::ID_OFFSET;

	ClientsTHREAD *ct=&lserver->threads[id];
	if(ct->initialized)
		return 0;
	ct->threadIsRunning=1;
	ct->threadIsStopped=0;
	ct->buffersize=lserver->buffersize;
	ct->buffersize2=lserver->buffersize2;
	
	ct->buffer.SetLength(ct->buffersize);
	ct->buffer2.SetLength(ct->buffersize2);
	
	ct->http_parser = new http();
	ct->https_parser = new https();
	
	ct->initialized=1;

	memset((char*)ct->buffer.GetBuffer(), 0, ct->buffer.GetRealLength());
	memset((char*)ct->buffer2.GetBuffer(), 0,ct->buffer2.GetRealLength());
	
	wait(5000);
	/*!
	*This function when is alive only call the controlConnections(...) function
	*of the ClientsTHREAD class instance used for control the thread.
	*/
	while(ct->threadIsRunning) 
	{
		ct->controlConnections();
		wait(1);
	}
	ct->threadIsStopped=1;
	
	myserver_thread::terminate();
	return 0;
}

/*!
*This is the main loop of the thread.
*Here are controlled all the connections that belongs to the ClientsTHREAD class instance.
*Every connection is controlled by its protocol.
*/
void ClientsTHREAD::controlConnections()
{
	static int i=0;
	/*!
	*Get the access to the connections list.
	*/
	lserver->connections_mutex_lock();
	LPCONNECTION c=lserver->getConnectionToParse(this->id);
	/*!
	*Check if c exists.
	*Check if c is a valid connection structure.
	*Do not parse a connection that is going to be parsed by another thread.
	*/
	if((!c) || (c->check_value!=CONNECTION::check_value_const) || c->parsing)
	{
		lserver->connections_mutex_unlock();
		return;
	}
	
	/*!
	*Set the connection parsing flag to true.
	*/
	c->parsing=1;
	
	/*!
	*Unlock connections list access after setting parsing flag.
	*/
	lserver->connections_mutex_unlock();
	nBytesToRead=c->socket.bytesToRead();/*!Number of bytes waiting to be read*/
	if(nBytesToRead || c->forceParsing)
	{
		c->forceParsing=0;
		if(nBytesToRead)
			err=c->socket.recv(&((char*)(buffer.GetBuffer()))[c->dataRead],KB(8) - c->dataRead, 0);
		if(err==-1)
		{
			lserver->deleteConnection(c,this->id);
			return;
		}
		if((c->dataRead+err)<KB(8))
		{
			((char*)buffer.GetBuffer())[c->dataRead+err]='\0';
		}
		else
		{
			lserver->deleteConnection(c,this->id);
			return;
		}
		buffer.SetBuffer(c->connectionBuffer, c->dataRead);
		buffer.SetLength( c->dataRead);
		/*!
		*Control the protocol used by the connection.
		*/
		int retcode=0;
		c->thread=this;
		switch(((vhost*)(c->host))->protocol)
		{
			/*!
			*controlHTTPConnection returns 0 if the connection must be removed from
			*the active connections list.
			*/
			case PROTOCOL_HTTP:
				retcode=http_parser->controlConnection(c,(char*)buffer.GetBuffer(),(char*)buffer2.GetBuffer(),buffer.GetLength(),buffer2.GetLength(),nBytesToRead,id);
				break;
			/*!
			*Parse an HTTPS connection request.
			*/
			case PROTOCOL_HTTPS:
				retcode=https_parser->controlConnection(c,(char*)buffer.GetBuffer(),(char*)buffer2.GetBuffer(),buffer.GetLength(),buffer2.GetLength(),nBytesToRead,id);
				break;
			default:
				dynamic_protocol* dp=lserver->getDynProtocol(((vhost*)(c->host))->protocol_name);
				if(dp==0)
				{
					retcode=0;
				}
				else
				{
					retcode=dp->controlConnection(c,(char*)buffer.GetBuffer(),(char*)buffer2.GetBuffer(),buffer.GetLength(),buffer2.GetLength(),nBytesToRead,id);
				}
				break;
		}
		/*!
		*The protocols parser functions return:
		*0 to delete the connection from the active connections list
		*1 to keep the connection active and clear the connectionBuffer
		*2 if the header is incomplete and to save it in a temporary buffer
		*3 if the header is incomplete without save it in a temporary buffer
		*/
		if(retcode==0)/*Delete the connection*/
		{
			lserver->deleteConnection(c,this->id);
			return;
		}
		else if(retcode==1)/*Keep the connection*/
		{
			c->dataRead=0;
			c->connectionBuffer[0]='\0';
		}
		else if(retcode==2)/*Incomplete request to buffer*/
		{
			/*!
			*If the header is incomplete save the current received
			*data in the connection buffer
			*/
			memcpy(c->connectionBuffer,(char*)buffer.GetBuffer(),c->dataRead+err);/*!Save the header in the connection buffer*/
			c->dataRead+=err;
		}
		else if(retcode==3)/*Incomplete request yet in the buffer*/
		{
			c->forceParsing=1;
		}		
		c->timeout=get_ticks();
	}
	else
	{
		/*!
		*Reset nTries after 5 seconds.
		*/
		if(get_ticks()-c->timeout>5000)
			c->nTries=0;
		/*!
		*If the connection is inactive for a time greater that the value
		*configured remove the connection from the connections pool
		*/
		if((get_ticks()- c->timeout) > lserver->connectionTimeout)
		{
			lserver->deleteConnection(c,this->id);
			return;
		}
	}
	/*!
	*Reset the parsing flag.
	*/
	c->parsing=0;
}

/*!
*Stop the thread
*/
void ClientsTHREAD::stop()
{
	/*!
	*Set the run flag to False
	*When the current thread find the threadIsRunning
	*flag setted to 0 automatically destroy the
	*thread.
	*/
	threadIsRunning=0;
}

/*!
*Clean the memory used by the thread.
*/
void ClientsTHREAD::clean()
{
	if(initialized==0)/*!If the thread was not initialized return from the clean function*/
		return;
	threadIsRunning=0;
	buffer.Free();
	buffer2.Free();
	
	delete http_parser;
	delete https_parser;
	initialized=0;
}


/*!
*Returns a non-null value if the thread is active.
*/
int ClientsTHREAD::isRunning()
{
	return threadIsRunning;
}

/*!
*Returns 1 if the thread is stopped.
*/
int ClientsTHREAD::isStopped()
{
	return threadIsStopped;
}

/*!
*Get a pointer to the buffer.
*/
CMemBuf* ClientsTHREAD::GetBuffer()
{
	return &buffer;
}
/*!
*Get a pointer to the buffer2.
*/
CMemBuf *ClientsTHREAD::GetBuffer2()
{
	return &buffer2;
}
