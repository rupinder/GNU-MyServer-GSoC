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
#include "../include/cserver.h"

#include "../include/http.h"	/*Include the HTTP protocol*/
#include "../include/https.h" /*Include the HTTPS protocol*/

#include "../include/security.h"
#include "../include/stringutils.h"
#include "../include/sockets.h"
extern "C" {
#ifdef WIN32
#include <Ws2tcpip.h>
#include <direct.h>
#endif
#ifdef NOT_WIN
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#endif
}

#ifdef NOT_WIN
#define LPINT int *
#define INVALID_SOCKET -1
#define WORD unsigned int
#define BYTE unsigned char
#define MAKEWORD(a, b) ((WORD) (((BYTE) (a)) | ((WORD) ((BYTE) (b))) << 8)) 
#define max(a,b) ((a>b)?a:b)
#endif

/*!
*This is the unique istance for the class cserver in the application.
*/
cserver *lserver=0;
/*!
When the flag mustEndServer is 1 all the threads are stopped and the application stop
*its execution.
*/
int mustEndServer;

cserver::cserver()
{
	threads=0;
	listingThreads=0;
}

cserver::~cserver()
{

}

void cserver::start()
{
	u_long i;
	/*!
	*Set everything to 0.
	*/
	memset(this, 0, sizeof(cserver));

	/*!
	*Save the unique instance of this class.
	*/
	lserver=this;

#ifdef WIN32
	/*!
	*Under the windows platform use the cls operating-system command to clear the screen.
	*/
	_flushall();
	system("cls");
#endif
#ifdef NOT_WIN
	/*!
	*Under an UNIX environment, clearing the screen can be done in a similar method
	*/
	sync();
	system("clear");
#endif
	/*!
	*Print the MyServer signature.
	*/
	char *software_signature=(char*)malloc(200);
	if(software_signature)
	{
		sprintf(software_signature,"************MyServer %s************",versionOfSoftware);
	
		i=(u_long)strlen(software_signature);
		while(i--)
			printf("*");
			printf("\n%s\n",software_signature);
		i=(u_long)strlen(software_signature);
		while(i--)
			printf("*");
		printf("\n");
		free(software_signature);
	}
	/*!
	*Set the current working directory.
	*/
	setcwdBuffer();
	
	/*!
	*Setup the server configuration.
	*/
    	printf("Initializing server configuration...\n");

	int OSVer=getOSVersion();

	initialize(OSVer);	
	/*!
	*Initialize the SSL library
	*/
#ifndef DO_NOT_USE_SSL
	SSL_library_init();
	SSL_load_error_strings();
#endif
	printf("%s\n",languageParser.getValue("MSG_SERVER_CONF"));

	/*!
	*Startup the socket library.
	*/
	printf("%s\n",languageParser.getValue("MSG_ISOCK"));
	int err= startupSocketLib(/*!MAKEWORD( 2, 2 )*/MAKEWORD( 1, 1));
	if (err != 0) 
	{ 

	        preparePrintError();
		printf("%s\n",languageParser.getValue("ERR_ISOCK"));
		endPrintError();
		return; 
	} 
	printf("%s\n",languageParser.getValue("MSG_SOCKSTART"));

	/*!
	*Get the name of the local machine.
	*/
	MYSERVER_SOCKET::gethostname(serverName,(u_long)sizeof(serverName));
	printf("%s: %s\n",languageParser.getValue("MSG_GETNAME"),serverName);

	/*!
	*Determine all the IP addresses of the local machine.
	*/
	MYSERVER_HOSTENT *localhe=MYSERVER_SOCKET::gethostbyname(serverName);
	in_addr ia;
	ipAddresses[0]='\0';
	printf("Host: %s\r\n",serverName);
	if(localhe)
	{
		for(i=0;(localhe->h_addr_list[i])&&(i< MAX_ALLOWED_IPs);i++)
		{
#ifdef WIN32
			ia.S_un.S_addr = *((u_long FAR*) (localhe->h_addr_list[i]));
#endif
#ifdef NOT_WIN
			ia.s_addr = *((u_long *) (localhe->h_addr_list[i]));
#endif
			printf("%s #%u: %s\n",languageParser.getValue("MSG_ADDRESS"),i+1,inet_ntoa(ia));
			sprintf(&ipAddresses[strlen(ipAddresses)],"%s%s",strlen(ipAddresses)?",":"",inet_ntoa(ia));
		}
	}
	loadSettings();
	/*!
	*Keep thread alive.
	*When the mustEndServer flag is set to True exit
	*from the loop and terminate the server execution.
	*/
	while(!mustEndServer)
	{
		wait(1000000);
#ifdef WIN32
		DWORD cNumRead,i; 
		INPUT_RECORD irInBuf[128]; 
		ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE),irInBuf,128,&cNumRead);
		for (i = 0; i < cNumRead; i++) 
		{
			switch(irInBuf[i].EventType) 
			{ 
				case KEY_EVENT:
					if(irInBuf[i].Event.KeyEvent.wRepeatCount!=1)
						continue;
					if(irInBuf[i].Event.KeyEvent.wVirtualKeyCode=='c'||irInBuf[i].Event.KeyEvent.wVirtualKeyCode=='C')
					{
						if((irInBuf[i].Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED)|(irInBuf[i].Event.KeyEvent.dwControlKeyState & RIGHT_CTRL_PRESSED))
						{
							printf ("%s\n",languageParser.getValue("MSG_SERVICESTOP"));
							this->stop();
						}
					}
					break; 
			} 
		}
		
