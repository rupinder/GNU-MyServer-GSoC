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
#include "../include/sockets.h"
#include "../include/stringutils.h"

#ifndef WIN32
extern "C" {
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#ifdef __linux__
#include <pthread.h>
#endif
}
#endif

// Define SD_BOTH in case it is not defined
#ifndef SD_BOTH
#define SD_BOTH 2 /*! to close tcp connection in both directions */
#endif

ClientsTHREAD::ClientsTHREAD()
{
	err=0;
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
#ifdef __linux__
void * startClientsTHREAD(void* pParam)
#endif
{
	u_long id=*((u_long*)pParam) - ClientsTHREAD::ID_OFFSET;

	ClientsTHREAD *ct=&lserver->threads[id];
	ct->threadIsRunning=1;
	ct->threadIsStopped=0;
	ct->buffersize=lserver->buffersize;
	ct->buffersize2=lserver->buffersize2;
	ct->buffer=new char[ct->buffersize];
	ct->buffer2=new char[ct->buffersize2];
	ct->initialized=1;

	memset(ct->buffer, 0, ct->buffersize);
	memset(ct->buffer2, 0, ct->buffersize2);

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
#ifdef WIN32
	_endthread();
#endif
#ifdef __linux__
	pthread_exit(0);
#endif
	return 0;
}
/*!
*This is the main loop of the thread.
*Here are controlled all the connections that belongs to the ClientsTHREAD class instance.
*Every connection is controlled by its protocol.
*/
void ClientsTHREAD::controlConnections()
{
	LPCONNECTION c=lserver->getConnectionToParse(this->id);
	/*
	*Check if c exists
	*/
	if(!c)
		return;
	/*
	*Do not parse a connection that is parsed by another thread
	*/
	if(c->parsing==1)
		return;
	/*
	*Check if c is a valid connection structure
	*/
	if(c->check_value!=CONNECTION::check_value_const)
		return;
	c->parsing=1;
	nBytesToRead=c->socket.bytesToRead();/*!Number of bytes waiting to be read*/
	if(nBytesToRead)
	{
		err=c->socket.recv(&buffer[c->dataRead],KB(8)-c->dataRead, 0);
		if(err==-1)
		{
			lserver->deleteConnection(c,this->id);
			return;
		}
		if((c->dataRead+err)<KB(8))
		{
			buffer[c->dataRead+err]='\0';
		}
		else
		{
			lserver->deleteConnection(c,this->id);
			return;
		}
		memcpy(buffer,c->connectionBuffer,c->dataRead);
		/*!
		*Control the protocol used by the connection.
		*/
		int retcode=0;
		switch(((vhost*)(c->host))->protocol)
		{
			/*!
			*controlHTTPConnection returns 0 if the connection must be removed from
			*the active connections list.
			*/
			case PROTOCOL_HTTP:
				retcode=controlHTTPConnection(c,buffer,buffer2,buffersize,buffersize2,nBytesToRead,id);
				break;
			/*!
			*Use the same parser for the HTTPS protocol too.
			*/
			case PROTOCOL_HTTPS:
				retcode=controlHTTPConnection(c,buffer,buffer2,buffersize,buffersize2,nBytesToRead,id);
				break;
			default:
				retcode=0;
				break;
		}
		/*!
		*The protocols parser functions return:
		*0 to delete the connection from the active connections list
		*1 to keep the connection active and clear the connectionBuffer
		*2 if the header is incomplete and to save it in a temporary buffer
		*/
		if(retcode==0)
		{
			lserver->deleteConnection(c,this->id);
			return;
		}
		else if(retcode==1)
		{
			c->dataRead=0;
		}
		else if(retcode==2)
		{
			/*!
			*If the header is incomplete save the current received
			*data in the connection buffer
			*/
			memcpy(c->connectionBuffer,buffer,c->dataRead+err);/*!Save the header in the connection buffer*/
			c->dataRead+=err;
				
		}
		c->timeout=clock();
	}
	else
	{
		if(clock()-c->timeout>5000)
			c->nTries=0;
		/*!
		*If the connection is inactive for a time greater that the value
		*configured remove the connection from the connections pool
		*/
		if((clock()- c->timeout) > lserver->connectionTimeout)
		{
			lserver->deleteConnection(c,this->id);
			return;
		}
	}
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
	if(buffer)
		delete[] buffer;
	if(buffer2)
		delete[] buffer2;
	buffer=buffer2=0;
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
