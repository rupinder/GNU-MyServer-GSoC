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
#include "..\include\sockets.h"

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
	terminateAccess(&ct->connectionWriteAccess,ct->id);
	while(ct->threadIsRunning) 
	{
		ct->controlConnections();
	}
	_endthreadex(0);
	return 0;
}
/*
*This is the main loop of the thread
*Here are controlled all the connections that belongs to the ClientsTHREAD class instance.
*Every connection is controlled by its protoco
*/
void ClientsTHREAD::controlConnections()
{
	requestAccess(&connectionWriteAccess,this->id);
	LPCONNECTION c=connections;
	BOOL logonStatus;
	for(c; c ;c=c->Next)
	{
		if(bytesToRead(c->socket))
		{
			logon(c,&logonStatus,&hImpersonation);
			err=ms_recv(c->socket,buffer,buffersize, 0);

			if((err==0) || (err==SOCKET_ERROR)||(err==WSAECONNABORTED)||(err==WSAENOTCONN))
			{
				if(deleteConnection(c))
					continue;
			}
			if(err!=WSAETIMEDOUT)
			{
				/*
				*Control the protocol used by the connection
				*/
				switch(c->protocol)
				{
					/*
					*controlHTTPConnection returns 0 if the connection must be removed from
					*the active connections list
					*/
					case PROTOCOL_HTTP:
						if(!controlHTTPConnection(c,buffer,buffer2,buffersize,buffersize2,nBytesToRead,&hImpersonation))
							deleteConnection(c);
						continue;
				}
			}
			c->timeout=clock();
			logout(logonStatus,&hImpersonation);
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
	*flag setted to FALSE automatically destroy the
	*thread
	*/
	threadIsRunning=FALSE;
}

/*
*Clean the memory used by the thread
*/
void ClientsTHREAD::clean()
{
	if(initialized==FALSE)
		return;
	requestAccess(&connectionWriteAccess,this->id);
	if(connections)
	{
		clearAllConnections();
	}
	free(buffer);
	free(buffer2);
	initialized=FALSE;
	terminateAccess(&connectionWriteAccess,this->id);

}

/*
*Add a new connection.
*Connections are defined using a CONNECTION struct.
*/
LPCONNECTION ClientsTHREAD::addConnection(MYSERVER_SOCKET s,CONNECTION_PROTOCOL protID,char *ipAddr)
{
	requestAccess(&connectionWriteAccess,this->id);
	const int maxRcvBuffer=KB(5);
	const BOOL keepAlive=TRUE;
	LPCONNECTION nc=(CONNECTION*)malloc(sizeof(CONNECTION));
	ZeroMemory(nc,sizeof(CONNECTION));
	nc->socket=s;
	nc->protocol=protID;
	lstrcpy(nc->ipAddr,ipAddr);
	nc->Next=connections;
	connections=nc;
	nConnections++;
	terminateAccess(&connectionWriteAccess,this->id);

	return nc;
}

/*
*Delete a connection
*/
BOOL ClientsTHREAD::deleteConnection(LPCONNECTION s)
{
	requestAccess(&connectionWriteAccess,this->id);
	BOOL ret=FALSE;
	ms_shutdown(s->socket,SD_BOTH );
	do
	{
		err=ms_recv(s->socket,buffer,buffersize,0);
	}while(err && (err!=SOCKET_ERROR));
	ms_closesocket(s->socket); 
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
	terminateAccess(&connectionWriteAccess,this->id);
	return ret;
}

/*
*Delete all the connections
*/
void ClientsTHREAD::clearAllConnections()
{

	requestAccess(&connectionWriteAccess,this->id);
	LPCONNECTION c=connections;
	for(;c;c=c->Next)
	{
		deleteConnection(c);
	}
	connections=NULL;
	nConnections=0;
	terminateAccess(&connectionWriteAccess,this->id);
}


/*
*Find a connection passing the socket that control it
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