#endif
	}
	this->terminate();
}
/*!
*This function is used to create a socket server and a thread listener for a protocol.
*/
int cserver::createServerAndListener(u_long port)
{
	/*!
	*Create the server socket.
	*/
	printf("%s\n",languageParser.getValue("MSG_SSOCKCREATE"));
	MYSERVER_SOCKET serverSocket;
	serverSocket.socket(AF_INET,SOCK_STREAM,0);
	MYSERVER_SOCKADDRIN sock_inserverSocket;
	if(serverSocket.getHandle()==INVALID_SOCKET)
	{
		preparePrintError();
		printf("%s\n",languageParser.getValue("ERR_OPENP"));
		endPrintError();		
		return 0;

	}
	printf("%s\n",languageParser.getValue("MSG_SSOCKRUN"));
	sock_inserverSocket.sin_family=AF_INET;
	sock_inserverSocket.sin_addr.s_addr=htonl(INADDR_ANY);
	sock_inserverSocket.sin_port=htons((u_short)port);

#ifdef NOT_WIN
	/*!
	*Under the unix environment the application needs some time before create a new socket
	*for the same address. To avoid this behavior we use the current code.
	*/
	int optvalReuseAddr=1;
	if(serverSocket.setsockopt(SOL_SOCKET,SO_REUSEADDR,(const char *)&optvalReuseAddr,sizeof(optvalReuseAddr))<0)
	{
        	preparePrintError();
		printf("%s setsockopt\n",languageParser.getValue("ERR_ERROR"));
	        endPrintError();
		return 0;
	}
#endif
	/*!
	*Bind the port.
	*/
	printf("%s\n",languageParser.getValue("MSG_BIND_PORT"));

	if(serverSocket.bind((sockaddr*)&sock_inserverSocket,sizeof(sock_inserverSocket))!=0)
	{
		preparePrintError();
		printf("%s\n",languageParser.getValue("ERR_BIND"));
	        endPrintError();
		return 0;
	}
	printf("%s\n",languageParser.getValue("MSG_PORT_BINDED"));

	/*!
	*Set connections listen queque to max allowable.
	*/
	printf("%s\n",languageParser.getValue("MSG_SLISTEN"));
	if (serverSocket.listen(SOMAXCONN))
	{ 
        preparePrintError();
		printf("%s\n",languageParser.getValue("ERR_LISTEN"));
        endPrintError();	
		return 0; 
	}

	printf("%s: %u\n",languageParser.getValue("MSG_LISTEN"),port);

	printf("%s\n",languageParser.getValue("MSG_LISTENTR"));
	/*!
	*Create the listen thread.
	*/
	listenThreadArgv* argv=new listenThreadArgv;
	argv->port=port;
	argv->serverSocket=serverSocket;

	myserver_thread_ID ID;
	
	myserver_thread::create(&ID, &::listenServer, (void *)(argv));
	
	return (ID)?1:0;
}

