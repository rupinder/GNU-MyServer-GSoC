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



#include "..\stdafx.h"
#include "..\include\clientsTHREAD.h"
#include "..\include\cserver.h"
#include "..\include\security.h"

ClientsTHREAD::ClientsTHREAD()
{
	err=0;
}
ClientsTHREAD::~ClientsTHREAD()
{
	clean();
}
unsigned int __stdcall startClientsTHREAD(void* pParam)
{
	DWORD id=*((DWORD*)pParam);
	ClientsTHREAD *ct=&lserver->threads[id];
	ct->threadIsRunning=TRUE;
	ct->connections=0;
	ct->buffersize=lserver->buffersize;
	ct->buffersize2=lserver->buffersize2;
	ct->buffer=(char*)malloc(ct->buffersize);
	ct->buffer2=(char*)malloc(ct->buffersize2);
	ct->initialized=TRUE;
	ZeroMemory(ct->buffer,ct->buffersize);
	ZeroMemory(ct->buffer2,ct->buffersize2);
	char mutexname[20];
	ct->connectionWriteAccess=0;
	while(ct->threadIsRunning) 
	{
		ct->controlConnections();
	}
	_endthreadex(0);
	return 0;
}

void ClientsTHREAD::controlConnections()
{
	while(connectionWriteAccess);
	connectionWriteAccess=this->id;
	LPCONNECTION c=connections;
	BOOL logonStatus;
	for(c; c ;c=c->Next)
	{
		ioctlsocket(c->socket,FIONREAD,&nBytesToRead);
		if(nBytesToRead)
		{
			logon(c,&logonStatus,&hImpersonation);
			err=recv(c->socket,buffer,buffersize, 0);

			if((err==0) || (err==SOCKET_ERROR)||(err==WSAECONNABORTED)||(err==WSAENOTCONN))
			{
				if(deleteConnection(c))
					continue;
			}
			if(err!=WSAETIMEDOUT)
			{
				switch(c->protocol)
				{
					/*
					*controlHTTPConnection returns 0 if the connection must be removed from
					*the active connections list
					*/
					case PROTOCOL_HTTP:
						if(controlHTTPConnection(c,buffer,buffer2,buffersize,buffersize2,nBytesToRead,&hImpersonation))
							deleteConnection(c);
						continue;
				}
			}
			c->timeout=getTime();
			logout(logonStatus,&hImpersonation);
		}
		else
		{
			if( getTime()- c->timeout > lserver->connectionTimeout)
				if(deleteConnection(c))
					continue;
		}
	}
	connectionWriteAccess=0;
	
}
void ClientsTHREAD::stop()
{
	/*
	*Set the run flag to False
	*When the current thread find the threadIsRunning
	*flag setted to FALSE automatically destroy the
	*thread
	*/
	threadIsRunning=FALSE;
}
void ClientsTHREAD::clean()
{
	/*
	*Clean the memory used by the thread
	*/
	if(initialized==FALSE)
		return;
	while(connectionWriteAccess);
	connectionWriteAccess=this->id;
	if(connections)
	{
		clearAllConnections();
	}
	free(buffer);
	free(buffer2);
	initialized=FALSE;
	connectionWriteAccess=0;
}
LPCONNECTION ClientsTHREAD::addConnection(SOCKET s,CONNECTION_PROTOCOL protID)
{
	/*
	*Add a new connection.
	*Connections are defined using a CONNECTION struct.
	*/
	while(connectionWriteAccess);
	connectionWriteAccess=this->id;
	const int maxRcvBuffer=KB(5);
	const BOOL keepAlive=TRUE;
	setsockopt(s,SOL_SOCKET,SO_RCVBUF,(char*)&maxRcvBuffer,sizeof(maxRcvBuffer));
	setsockopt( s,SOL_SOCKET, SO_SNDTIMEO,(char *)&lserver->socketRcvTimeout,sizeof(lserver->socketRcvTimeout));
	setsockopt( s,SOL_SOCKET, SO_KEEPALIVE,(char *)&keepAlive,sizeof(keepAlive));
	LPCONNECTION nc=(CONNECTION*)malloc(sizeof(CONNECTION));
	ZeroMemory(nc,sizeof(CONNECTION));
	nc->socket=s;
	nc->protocol=protID;
	nc->Next=connections;
	connections=nc;
	nConnections++;
	connectionWriteAccess=0;
	return nc;
}
BOOL ClientsTHREAD::deleteConnection(LPCONNECTION s)
{
	/*
	*Delete a connection
	*/
	while(connectionWriteAccess);
	connectionWriteAccess=this->id;
	BOOL ret=FALSE;
	shutdown(s->socket,SD_BOTH );
	do
	{
		err=recv(s->socket,buffer,buffersize,0);
	}while(err && (err!=SOCKET_ERROR));
	closesocket(s->socket); 
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
			ret=TRUE;
			break;
		}
		prev=i;
	}
	nConnections--;
	connectionWriteAccess=0;
	return ret;
}
void ClientsTHREAD::clearAllConnections()
{
	while(connectionWriteAccess);
	connectionWriteAccess=this->id;
	LPCONNECTION c=connections;
	for(;c;c=c->Next)
	{
		deleteConnection(c);
	}
	connections=NULL;
	nConnections=0;
	connectionWriteAccess=0;
}



LPCONNECTION ClientsTHREAD::findConnection(SOCKET a)
{
	/*
	*Find a connection passing the socket that control it
	*/
	LPCONNECTION c;
	for(c=connections;c;c=c->Next)
	{
		if(c->socket==a)
			return c;
	}
	return NULL;
}
