/*
*myServer
*Copyright (C) Rocky_10_Balboa
*This library is free software; you can redistribute it and/or
*modify it under the terms of the GNU Library General Public
*License as published by the Free Software Foundation; either
*version 2 of the License, or (at your option) any later version.

*This library is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*Library General Public License for more details.

*You should have received a copy of the GNU Library General Public
*License along with this library; if not, write to the
*Free Software Foundation, Inc., 59 Temple Place - Suite 330,
*Boston, MA  02111-1307, USA.
*/

#include "../stdafx.h"
#include "../include/clientsThread.h"
#include "../include/cserver.h"
#include "../include/security.h"
#include "../include/sockets.h"
#include "../include/stringutils.h"

#ifndef WIN32
extern "C" {
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
#define SD_BOTH 2 /* to close tcp connection in both directions */
#endif

ClientsTHREAD::ClientsTHREAD()
{
	err=0;
}
ClientsTHREAD::~ClientsTHREAD()
{
	clean();
}
/*
*This function starts a new thread controlled by a ClientsTHREAD class instance.
*/
#ifdef WIN32
unsigned int __stdcall startClientsTHREAD(void* pParam)
#else
void * startClientsTHREAD(void* pParam)
#endif
{
	u_long id=*((u_long*)pParam);
	ClientsTHREAD *ct=&lserver->threads[id];
	ct->threadIsRunning=true;
	ct->connections=0;
	ct->threadIsStopped=false;
	ct->buffersize=lserver->buffersize;
	ct->buffersize2=lserver->buffersize2;
	ct->buffer=(char*)malloc(ct->buffersize);
	ct->buffer2=(char*)malloc(ct->buffersize2);
	ct->initialized=true;
#ifdef WIN32
	ZeroMemory(ct->buffer,ct->buffersize);
	ZeroMemory(ct->buffer2,ct->buffersize2);
#else
	memset(ct->buffer, 0, ct->buffersize);
	memset(ct->buffer2, 0, ct->buffersize2);
#endif
	ms_terminateAccess(&ct->connectionWriteAccess,ct->id);
	/*
	*This function when is alive only call the controlConnections(...) function
	*of the ClientsTHREAD class instance used for control the thread.
	*/
	while(ct->threadIsRunning) 
	{
		ct->controlConnections();

#ifdef WIN32
		Sleep(1);
#else
		usleep(1);
#endif
	}
	ct->threadIsStopped=true;
#ifdef WIN32
	_endthreadex(0);
#endif
#ifdef __linux__
	pthread_exit(0);
#endif
	return 0;
}
/*
*This is the main loop of the thread.
*Here are controlled all the connections that belongs to the ClientsTHREAD class instance.
*Every connection is controlled by its protocol.
*/
void ClientsTHREAD::controlConnections()
{
	ms_requestAccess(&connectionWriteAccess,this->id);
	LPCONNECTION c=connections;
	LPCONNECTION next=0;
	int logonStatus;
	for(c; c && connections ;c=next)
	{
		next=c->Next;
		nBytesToRead=ms_bytesToRead(c->socket);/*Number of bytes waiting to be read*/
		if(nBytesToRead)
		{
			ms_logon(c,&logonStatus,&hImpersonation);
			err=ms_recv(c->socket,buffer,KB(2), 0);
			if(err==-1)
			{
				if(deleteConnection(c))
					continue;
			}

			/*
			*Control the protocol used by the connection.
			*/
			switch(c->protocol)
			{
				/*
				*controlHTTPConnection returns 0 if the connection must be removed from
				*the active connections list.
				*/
				case PROTOCOL_HTTP:
					if(!controlHTTPConnection(c,buffer,buffer2,buffersize,buffersize2,nBytesToRead,&hImpersonation,id))
						deleteConnection(c);
					break;
			}
			c->timeout=clock();
			ms_logout(logonStatus,&hImpersonation);
		}
		else
		{
			if((clock()- c->timeout) > lserver->connectionTimeout)
				if(deleteConnection(c))
					continue;
		}
	}
	ms_terminateAccess(&connectionWriteAccess,this->id);
}
void ClientsTHREAD::stop()
{
	/*
	*Set the run flag to False
	*When the current thread find the threadIsRunning
	*flag setted to false automatically destroy the
	*thread.
	*/
	threadIsRunning=false;
}

/*
*Clean the memory used by the thread.
*/
void ClientsTHREAD::clean()
{
	if(initialized==false)
		return;
	ms_requestAccess(&connectionWriteAccess,this->id);
	if(connections)
	{
		clearAllConnections();
	}
	free(buffer);
	free(buffer2);
	initialized=false;
	ms_terminateAccess(&connectionWriteAccess,this->id);

}

/*
*Add a new connection.
*Connections are defined using a CONNECTION struct.
*/
LPCONNECTION ClientsTHREAD::addConnection(MYSERVER_SOCKET s,CONNECTION_PROTOCOL protID,char *ipAddr,char *localIpAddr,int port)
{
	ms_requestAccess(&connectionWriteAccess,this->id);
	LPCONNECTION nc=(CONNECTION*)malloc(sizeof(CONNECTION));
#ifdef WIN32
	ZeroMemory(nc,sizeof(CONNECTION));
#else
	memset(nc, 0, sizeof(CONNECTION));
#endif
	nc->socket=s;
	nc->port=(u_short)port;
	nc->timeout=clock();
	nc->protocol=protID;
	lstrcpy(nc->ipAddr,ipAddr);
	lstrcpy(nc->localIpAddr,localIpAddr);
	nc->Next=connections;
	connections=nc;
	nConnections++;
	ms_terminateAccess(&connectionWriteAccess,this->id);
	return nc;
}

/*
*Delete a connection.
*/
int ClientsTHREAD::deleteConnection(LPCONNECTION s)
{
	ms_requestAccess(&connectionWriteAccess,this->id);
	int ret=false;
	/*
	*First of all close the socket communication.
	*/
	ms_shutdown(s->socket,SD_BOTH );
	do
	{
		err=ms_recv(s->socket,buffer,buffersize,0);
	}while(err!=-1);
	while(ms_closesocket(s->socket));
	/*
	*Then remove the connection from the active connections list.
	*/
	LPCONNECTION prev=0;
	for(LPCONNECTION i=connections;i;i=i->Next)
	{
		if(i->socket == s->socket)
		{
			if(prev)
				prev->Next=i->Next;
			else
				connections=i->Next;
			free(i);
			ret=true;
			break;
		}
		else
		{
			prev=i;
		}
	}
	nConnections--;
	ms_terminateAccess(&connectionWriteAccess,this->id);
	return ret;
}

/*
*Delete all the connections.
*/
void ClientsTHREAD::clearAllConnections()
{

	ms_requestAccess(&connectionWriteAccess,this->id);
	LPCONNECTION c=connections;
	LPCONNECTION next=0;
	for(u_long i=0;i<nConnections;i++)
	{
		next=c->Next;
		deleteConnection(c);
		c=next;
	}
	nConnections=0;
	connections=NULL;
	ms_terminateAccess(&connectionWriteAccess,this->id);
}


/*
*Find a connection passing the socket that control it.
*/
LPCONNECTION ClientsTHREAD::findConnection(MYSERVER_SOCKET a)
{
	LPCONNECTION c;
	for(c=connections;c;c=c->Next)
	{
		if(c->socket==a)
			return c;
	}
	return NULL;
}

/*
*Returns true if the thread is active.
*/
int ClientsTHREAD::isRunning()
{
	return threadIsRunning;
}

/*
*Returns true if the thread is stopped.
*/
int ClientsTHREAD::isStopped()
{
	return threadIsStopped;
}
