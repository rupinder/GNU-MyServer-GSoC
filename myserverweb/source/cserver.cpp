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
#include "../include/security.h"
#include "../include/stringutils.h"
#include "../include/fastCGI.h"
#include "../include/sockets.h"
#include "../include/isapi.h"
extern "C" {
#ifdef WIN32
#include <Ws2tcpip.h>
#include <direct.h>
#endif
#ifdef __linux__
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#endif
}

#ifndef WIN32
#define LPINT int *
#define INVALID_SOCKET -1
#define WORD unsigned int
#define BYTE unsigned char
#define MAKEWORD(a, b) ((WORD) (((BYTE) (a)) | ((WORD) ((BYTE) (b))) << 8)) 
#endif

/*
*These variables are the unique istance of the class cserver in the application and the flag
*mustEndServer. When mustEndServer is 1 all the threads are stopped and the application stop
*its execution.
*/
cserver *lserver=0;
int mustEndServer;



void cserver::start()
{
	u_long i;
	nConnections=0;
	connections=0;
	connectionToParse=0;
	/*
	*Set the current working directory.
	*/
	setcwdBuffer();
	mustEndServer=0;
	memset(this, 0, sizeof(cserver));


	/*
	*If another instance of the class exists destroy it.
	*/
	if(lserver)
	{
		mustEndServer=1;
		lserver->terminate();
		wait(2000);/*Wait for a while*/
		mustEndServer=0;
	}
	/*
	*Save the unique instance of this class.
	*/
	lserver=this;

#ifdef WIN32
	/*
	*Under the windows platform use the cls operating-system command to clear the screen.
	*/
	_flushall();
	system("cls");
#endif
#ifdef __linux__
	/*
	*Under an UNIX environment, clearing the screen can be done in a similar method
	*/
	sync();
	system("clear");
#endif
	
	/*
	*Print the myServer logo.
	*/
	char *software_signature=(char*)malloc(200);
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

	/*
	*Setup the server configuration.
	*/
    printf("Initializing server configuration...\n");
#ifdef WIN32
	envString=GetEnvironmentStrings();
#endif
	int OSVer=getOSVersion();

	initialize(OSVer);
	
	languageParser.open(languageFile);
	printf("%s\n",languageParser.getValue("MSG_LANGUAGE"));

#ifdef WIN32
	/*
	*Under WIN32 initialize ISAPI.
	*/
	initISAPI();
#endif	
	initializeFASTCGI();

	printf("%s\n",languageParser.getValue("MSG_SERVER_CONF"));

	/*
	*The guestLoginHandle value is filled by the call to cserver::initialize.
	*/
	if(useLogonOption==0)
	{
        preparePrintError();		
		printf("%s\n",languageParser.getValue("AL_NO_SECURITY"));
        endPrintError();
	}

	/*
	*Startup the socket library.
	*/
	printf("%s\n",languageParser.getValue("MSG_ISOCK"));
	int err= startupSocketLib(/*MAKEWORD( 2, 2 )*/MAKEWORD( 1, 1));
	if (err != 0) 
	{ 

        preparePrintError();
		printf("%s\n",languageParser.getValue("ERR_ISOCK"));
		endPrintError();
		return; 
	} 
	printf("%s\n",languageParser.getValue("MSG_SOCKSTART"));

	
	/*
	*Get the name of the local machine.
	*/
	MYSERVER_SOCKET::gethostname(serverName,(u_long)sizeof(serverName));
	printf("%s: %s\n",languageParser.getValue("MSG_GETNAME"),serverName);

	/*
	*Determine all the IP addresses of the local machine.
	*/
	MYSERVER_HOSTENT *localhe=MYSERVER_SOCKET::gethostbyname(serverName);
	in_addr ia;
	ipAddresses[0]='\0';
	for(i=0;localhe->h_addr_list[i];i++)
	{
#ifdef WIN32
		ia.S_un.S_addr = *((u_long FAR*) (localhe->h_addr_list[i]));
#endif
#ifdef __linux__
		ia.s_addr = *((u_long *) (localhe->h_addr_list[i]));
#endif
		printf("%s #%u: %s\n",languageParser.getValue("MSG_ADDRESS"),i,inet_ntoa(ia));
		sprintf(&ipAddresses[strlen(ipAddresses)],"%s%s",strlen(ipAddresses)?",":"",inet_ntoa(ia));
	}
	

	/*
	*Load the MSCGI library.
	*/
	mscgiLoaded=loadMSCGILib();
	/*
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

#ifdef WIN32
	unsigned int ID;
#endif
#ifdef __linux__
	pthread_t ID;
#endif
	threads=new ClientsTHREAD[nThreads];
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
#ifdef WIN32
		_beginthreadex(NULL,0,&::startClientsTHREAD,&threads[i].id,0,&ID);
#endif
#ifdef __linux__
		pthread_create(&ID, NULL, &::startClientsTHREAD, (void *)&(threads[i].id));
#endif
		printf("%s\n",languageParser.getValue("MSG_THREADR"));
	}

	getdefaultwd(path,MAX_PATH);
	/*
	*Then we create here all the listens threads. Check that all the port used for listening
	*have a listen thread.
	*/
	printf("%s\n",languageParser.getValue("MSG_LISTENT"));
	/*
	*Load the virtual hosts configuration from the xml file
	*/
	vhostList.loadXMLConfigurationFile("virtualhosts.xml",this->getMaxLogFileSize());


	/*
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

	printf("%s\n",languageParser.getValue("MSG_READY"));
	printf("%s\n",languageParser.getValue("MSG_BREAK"));

	/*
	*Keep thread alive.
	*When the mustEndServer flag is set to True exit
	*from the loop and terminate the server execution.
	*/
	while(!mustEndServer)
	{
#ifdef WIN32
		Sleep(1);
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
#ifdef __linux__
		sleep(1);
#endif
	}
	this->terminate();
}
/*
*This function is used to create a socket server and a thread listener for a protocol.
*/
int cserver::createServerAndListener(u_long port)
{
	/*
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

#ifdef __linux__
	/*
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

	/*
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

	/*
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


	/*
	*Create the listen thread.
	*/
	listenThreadArgv* argv=new listenThreadArgv;
	argv->port=port;
	argv->serverSocket=serverSocket;
#ifdef WIN32
	unsigned int ID;
	_beginthreadex(NULL,0,&::listenServer,argv,0,&ID);
#endif
#ifdef __linux__
	pthread_t ID;
	pthread_create(&ID, NULL, &::listenServer, (void *)(argv));
#endif
	return (ID)?1:0;
}
/*
*This is the thread that listens for a new connection on the port specified by the protocol.
*/
#ifdef WIN32
unsigned int __stdcall listenServer(void* params)
#endif
#ifdef __linux__
void * listenServer(void* params)
#endif
{
	listenThreadArgv *argv=(listenThreadArgv*)params;
	MYSERVER_SOCKET serverSocket=argv->serverSocket;
	delete argv;

	MYSERVER_SOCKADDRIN asock_in;
	int asock_inLen=sizeof(asock_in);
	MYSERVER_SOCKET asock;
	while(!mustEndServer)
	{
		/*
		*Accept connections.
		*Every new connection is sended to cserver::addConnection function;
		*this function sends connections between the various threads.
		*/
		asock=serverSocket.accept((struct sockaddr*)&asock_in,(LPINT)&asock_inLen);
		if(asock.getHandle()==0)
			continue;
		if(asock.getHandle()==INVALID_SOCKET)
			continue;
		lserver->addConnection(asock,&asock_in);
	}
	/*
	*When the flag mustEndServer is 1 end current thread and clean the socket used for listening.
	*/

	serverSocket.shutdown( 2);
	serverSocket.closesocket();
#ifdef WIN32
	_endthread();
#endif
#ifdef __linux__
	pthread_exit(0);
#endif

	return 0;

}
/*
*Returns the numbers of active connections the list.
*/
u_long cserver::getNumConnections()
{
	return nConnections;
}