/*!
*Create a listen thread for every port used by MyServer
*/
void cserver::createListenThreads()
{
	/*!
	*Create the listens threads.
	*Every port uses a thread.
	*/
	for(vhostmanager::sVhostList *list=vhostList.getvHostList();list;list=list->next )
	{
		int needThread=1;
		vhostmanager::sVhostList *list2=vhostList.getvHostList();
		for(;;)
		{
			list2=list2->next;
			if(list2==0)
				break;
			if(list2==list)
				break;
			if(list2->host->port==list->host->port)
				needThread=0;
		}
		if(list->host->port==0)
			continue;
		if(needThread)
		{
			if(createServerAndListener(list->host->port)==0)
			{
				preparePrintError();
				printf("%s: Listen threads\n",languageParser.getValue("ERR_ERROR"));
				endPrintError();
				return;
			}
		}
	}


}

/*!
*This is the thread that listens for a new connection on the port specified by the protocol.
*/
#ifdef WIN32
unsigned int __stdcall listenServer(void* params)
#endif
#ifdef HAVE_PTHREAD
void * listenServer(void* params)
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
	listenThreadArgv *argv=(listenThreadArgv*)params;
	MYSERVER_SOCKET serverSocket=argv->serverSocket;
	delete argv;

	MYSERVER_SOCKADDRIN asock_in;
	int asock_inLen=sizeof(asock_in);
	MYSERVER_SOCKET asock;
	lserver->increaseListeningThreadCount();
	while(!mustEndServer)
	{
		/*!
		*Accept connections.
		*Every new connection is sended to cserver::addConnection function;
		*this function sends connections between the various threads.
		*/
		if(serverSocket.dataOnRead()==0)
		{
			wait(100);
			continue;
		}
		asock=serverSocket.accept((struct sockaddr*)&asock_in,(LPINT)&asock_inLen);
		asock.setServerSocket(&serverSocket);
		if(asock.getHandle()==0)
			continue;
		if(asock.getHandle()==INVALID_SOCKET)
			continue;
		lserver->addConnection(asock,&asock_in);
	}
	
	/*!
	*When the flag mustEndServer is 1 end current thread and clean the socket used for listening.
	*/
	serverSocket.shutdown( SD_BOTH);
	char buffer[256];
	int err;
	do
	{
		err=serverSocket.recv(buffer,256,0);
	}while(err!=-1);
	serverSocket.closesocket();
	lserver->decreaseListeningThreadCount();
	
	/*!
	*Automatically free the current thread.
	*/	
	myserver_thread::terminate();
	return 0;

}

/*!
*Returns the numbers of active connections the list.
*/
u_long cserver::getNumConnections()
{
	return nConnections;
}

/*!
*Get the verbosity value.
*/
u_long cserver::getVerbosity()
{
	return verbosity;
}

/*!
*Set the verbosity value.
*/
void  cserver::setVerbosity(u_long nv)
{
	verbosity=nv;
}

/*!
*Stop the execution of the server.
*/
void cserver::stop()
{
	mustEndServer=1;
}

