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
	u_long id=*((u_long*)pParam);
	ClientsTHREAD *ct=&lserver->threads[id];
	ct->threadIsRunning=TRUE;
	ct->connections=0;
	ct->threadIsStopped=FALSE;
	ct->buffersize=lserver->buffersize;
	ct->buffersize2=lserver->buffersize2;
	ct->buffer=(char*)malloc(ct->buffersize);
	ct->buffer2=(char*)malloc(ct->buffersize2);
	ct->initialized=TRUE;
	ZeroMemory(ct->buffer,ct->buffersize);
	ZeroMemory(ct->buffer2,ct->buffersize2);
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
	}
	ct->threadIsStopped=TRUE;
	_endthreadex(0);
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
	int logonStatus;
	for(c; c ;c=c->Next)
	{
		nBytesToRead=bytesToRead(c->socket);
		if(nBytesToRead)
		{
			logon(c,&logonStatus,&hImpersonation);
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
	*thread.
	*/
	threadIsRunning=FALSE;
}

/*
*Clean the memory used by the thread.
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
LPCONNECTION ClientsTHREAD::addConnection(MYSERVER_SOCKET s,CONNECTION_PROTOCOL protID,char *ipAddr,int port)
{
	requestAccess(&connectionWriteAccess,this->id);
	LPCONNECTION nc=(CONNECTION*)malloc(sizeof(CONNECTION));
	ZeroMemory(nc,sizeof(CONNECTION));
	nc->socket=s;
	nc->port=(u_short)port;
	nc->timeout=clock();
	nc->protocol=protID;
	lstrcpy(nc->ipAddr,ipAddr);
	nc->Next=connections;
	connections=nc;
	nConnections++;
	terminateAccess(&connectionWriteAccess,this->id);
	return nc;
}

/*
*Delete a connection.
*/
int ClientsTHREAD::deleteConnection(LPCONNECTION s)
{
	requestAccess(&connectionWriteAccess,this->id);
	int ret=FALSE;
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
			ret=TRUE;
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
	for(;connections && c;c=c->Next)
	{
		deleteConnection(c);
	}
	connections=NULL;
	nConnections=0;
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
*Returns TRUE if the thread is active.
*/
int ClientsTHREAD::isRunning()
{
	return threadIsRunning;
}

/*
*Returns TRUE if the thread is stopped.
*/
int ClientsTHREAD::isStopped()
{
	return threadIsStopped;
}