/*
*Get the verbosity value.
*/
u_long cserver::getVerbosity()
{
	return verbosity;
}

/*
*Set the verbosity value.
*/
void  cserver::setVerbosity(u_long nv)
{
	verbosity=nv;
}
/*
*Stop the execution of the server.
*/
void cserver::stop()
{
	mustEndServer=1;
}

void cserver::terminate()
{
	/*
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
	if(connections)
	{
		clearAllConnections();
	}
	if(verbosity>1)
	{
		printf("%s\n",languageParser.getValue("MSG_MEMCLEAN"));
	}
	/*
	*Clean here the memory allocated.
	*/
	vhostList.clean();
	languageParser.close();
	mimeManager.clean();
	u_long threadsStopped=0;
#ifdef WIN32
	FreeEnvironmentStrings((LPTSTR)envString);
	cleanupISAPI();
#endif	
	cleanFASTCGI();

	freeMSCGILib();
	/*
	*Wait before clean the threads that all the threads are stopped.
	*/

	for(i=0;i<nThreads;i++)
		threads[i].clean();

	for(;;)
	{
		threadsStopped=0;
		for(i=0;i<nThreads;i++)
			if(threads[i].isStopped())
				threadsStopped++;
		/*
		*If all the threads are stopped break the loop.
		*/
		if(threadsStopped==nThreads)
			break;
	}
	delete[] defaultFilename;

	delete[] threads;
	if(verbosity>1)
	{
		printf("MyServer is stopped\n\n");
	}
}
/*
*Get the server administrator e-mail address.
*To change this use the main configuration file.
*/
char *cserver::getServerAdmin()
{
	return serverAdmin;
}
/*
*Here is loaded the configuration of the server.
*The configuration file is a XML file.
*/
void cserver::initialize(int OSVer)
{
	/*
	*Store the default values.
	*/
	u_long nThreadsA=1;
	u_long nThreadsB=0;
	socketRcvTimeout = 10;
	useLogonOption = 1;
	connectionTimeout = SEC(25);
	lstrcpy(languageFile,"languages/english.xml");
	browseDirCSSpath[0]='\0';
	mustEndServer=0;
	verbosity=1;
	serverAdmin[0]='\0';

	useMessagesFiles=1;
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

	data=configurationFileManager.getValue("BROWSEFOLDER_CSS");
	if(data)
	{
		lstrcpy(browseDirCSSpath,data);
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
	/*
	*The number of the threads used by the server is:
	*N_THREADS=nThreadsForCPU*CPU_COUNT+nThreadsAlwaysActive;
	*/
	nThreads=nThreadsA*getCPUCount()+nThreadsB;

	/*
	*Determine the number of default filenames written in the configuration file.
	*/
	nDefaultFilename=0;
	for(;;)
	{
		char xmlMember[21];
		sprintf(xmlMember,"DEFAULT_FILENAME%i",nDefaultFilename);
		if(!strlen(configurationFileManager.getValue(xmlMember)))
			break;
		nDefaultFilename++;

	}
	/*
	*Copy the right values in the buffer.
	*/
	if(nDefaultFilename==0)
	{
		defaultFilename =(char*)new char[MAX_PATH];
		strcpy(defaultFilename,"default.html");
	}
	else
	{
		u_long i;
		defaultFilename =new char[MAX_PATH*nDefaultFilename];
		for(i=0;i<nDefaultFilename;i++)
		{
			char xmlMember[21];
			sprintf(xmlMember,"DEFAULT_FILENAME%i",i);
			data=configurationFileManager.getValue(xmlMember);
			if(data)
				strcpy(&defaultFilename[i*MAX_PATH],data);
		}
	}

	data=configurationFileManager.getValue("SERVER_ADMIN");
	if(data)
	{
		lstrcpy(serverAdmin,data);
	}


	data=configurationFileManager.getValue("USE_ERRORS_FILES");
	if(data)
	{
		if(!lstrcmpi(data,"YES"))
			useMessagesFiles=1;
		else
			useMessagesFiles=0;
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

}
/*
*Get the max size of the logs file
*/
int cserver::getMaxLogFileSize()
{
	return maxLogFileSize;
}
/*
*Returns the connection timeout
*/
u_long cserver::getTimeout()
{
	return connectionTimeout;
}
/*
*This function add a new connection to the list.
*/
int cserver::addConnection(MYSERVER_SOCKET s,MYSERVER_SOCKADDRIN *asock_in)
{
	if(s.getHandle()==0)
		return 0;
	static int ret;
	ret=1;
	
	char ip[MAX_IP_STRING_LEN];
	char myIp[MAX_IP_STRING_LEN];
	MYSERVER_SOCKADDRIN  localsock_in;
#ifdef WIN32
	ZeroMemory(&localsock_in,sizeof(localsock_in));
#endif
#ifdef __linux__
	memset(&localsock_in, 0, sizeof(localsock_in));
#endif
	int dim=sizeof(localsock_in);
	s.getsockname((MYSERVER_SOCKADDR*)&localsock_in,&dim);

	strncpy(ip, inet_ntoa(asock_in->sin_addr), MAX_IP_STRING_LEN); // NOTE: inet_ntop supports IPv6
	strncpy(myIp, inet_ntoa(localsock_in.sin_addr), MAX_IP_STRING_LEN); // NOTE: inet_ntop supports IPv6

	int port=ntohs((*asock_in).sin_port);/*Port used by the client*/
	int myport=ntohs(localsock_in.sin_port);/*Port connected to*/

	if(!addConnectionToList(s,asock_in,&ip[0],&myIp[0],port,myport,1))
	{
		/*If we report error to add the connection to the thread*/
		ret=0;
		s.shutdown(2);/*Shutdown the socket both on receive that on send*/
		s.closesocket();/*Then close it*/
	}
	return ret;
}

/*
*Add a new connection.
*A connection is defined using a CONNECTION struct.
*/
LPCONNECTION cserver::addConnectionToList(MYSERVER_SOCKET s,MYSERVER_SOCKADDRIN *asock_in,char *ipAddr,char *localIpAddr,int port,int localPort,int id)
{
	requestAccess(&connectionWriteAccess,id);
	u_long cs=sizeof(CONNECTION);
	LPCONNECTION nc=(CONNECTION*)malloc(cs);
	if(!nc)
		return NULL;
	nc->check_value = CONNECTION::check_value_const;
	nc->connectionBuffer[0]='\0';
	nc->socket=s;
	nc->port=(u_short)port;
	nc->timeout=clock();
	nc->dataRead = 0;
	nc->localPort=(u_short)localPort;
	strcpy(nc->ipAddr,ipAddr);
	strcpy(nc->localIpAddr,localIpAddr);
	nc->next =connections;
    nc->host=(void*)lserver->vhostList.getvHost(0,localIpAddr,(u_short)localPort);
	nc->login[0]='\0';
	nc->nTries=0;
	nc->password[0]='\0';
	if(nc->host==0)
	{
		free(nc);
		return 0;
	}
    connections=nc;
	nConnections++;

	char msg[500];
#ifdef WIN32
	sprintf(msg, "%s:%s ->%s %s:", "Connection from", inet_ntoa(asock_in->sin_addr), lserver->getServerName(), "at time");
	getRFC822GMTTime(&msg[strlen(msg)],HTTP_RESPONSE_DATE_DIM);
	strcat(msg,"\r\n");
#endif
#ifdef __linux__
	snprintf(msg, 500,"%s:%s ->%s %s:", "Connection from", inet_ntoa(asock_in->sin_addr), lserver->getServerName(), "at time");
	getRFC822GMTTime(&msg[strlen(msg)],HTTP_RESPONSE_DATE_DIM);
	strcat(msg,"\r\n");
#endif
	((vhost*)(nc->host))->accessesLogWrite(msg);

	if(nc==0)
	{
		if(lserver->getVerbosity()>0)
		{
#ifdef WIN32
			sprintf(msg, "%s:%s ->%s %s:", "Error connection from", inet_ntoa(asock_in->sin_addr), lserver->getServerName(), "at time");
			getRFC822GMTTime(&msg[strlen(msg)],HTTP_RESPONSE_DATE_DIM);
			strcat(msg,"\r\n");
#endif
#ifdef __linux__
			snprintf(msg, 500,"%s:%s ->%s %s:", "Error connection from", inet_ntoa(asock_in->sin_addr), lserver->getServerName(), "at time");
			getRFC822GMTTime(&msg[strlen(msg)],HTTP_RESPONSE_DATE_DIM);
			strcat(msg,"\r\n");
#endif
			((vhost*)(nc->host))->warningsLogWrite(msg);
		}
	}
	terminateAccess(&connectionWriteAccess,id);
	return nc;
}


/*
*Delete a connection.
*/
int cserver::deleteConnection(LPCONNECTION s,int id)
{
	if(!s)
		return 0;
	requestAccess(&connectionWriteAccess,id);
	int ret=0,err;
	/*
	*First of all close the socket communication.
	*/
	s->socket.shutdown(SD_BOTH );
	char buffer[256];
	int buffersize=256;
	do
	{
		err=s->socket.recv(buffer,buffersize,0);
	}while(err!=-1);
	while(s->socket.closesocket());
	/*
	*Then remove the connection from the active connections list.
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
			free(i);
			ret=1;
			break;
		}
		else
		{
			prev=i;
		}
	}
	terminateAccess(&connectionWriteAccess,id);
	nConnections--;
	return ret;
}
/*
*Get a connection to parse.
*/
LPCONNECTION cserver::getConnectionToParse(int id)
{
	requestAccess(&connectionWriteAccess,id);
	if(connectionToParse)
	{
		/*Be sure that connectionToParse is a valid connection struct*/
		if(connectionToParse->check_value!=CONNECTION::check_value_const)
			connectionToParse=connections;
		else
			connectionToParse=connectionToParse->next;
	}
	else
	{/*Restart loop if the connectionToParse points to the last element*/
		connectionToParse=connections;
	}
	if(connectionToParse==0)
		connectionToParse=connections;
	terminateAccess(&connectionWriteAccess,id);
	return connectionToParse;
}
/*
*Delete all the active connections.
*/
void cserver::clearAllConnections()
{
	/*
	*Keep access to the connections list
	*/
	requestAccess(&connectionWriteAccess,1);
	LPCONNECTION c=connections;
	LPCONNECTION next=0;
	while(c)
	{
		next=c->next;
		deleteConnection(c,1);
		c=next;
	}
	nConnections=0;
	connections=0;
	connectionToParse=0;
	terminateAccess(&connectionWriteAccess,1);
}


/*
*Find a connection passing its socket.
*/
LPCONNECTION cserver::findConnection(MYSERVER_SOCKET a)
{
	requestAccess(&connectionWriteAccess,1);
	LPCONNECTION c;
	for(c=connections;c;c=c->next )
	{
		if(c->socket==a)
			return c;
	}
	terminateAccess(&connectionWriteAccess,1);
	return NULL;
}

/*
*Returns the full path of the binaries folder.
*/
char *cserver::getPath()
{
	return path;
}
/*
*Returns the default filename.
*/
char *cserver::getDefaultFilenamePath(u_long ID)
{
	if(ID<nDefaultFilename)
		return defaultFilename+ID*MAX_PATH;
	else
		return 0;
}
/*
*Returns the name of the server(the name of the current PC).
*/
char *cserver::getServerName()
{
	return serverName;
}
/*
*Returns 1 if we use personalized errors page
*0 if we don't use personalized errors page.
*/
int cserver::mustUseMessagesFiles()
{
	return useMessagesFiles;
}
/*
*Returns if we use the logon.
*/
int cserver::mustUseLogonOption()
{
	return useLogonOption;
}
/*
*Returns the file name of the css used to browse a directory.
*/
char *cserver::getBrowseDirCSS()
{ 
	return browseDirCSSpath;
}
/*
*Gets the number of threads.
*/
u_long cserver::getNumThreads()
{
	return nThreads;
}
/*
*Returns a comma-separated local machine IPs list.
*For example: 192.168.0.1,61.62.63.64,65.66.67.68.69
*/
char *cserver::getAddresses()
{
	return ipAddresses;
}