/*!
*Unload the server.
*/
void cserver::terminate()
{
	/*!
	*Stop the server execution.
	*/
	u_long i;
	for(i=0;i<nThreads;i++)
	{
		if(verbosity>1)
			printf("%s\n",languageParser.getValue("MSG_STOPT"));
		threads[i].stop();
		if(verbosity>1)
			printf("%s\n",languageParser.getValue("MSG_TSTOPPED"));
	}
	while(lserver->getListeningThreadCount())
	{
		wait(1000000);
	}

	if(verbosity>1)
	{
		printf("%s\n",languageParser.getValue("MSG_MEMCLEAN"));
	}
	stopThreads();
	
	/*!
	*If there are open connections close them.
	*/
	if(connections)
	{
		clearAllConnections();
	}
	vhostList.clean();
	languageParser.close();
	mimeManager.clean();
#ifdef WIN32
	/*!
	*Under WIN32 cleanup environment strings.
	*/
	FreeEnvironmentStrings((LPTSTR)envString);
#endif
	http::unloadProtocol(&languageParser);
	https::unloadProtocol(&languageParser);
	protocols.unloadProtocols(&languageParser);

	/*!
	*Destroy the connections mutex.
	*/
	delete c_mutex;
	if(threads)
		free(threads);
	threads=0;
	nThreads=0;
	if(verbosity>1)
	{
		printf("MyServer is stopped.\n\n");
	}
}

