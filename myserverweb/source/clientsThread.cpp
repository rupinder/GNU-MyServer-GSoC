/*
*myServer
*Copyright (C) 2002 Giuseppe Scrivano
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
#endif
#ifdef __linux__
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
	ct->buffer=new char[ct->buffersize];
	ct->buffer2=new char[ct->buffersize2];
	ct->initialized=true;

	memset(ct->buffer, 0, ct->buffersize);
	memset(ct->buffer2, 0, ct->buffersize2);

	terminateAccess(&ct->connectionWriteAccess,ct->id);
	/*
	*This function when is alive only call the controlConnections(...) function
	*of the ClientsTHREAD class instance used for control the thread.
	*/
	while(ct->threadIsRunning) 
	{
		ct->controlConnections();

#ifdef WIN32
		Sleep(1);
#endif
#ifdef __linux__
		usleep(1);
#endif
	}
	ct->threadIsStopped=true;
#ifdef WIN32
	_endthread();
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
	requestAccess(&connectionWriteAccess,this->id);
	LPCONNECTION c=connections;
	LPCONNECTION next=0;
	int logonStatus;
	for(c; c && connections ;c=next)
	{
		next=c->Next;
		nBytesToRead=c->socket.bytesToRead();/*Number of bytes waiting to be read*/
		if(nBytesToRead)
		{
			err=c->socket.recv(&buffer[c->dataRead],KB(5), 0);
			if(err==-1)
			{
				deleteConnection(c);
					continue;
			}
			if((c->dataRead+err)<KB(5))
			{
				buffer[c->dataRead+err]='\0';
			}
			/*
			*Control the protocol used by the connection.
			*/
			int retcode=0;
			switch(((vhost*)(c->host))->protocol)
			{
				/*
				*controlHTTPConnection returns 0 if the connection must be removed from
				*the active connections list.
				*/
				case PROTOCOL_HTTP:
					retcode=controlHTTPConnection(c,buffer,buffer2,buffersize,buffersize2,nBytesToRead,id);
					if(retcode==0)
					{
						deleteConnection(c);
					}
					else if(retcode==1)
					{
						c->dataRead=0;
					}
					else if(retcode==2)
					{
						c->dataRead+=err;
					}
					break;
			}
			c->timeout=clock();
		}
		else
		{
			if((clock()- c->timeout) > lserver->connectionTimeout)
				if(deleteConnection(c))
					continue;
		}
	}
	terminateAccess(&connectionWriteAccess,this->id);
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
	requestAccess(&connectionWriteAccess,this->id);
	if(connections)
	{
		clearAllConnections();
	}
	delete[] buffer;
	delete[] buffer2;
	buffer=buffer2=0;
	initialized=false;
	terminateAccess(&connectionWriteAccess,this->id);

}

/*
*Add a new connection.
*A connection is defined using a CONNECTION struct.
*/
LPCONNECTION ClientsTHREAD::addConnection(MYSERVER_SOCKET s,MYSERVER_SOCKADDRIN *asock_in,char *ipAddr,char *localIpAddr,int port,int localPort)
{
	requestAccess(&connectionWriteAccess,this->id);
	LPCONNECTION nc=new CONNECTION;
	memset(nc, 0, sizeof(CONNECTION));
	nc->socket=s;
	nc->port=(u_short)port;
	nc->timeout=clock();
	nc->dataRead = 0;
	nc->localPort=(u_short)localPort;
	lstrcpy(nc->ipAddr,ipAddr);
	lstrcpy(nc->localIpAddr,localIpAddr);
	nc->Next=connections;
	nc->host=(void*)lserver->vhostList.getvHost(0,localIpAddr,(u_short)localPort);
	if(nc->host==0)
	{
		delete nc;
		return 0;
	}
    connections=nc;
	nConnections++;


	char msg[500];
#ifdef WIN32
	sprintf(msg, "%s:%s ->%s %s:", msgNewConnection, inet_ntoa(asock_in->sin_addr), lserver->getServerName(), msgAtTime);
	getRFC822GMTTime(&msg[strlen(msg)],HTTP_RESPONSE_DATE_DIM);
	strcat(msg,"\r\n");

#endif
#ifdef __linux__
	snprintf(msg, 500,"%s:%s ->%s %s:", msgNewConnection, inet_ntoa(asock_in->sin_addr), lserver->getServerName(), msgAtTime);
	getRFC822GMTTime(&msg[strlen(msg)],HTTP_RESPONSE_DATE_DIM);
	strcat(msg,"\r\n");

#endif
	((vhost*)(nc->host))->accessesLogWrite(msg);

	if(nc==0)
	{
		if(lserver->getVerbosity()>0)
		{
#ifdef WIN32
			sprintf(msg, "%s:%s ->%s %s:", msgErrorConnection, inet_ntoa(asock_in->sin_addr), lserver->getServerName(), msgAtTime);
			getRFC822GMTTime(&msg[strlen(msg)],HTTP_RESPONSE_DATE_DIM);
			strcat(msg,"\r\n");
#endif
#ifdef __linux__
			snprintf(msg, 500,"%s:%s ->%s %s:", msgErrorConnection, inet_ntoa(asock_in->sin_addr), lserver->getServerName(), msgAtTime);
			getRFC822GMTTime(&msg[strlen(msg)],HTTP_RESPONSE_DATE_DIM);
			strcat(msg,"\r\n");
#endif
			((vhost*)(nc->host))->warningsLogWrite(msg);
		}
	}
	terminateAccess(&connectionWriteAccess,this->id);
	return nc;
}

/*
*Delete a connection.
*/
int ClientsTHREAD::deleteConnection(LPCONNECTION s)
{
	if(!s)
		return 0;
	requestAccess(&connectionWriteAccess,this->id);
	int ret=false;
	/*
	*First of all close the socket communication.
	*/
	s->socket.shutdown(SD_BOTH );
	do
	{
		err=s->socket.recv(buffer,buffersize,0);
	}while(err!=-1);
	while(s->socket.closesocket());
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
			delete i;
			ret=true;
			break;
		}
		else
		{
			prev=i;
		}
	}
	nConnections--;
	terminateAccess(&connectionWriteAccess,this->id);
	return ret;
}

/*
*Delete all the connections.
*/
void ClientsTHREAD::clearAllConnections()
{

	requestAccess(&connectionWriteAccess,this->id);
	LPCONNECTION c=connections;
	LPCONNECTION next=0;
	for(u_long i=0;c && i<nConnections;i++)
	{
		next=c->Next;
		deleteConnection(c);
		c=next;
	}
	nConnections=0;
	connections=NULL;
	terminateAccess(&connectionWriteAccess,this->id);
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