/*!
*Stop all the threads.
*/
void cserver::stopThreads()
{
	/*!
	*Clean here the memory allocated.
	*/
	u_long threadsStopped=0;

	/*!
	*Wait before clean the threads that all the threads are stopped.
	*/
	u_long i;
	for(i=0;i<nThreads;i++)
		threads[i].clean();
	u_long threadsStopTime=0;
	for(;;)
	{
		threadsStopped=0;
		for(i=0;i<nThreads;i++)
			if(threads[i].isStopped())
				threadsStopped++;
		/*!
		*If all the threads are stopped break the loop.
		*/
		if(threadsStopped==nThreads)
			break;
		/*!
		*Do not wait a lot to kill the thread.
		*/
		if(++threadsStopTime > 500 )
			break;
		wait(100);
	}

}
/*!
*Get the server administrator e-mail address.
*To change this use the main configuration file.
*/
char *cserver::getServerAdmin()
{
	return serverAdmin;
}
/*!
*Here is loaded the configuration of the server.
*The configuration file is a XML file.
*/
void cserver::initialize(int /*!OSVer*/)
{
#ifdef WIN32
	envString=GetEnvironmentStrings();
#endif
	/*!
	*Create the mutex for the connections.
	*/
	c_mutex= new myserver_mutex;
	
	/*!
	*Store the default values.
	*/
	u_long nThreadsA=1;
	u_long nThreadsB=0;
	u_long i=0;
	socketRcvTimeout = 10;
	useLogonOption = 1;
	connectionTimeout = SEC(25);
	lstrcpy(languageFile,"languages/english.xml");
	mustEndServer=0;
	verbosity=1;
	maxConnections=0;
	serverAdmin[0]='\0';
	
	/*!
	*If the myserver.xml files doesn't exist copy it from the default one.
	*/
	if(!MYSERVER_FILE::fileExists("myserver.xml"))
	{
			MYSERVER_FILE inputF;
			MYSERVER_FILE outputF;
			int ret=inputF.openFile("myserver.xml.default",MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_IFEXISTS);
			if(ret<1)
			{
				preparePrintError();
				printf("%s\n",languageParser.getValue("ERR_LOADMIME"));
				endPrintError();				
				return;
			}
			outputF.openFile("myserver.xml",MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_OPEN_ALWAYS);
			char buffer[512];
			u_long nbr,nbw;
			for(;;)
			{
				inputF.readFromFile(buffer,512,&nbr );
				if(nbr==0)
					break;
				outputF.writeToFile(buffer,nbr,&nbw);
			}
			inputF.closeFile();
			outputF.closeFile();
	}		
	
	configurationFileManager.open("myserver.xml");
	char *data;

	data=configurationFileManager.getValue("VERBOSITY");
	if(data)
	{
		verbosity=(u_long)atoi(data);
	}
	data=configurationFileManager.getValue("LANGUAGE");
	if(data)
	{
		sprintf(languageFile,"languages/%s",data);	
	}

	data=configurationFileManager.getValue("BUFFER_SIZE");
	if(data)
	{
		buffersize=buffersize2=max((u_long)atol(data),81920);
	}
	data=configurationFileManager.getValue("CONNECTION_TIMEOUT");
	if(data)
	{
		connectionTimeout=SEC((u_long)atol(data));
	}

	data=configurationFileManager.getValue("NTHREADS_A");
	if(data)
	{
		nThreadsA=atoi(data);
	}
	data=configurationFileManager.getValue("NTHREADS_B");
	if(data)
	{
		nThreadsB=atoi(data);
	}
	
	/*!
	*The number of the threads used by the server is:
	*N_THREADS=nThreadsForCPU*CPU_COUNT+nThreadsAlwaysActive;
	*/
	nThreads=nThreadsA*getCPUCount()+nThreadsB;

	/*!
	*Get the max connections number to allow.
	*/
	data=configurationFileManager.getValue("MAX_CONNECTIONS");
	if(data)
	{
		maxConnections=atoi(data);
	}			
	/*!
	*Load the server administrator e-mail.
	*/	
	data=configurationFileManager.getValue("SERVER_ADMIN");
	if(data)
	{
		lstrcpy(serverAdmin,data);
	}

	data=configurationFileManager.getValue("USE_LOGON_OPTIONS");
	if(data)
	{
		if(!lstrcmpi(data,"YES"))
			useLogonOption=1;
		else
			useLogonOption=0;
	}

	data=configurationFileManager.getValue("MAX_LOG_FILE_SIZE");
	if(data)
	{
		maxLogFileSize=(u_long)atol(data);
	}
	
	configurationFileManager.close();
	
	languageParser.open(languageFile);
	printf("%s\n",languageParser.getValue("MSG_LANGUAGE"));

	
}
/*!
*This function returns the max size of the logs file as defined in the 
*configuration file.
*/
int cserver::getMaxLogFileSize()
{
	return maxLogFileSize;
}
/*!
*Returns the connection timeout.
*/
u_long cserver::getTimeout()
{
	return connectionTimeout;
}
/*!
*This function add a new connection to the list.
*/
int cserver::addConnection(MYSERVER_SOCKET s,MYSERVER_SOCKADDRIN *asock_in)
{
	if(s.getHandle()==0)
		return 0;
	static int ret;
	ret=1;
	/*!
	*ip is the string containing the address of the remote host connecting to the server
	*myip is the address of the local host on the same network.
	*/
	char ip[MAX_IP_STRING_LEN];
	char myIp[MAX_IP_STRING_LEN];
	MYSERVER_SOCKADDRIN  localsock_in;

	memset(&localsock_in, 0, sizeof(localsock_in));

	int dim=sizeof(localsock_in);
	s.getsockname((MYSERVER_SOCKADDR*)&localsock_in,&dim);

	strncpy(ip, inet_ntoa(asock_in->sin_addr), MAX_IP_STRING_LEN); // NOTE: inet_ntop supports IPv6
	strncpy(myIp, inet_ntoa(localsock_in.sin_addr), MAX_IP_STRING_LEN); // NOTE: inet_ntop supports IPv6

	int port=ntohs((*asock_in).sin_port);/*!Port used by the client*/
	int myport=ntohs(localsock_in.sin_port);/*!Port connected to*/

	if(!addConnectionToList(s,asock_in,&ip[0],&myIp[0],port,myport,1))
	{
		/*!If we report error to add the connection to the thread*/
		ret=0;
		s.shutdown(2);/*!Shutdown the socket both on receive that on send*/
		s.closesocket();/*!Then close it*/
	}
	return ret;
}

/*!
*Add a new connection.
*A connection is defined using a CONNECTION struct.
*/
LPCONNECTION cserver::addConnectionToList(MYSERVER_SOCKET s,MYSERVER_SOCKADDRIN* /*asock_in*/,char *ipAddr,char *localIpAddr,int port,int localPort,int id)
{
	u_long cs=sizeof(CONNECTION);
	LPCONNECTION nc=(CONNECTION*)malloc(cs);
	if(!nc)
	{
		return NULL;
	}
	nc->check_value = CONNECTION::check_value_const;
	nc->connectionBuffer[0]='\0';
	nc->socket=s;
	nc->toRemove=0;
	nc->forceParsing=0;
	nc->parsing=0;
	nc->port=(u_short)port;
	nc->timeout=get_ticks();
	nc->dataRead = 0;
	nc->localPort=(u_short)localPort;
	strncpy(nc->ipAddr,ipAddr,MAX_IP_STRING_LEN);
	strncpy(nc->localIpAddr,localIpAddr,MAX_IP_STRING_LEN);
	nc->host = (void*)lserver->vhostList.getvHost(0,localIpAddr,(u_short)localPort);
	if(nc->host == 0) /* No vhost for the connection so bail */
	{
		free(nc);
		return 0;
	}
	int doSSLhandshake=0;
	if(((vhost*)nc->host)->protocol > 1000	)
	{
		doSSLhandshake=1;
	}
	else if(((vhost*)nc->host)->protocol==PROTOCOL_UNKNOWN)
	{	
		dynamic_protocol* dp=lserver->getDynProtocol(((vhost*)(nc->host))->protocol_name);	
		if(dp->getOptions() & PROTOCOL_USES_SSL)
			doSSLhandshake=1;	
	}
	
	/*!
	*Do the SSL handshake if required.
	*/
	if(doSSLhandshake)
	{
		int ret =0;
#ifndef DO_NOT_USE_SSL		
		SSL_CTX* ctx=((vhost*)nc->host)->getSSLContext();
		nc->socket.setSSLContext(ctx);
		ret=nc->socket.sslAccept();
		
#endif		
		if(ret<0)
		{
			/*
			*Free the connection on errors.
			*/
			free(nc);
			return 0;
		}
	}
	
	nc->login[0]='\0';
	nc->nTries=0;
	nc->password[0]='\0';
	nc->protocolBuffer=0;

	if(nc->host==0)
	{
		free(nc);
		return 0;
	}
	/*!
	*Update the list.
	*/
	lserver->connections_mutex_lock();
	nc->next = connections;
    	connections=nc;
	nConnections++;
	lserver->connections_mutex_unlock();
	/*
	If defined maxConnections and the number of active connections is bigger than it
	*say to the protocol that will parse the connection to remove it from the active
	*connections list.
	*/
	if(maxConnections && (nConnections>maxConnections))
		nc->toRemove=CONNECTION_REMOVE_OVERLOAD;

	return nc;
}

/*!
*Delete a connection from the list.
*/
int cserver::deleteConnection(LPCONNECTION s,int id)
{
	if(!s)
	{
		return 0;
	}
	if(s->check_value!=CONNECTION::check_value_const)
	{
		return 0;
	}
	MYSERVER_SOCKET socket=s->socket;
	/*!
	*Get the access to the  connections list.
	*/
	lserver->connections_mutex_lock();
	int ret=0,err;
	/*!
	*Remove the connection from the active connections list.
	*/
	LPCONNECTION prev=0;
	for(LPCONNECTION i=connections;i;i=i->next )
	{
		if(i->socket == s->socket)
		{
			if(connectionToParse)
				if(connectionToParse->socket==s->socket)
					connectionToParse=connectionToParse->next;

			if(prev)
				prev->next =i->next;
			else
				connections=i->next;
			ret=1;
			break;
		}
		else
		{
			prev=i;
		}
	}

	nConnections--;

	if(s->protocolBuffer)
		free(s->protocolBuffer);
	free(s);

	lserver->connections_mutex_unlock();
	/*!
	*Close the socket communication.
	*/
	socket.shutdown(SD_BOTH);
	char buffer[256];
	int buffersize=256;
	do
	{
		err=socket.recv(buffer,buffersize,0);
	}while(err!=-1);
	socket.closesocket();

	return ret;
}
/*!
*Get a connection to parse. Be sure to have the connections access for the caller thread before use this.
*Using this without the right permissions can cause wrong data returned to the client.
*/
LPCONNECTION cserver::getConnectionToParse(int id)
{
	/*
	*Do nothing if there are not connections.
	*/
	if(connections==0)
		return 0;
	/*!
	*Stop the thread if the server is pausing.
	*/
	while(pausing)
	{
		wait(5);
	}
	if(connectionToParse)
	{
		/*!Be sure that connectionToParse is a valid connection struct*/
		if(connectionToParse->check_value!=CONNECTION::check_value_const)
			connectionToParse=connections;
		else
			connectionToParse=connectionToParse->next;
	}
	else
	{
		connectionToParse=connections;
	}
	if(connectionToParse==0)
		connectionToParse=connections;


	return connectionToParse;
}
/*!
*Delete all the active connections.
*/
void cserver::clearAllConnections()
{
	LPCONNECTION c=connections;
	LPCONNECTION next=0;
	while(c)
	{
		next=c->next;
		deleteConnection(c,1);
		c=next;
	}
	/*!
	*Reset everything
	*/
	nConnections=0;
	connections=0;
	connectionToParse=0;
}


/*!
*Find a connection passing its socket.
*/
LPCONNECTION cserver::findConnection(MYSERVER_SOCKET a)
{
	connections_mutex_lock();
	LPCONNECTION c;
	for(c=connections;c;c=c->next )
	{
		if(c->socket==a)
		{
			connections_mutex_unlock();
			return c;
		}
	}
	connections_mutex_unlock();
	return NULL;
}

/*!
*Returns the full path of the binaries folder. The folder where the file myserver(.exe) is.
*/
char *cserver::getPath()
{
	return path;
}
/*!
*Returns the name of the server(the name of the current PC).
*/
char *cserver::getServerName()
{
	return serverName;
}
/*!
*Returns if we use the logon.
*/
int cserver::mustUseLogonOption() 
{
	return useLogonOption;
}
/*!
*Gets the number of threads.
*/
u_long cserver::getNumThreads()
{
	return nThreads;
}
/*!
*Returns a comma-separated local machine IPs list.
*For example: 192.168.0.1,61.62.63.64,65.66.67.68.69
*/
char *cserver::getAddresses()
{
	return ipAddresses;
}
/*!
*Get the dynamic protocol to use for that connection.
*/
dynamic_protocol* cserver::getDynProtocol(char *protocolName)
{
	return protocols.getDynProtocol(protocolName);
}

/*!
*Lock connections list access to the caller thread.
*/
int cserver::connections_mutex_lock()
{
	c_mutex->myserver_mutex_lock();
	return 1;
}

/*!
*Unlock connections list access.
*/
int cserver::connections_mutex_unlock()
{
	c_mutex->myserver_mutex_unlock();
	return 1;
}
void cserver::loadSettings()
{

	/*!
	*The guestLoginHandle value is filled by the call to cserver::initialize.
	*/
	if(useLogonOption==0)
	{
        	preparePrintError();		
		printf("%s\n",languageParser.getValue("AL_NO_SECURITY"));
        	endPrintError();
	}

	u_long i;
	/*!
	*If the MIMEtypes.xml files doesn't exist copy it from the default one.
	*/
	if(!MYSERVER_FILE::fileExists("MIMEtypes.xml"))
	{
		MYSERVER_FILE inputF;
		MYSERVER_FILE outputF;
		int ret=inputF.openFile("MIMEtypes.xml.default",MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_IFEXISTS);
		if(ret<1)
		{
			preparePrintError();
			printf("%s\n",languageParser.getValue("ERR_LOADMIME"));
			endPrintError();				
			return;
		}
		outputF.openFile("MIMEtypes.xml",MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_OPEN_ALWAYS);
		char buffer[512];
		u_long nbr,nbw;
		for(;;)
		{
			inputF.readFromFile(buffer,512,&nbr );
			if(nbr==0)
				break;
			outputF.writeToFile(buffer,nbr,&nbw);
		}
		
		inputF.closeFile();
		outputF.closeFile();
	}
	/*!
	*Load the MIME types.
	*/		
	printf("%s\n",languageParser.getValue("MSG_LOADMIME"));
	if(int nMIMEtypes=mimeManager.loadXML("MIMEtypes.xml"))
	{
		printf("%s: %i\n",languageParser.getValue("MSG_MIMERUN"),nMIMEtypes);
	}
	else
	{
        	preparePrintError();
		printf("%s\n",languageParser.getValue("ERR_LOADMIME"));
        	endPrintError();
		return;
	}
	printf("%s %u\n",languageParser.getValue("MSG_NUM_CPU"),getCPUCount());

	
	
	/*!
	*If the virtualhosts.xml file doesn't exist copy it from the default one.
	*/
	if(!MYSERVER_FILE::fileExists("virtualhosts.xml"))
	{
		MYSERVER_FILE inputF;
		MYSERVER_FILE outputF;
		int ret = inputF.openFile("virtualhosts.xml.default", MYSERVER_FILE_OPEN_READ | MYSERVER_FILE_OPEN_IFEXISTS );
		if(ret<1)
		{
			preparePrintError();
			printf("%s\n",languageParser.getValue("ERR_LOADMIME"));
			endPrintError();				
			return;
		}
		outputF.openFile("virtualhosts.xml",MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_OPEN_ALWAYS);
		char buffer[512];
		u_long nbr,nbw;
		for(;;)
		{
			inputF.readFromFile(buffer,512,&nbr );
			if(nbr==0)
				break;
			outputF.writeToFile(buffer,nbr,&nbw);
		}
		
		inputF.closeFile();
		outputF.closeFile();
	}	
	/*!
	*Load the virtual hosts configuration from the xml file
	*/
	vhostList.loadXMLConfigurationFile("virtualhosts.xml",this->getMaxLogFileSize());

	http::loadProtocol(&languageParser,"myserver.xml");
	https::loadProtocol(&languageParser,"myserver.xml");
	protocols.loadProtocols("external/protocols",&languageParser,"myserver.xml",this);

	myserver_thread_ID ID;
	if(threads)
		free(threads);
	threads=(ClientsTHREAD*)malloc(sizeof(ClientsTHREAD)*nThreads);
	memset(threads,0,sizeof(ClientsTHREAD)*nThreads);
	if(threads==NULL)
	{
		preparePrintError();
		printf("%s: Threads creation\n",languageParser.getValue("ERR_ERROR"));
        	endPrintError();	
	}
	for(i=0;i<nThreads;i++)
	{
		printf("%s %u...\n",languageParser.getValue("MSG_CREATET"),i);
		threads[i].id=(i+ClientsTHREAD::ID_OFFSET);
		myserver_thread::create(&ID,  &::startClientsTHREAD, (void *)&(threads[i].id));
		printf("%s\n",languageParser.getValue("MSG_THREADR"));
	}
	getdefaultwd(path,MAX_PATH);
	/*!
	*Then we create here all the listens threads. Check that all the port used for listening
	*have a listen thread.
	*/
	printf("%s\n",languageParser.getValue("MSG_LISTENT"));
	
	createListenThreads();

	printf("%s\n",languageParser.getValue("MSG_READY"));
	printf("%s\n",languageParser.getValue("MSG_BREAK"));
}
/*!
*Reboot the server.
*/
void cserver::reboot()
{
	printf("%c\n\nRebooting.......\n\n",0x7);/*0x7 is the beep*/
	if(mustEndServer)
		return;
	mustEndServer=1;
	terminate();
	mustEndServer=0;
	wait(1000000);
	initialize(0);
	loadSettings();

}

/*!
*Get the number of active listening threads.
*/
int cserver::getListeningThreadCount()
{
	return listingThreads;
}
/*!
*Increase of one the number of active listening threads.
*/
void cserver::increaseListeningThreadCount()
{
	connections_mutex_lock();
	listingThreads++;
	connections_mutex_unlock();
}

/*!
*Decrease of one the number of active listening threads.
*/
void cserver::decreaseListeningThreadCount()
{
	connections_mutex_lock();
	listingThreads--;
	connections_mutex_unlock();
}
