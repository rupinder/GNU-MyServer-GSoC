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
#include "..\include\cserver.h"
#include <Ws2tcpip.h>

cserver *lserver=0;
BOOL mustEndServer;
int err;
void cserver::start(HINSTANCE hInst)
{
	mustEndServer=FALSE;
	cserver::hInst=hInst;
	ZeroMemory(this,sizeof(cserver));

	/*
	*If another instance of the class exists destroy it
	*/
	if(lserver)
	{
		mustEndServer=TRUE;
		WaitForSingleObject(lserver->listenServerHTTPHandle,INFINITE);
		mustEndServer=FALSE;
	}
	/*
	*Save the unique instance of this class
	*/
	lserver=this;
	logFile=fopen("myServer.log","a+t");
	setLogFile(logFile);
	controlSizeLogFile();

	if(verbosity>2)
		printSystemInfoOnLogFile();
	

	/*
	*Get the OS version.
	*/
	INT OSVer=getOSVersion();
	printOSInfo(OSVer);


	/*
	*Setup the server configuration
	*/
	printf("\nInitializing server configuration...\n");
	initialize(OSVer);
	printf("Server configuration terminated\n\n");
	
	if(guestLoginHandle==0)
	{
		logFileWrite("Security access is not used, the web folder contents is accessible to anyone\n");
		printf("Security access is not used, the web folder contents is accessible to anyone\n");
		useLogonOption=FALSE;
	}

	/*
	*Startup the socket library
	*/
	WSADATA wsaData;
	printf("Initializing socket library...\n");
	if ((err = WSAStartup(/*MAKEWORD( 2, 2 )*/MAKEWORD( 1, 1), &wsaData)) != 0) 
	{ 
		logFileWrite("Error initializing socket library\n");
		printf("Error initializing socket library\n");
		return; 
	} 
	printf("Socket library was initialized...\n");

	/*
	*Create the server socket
	*/
	printf("Creating server socket...\n");
	serverSocketHTTP=socket(AF_INET,SOCK_STREAM,0);
	if(serverSocketHTTP==INVALID_SOCKET)
	{
		logFileWrite("Error opening port, port can be used by another application\n");
		printf("Error opening port, port can be used by another application\n");
		return;
	}
	printf("Server socket created\n");


	sock_inserverSocketHTTP.sin_family=AF_INET;
	sock_inserverSocketHTTP.sin_addr.s_addr=htonl(INADDR_ANY);
	sock_inserverSocketHTTP.sin_port=htons(port_HTTP);

	/*
	*Bind the HTTP port
	*/
	printf("Trying to binding HTTP port...\n");
	if(bind(serverSocketHTTP,(sockaddr*)&sock_inserverSocketHTTP,sizeof(sock_inserverSocketHTTP))!=0)
	{
		logFileWrite("Error binding port, port can be used by another application\n");
		printf("Error binding port, port can be used by another application\n");
		return;
	}
	printf("HTTP port is binded\n");

	/*
	*Set connections listen queque to max allowable
	*/
	printf("Trying to listen on HTTP port...\n");
	if (listen(serverSocketHTTP,SOMAXCONN))
	{ 
		logFileWrite("Error listening\n");
		printf("Error listening\n");
		return; 
	}
	printf("Listen on HTTP port successfully\n");


	/*
	*Get the machine name
	*/
	DWORD lenServerName=sizeof(serverName);
	if(GetComputerNameA(serverName,&lenServerName)==0)
	{
		lstrcpy(serverName,"localhost");
	}
	printf("Computer name is:%s\n",serverName);

	/*
	*If the OS support the getaddrinfo function call it
	*to get info about the address of current machine
	*/
	if((OSVer==OS_WINDOWS_2000)|(OSVer==OS_WINDOWS_XP))
	{
		addrinfo *ai;
		addrinfo *iai;
		getaddrinfo(serverName,NULL,NULL,&ai);
		iai=ai;
		WORD addresses_count=0;
		do
		{
			addresses_count++;
			sockaddr_in *sai=(sockaddr_in*)(iai->ai_addr);
			printf("Address #%u: %u.%u.%u.%u\n",addresses_count,sai->sin_addr.S_un.S_un_b.s_b1,sai->sin_addr.S_un.S_un_b.s_b2,sai->sin_addr.S_un.S_un_b.s_b3,sai->sin_addr.S_un.S_un_b.s_b4);
			iai=ai->ai_next;
		}while(iai);
		freeaddrinfo(ai);
	}

	/*
	*Load the MIME types
	*/
	printf("Loading MIME types...\n");
	if(!mimeManager.load("MIMEtypes.txt"))
		printf("MIME types loaded successfully\n");
	else
		printf("MIME types cannot be loaded\n");

	/*
	*Create the threads
	*/
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	switch(si.dwNumberOfProcessors)
	{
		case 1:
			printf("Machine with single CPU detected\n");
			break;
		default:
			printf("Machine with %u CPUs detected\n",si.dwNumberOfProcessors);
			break;
	}
	/*
	*Create a thread for every CPU.
	*/
	nThreads=si.dwNumberOfProcessors;

	unsigned int ID;
	for(DWORD i=0;i<nThreads;i++)
	{
		printf("Creating thread %u...\n",i);
		threads[i].id=i;
		threads[i].threadHandle=(HANDLE)_beginthreadex(NULL,0,&::startClientsTHREAD,&threads[i].id,0,&ID);
		printf("Thread %u created\n",i);
	}
	printf("Creating listening thread\n");
	listenServerHTTPHandle=(HANDLE)_beginthreadex(NULL,0,&::listenServerHTTP,0,0,&ID);
	
	printf("Listening thread is created\n");
	printf("myServer is now ready to accept connections\nPress Ctrl+C to break execution\n");

	/*
	*Keep thread alive.
	*When the mustEndServer flag is set to True exit
	*from the loop and terminate the server execution
	*/
	while(!mustEndServer);

	shutdown(serverSocketHTTP, 2);
	cserver::terminate();
	closesocket(serverSocketHTTP);
}
unsigned int WINAPI listenServerHTTP(void*)
{
	INT asock_inLenHTTP=sizeof(lserver->asock_inHTTP);
	while(!mustEndServer)
	{
		/*
		*Accept connections.
		*Every new connection is sended to cserver::addConnection
		*function; this function dispatch connections 
		*between the various threads.
		*/
		lserver->asockHTTP=accept(lserver->serverSocketHTTP,(struct sockaddr*)&lserver->asock_inHTTP,(LPINT)&asock_inLenHTTP);
		if(lserver->asockHTTP==0)
			continue;
		if(lserver->asockHTTP==INVALID_SOCKET)
			continue;
		
		lserver->addConnection(lserver->asockHTTP,PROTOCOL_HTTP);
	}	
	_endthreadex( 0 );
	return 0;
}
DWORD cserver::getNumConnections()
{
	/*
	*Get the number of connections in all the threads.
	*/
	DWORD nConnections=0;
	for(DWORD i=0;i<nThreads;i++)
	{
		nConnections+=threads[i].nConnections;
	}
	return nConnections;
}

DWORD cserver::getVerbosity()
{
	return verbosity;
}
void  cserver::setVerbosity(DWORD nv)
{
	verbosity=nv;
}

void cserver::stop()
{
	mustEndServer=TRUE;
}
void cserver::terminate()
{
	/*
	*If the guestLoginHandle is allocated close it.
	*/
	if(useLogonOption)
		CloseHandle(guestLoginHandle);
	/*
	*Stop server
	*/
	for(DWORD i=0;i<nThreads;i++)
	{
		if(verbosity>2)
			printf("Terminating threads...\n");
		threads[i].stop();
		if(verbosity>2)
			printf("Threads terminated\n");
	}
	if(verbosity>2)
	{
		printf("Cleaning memory...\n");
	}
	/*
	*Clean memory allocated here
	*/
	mimeManager.clean();
	for(i=0;i<nThreads;i++)
		threads[i].clean();
	if(verbosity>2)
	{
		printf("myServer is stopped\n\n");
	}
	fclose(logFile);
	_fcloseall();
}
void cserver::initialize(INT OSVer)
{
	/*
	*Here is loaded the configuration of the server.
	*The configuration file is a pseudo-XML file.
	*/

	socketRcvTimeout = 10;


	useLogonOption = TRUE;
	guestLoginHandle=0;
	connectionTimeout = SEC(25);
	lstrcpy(guestLogin,"myServerUnknown");
	lstrcpy(guestPassword,"myServerUnknown");
	lstrcpy(defaultFilename,"default.html");
	mustEndServer=FALSE;
	port_HTTP=80;
	verbosity=1;
	buffersize=1024*1024;
	buffersize2=1024*1024;
	GetCurrentDirectory(MAX_PATH,path);
	lstrcat(path,"\\web");
	GetCurrentDirectory(MAX_PATH,systemPath);
	lstrcat(systemPath,"\\system");
	useMessagesFiles=TRUE;


	configurationFileManager.open("myserver.xml");
	CHAR *data;

	data=configurationFileManager.getValue("HTTP_PORT");
	if(data)
	{
		port_HTTP=(WORD)atoi(data);
	}

	data=configurationFileManager.getValue("VERBOSITY");
	if(data)
	{
		verbosity=(DWORD)atoi(data);
	}

	data=configurationFileManager.getValue("BUFFER_SIZE");
	if(data)
	{
		buffersize=buffersize2=(DWORD)atol(data);
	}

	data=configurationFileManager.getValue("CONNECTION_TIMEOUT");
	if(data)
	{
		connectionTimeout=SEC((DWORD)atol(data));
	}


	data=configurationFileManager.getValue("GUEST_LOGIN");
	if(data)
	{
		lstrcpy(guestLogin,data);
	}

	data=configurationFileManager.getValue("GUEST_PASSWORD");
	if(data)
	{
		lstrcpy(guestPassword,data);
	}


	data=configurationFileManager.getValue("DEFAULT_FILENAME");
	if(data)
	{
		lstrcpy(defaultFilename,data);
	}


	data=configurationFileManager.getValue("WEB_DIRECTORY");
	if(data)
	{
		GetCurrentDirectory(MAX_PATH,path);
		lstrcat(path,"\\");
		lstrcat(path,data);
	}


	data=configurationFileManager.getValue("SYSTEM_DIRECTORY");
	if(data)
	{
		GetCurrentDirectory(MAX_PATH,systemPath);
		lstrcat(systemPath,"\\");
		lstrcat(systemPath,data);
	}

	data=configurationFileManager.getValue("USE_ERRORS_FILES");
	if(data)
	{
		if(!lstrcmpi(data,"YES"))
			useMessagesFiles=TRUE;
		else
			useMessagesFiles=FALSE;
	}

	data=configurationFileManager.getValue("USE_LOGON_OPTIONS");
	if(data)
	{
		if(!lstrcmpi(data,"YES"))
			useLogonOption=TRUE;
		else
			useLogonOption=FALSE;
	}

	data=configurationFileManager.getValue("MAX_LOG_FILE_SIZE");
	if(data)
	{
		maxLogFileSize=(DWORD)atol(data);
		controlSizeLogFile();
	}

	configurationFileManager.close();

	printf("Web folder:%s\nSystem folder:%s\n",path,systemPath);
	printf("myServer listen on port %u\n",port_HTTP);
	
	/*
	*Actually is supported only the WinNT access
	*security options.
	*The useLogonOption variable is used like a boolean,
	*in the future this can be used like an ID of the
	*security access engine used.
	*Logon options cannot be used onto Win9X family.
	*/
	useLogonOption=useLogonOption && (OSVer!=OS_WINDOWS_9X);

	if(useLogonOption)
		LogonUser(guestLogin,NULL,guestPassword,LOGON32_LOGON_NETWORK, LOGON32_PROVIDER_DEFAULT, &guestLoginHandle);
}

VOID cserver::controlSizeLogFile()
{
	/*
	*Control the size of the log file.
	*If this is major than maxLogFileSize
	*delete it.
	*/
	if(!logFile)
	{
		logFile=fopen("myServer.log","a+t");
	}
	DWORD fs=0;
	getFileSize(&fs,logFile);
	if(fs>maxLogFileSize)
	{
		fclose(logFile);
		DeleteFile("myServer.log");
		logFile=fopen("myServer.log","a+t");
	}
	setLogFile(logFile);
}
BOOL cserver::addConnection(SOCKET s,CONNECTION_PROTOCOL protID)
{
	if(s==0)
		return FALSE;
	static BOOL ret;
	ret=TRUE;
	if(verbosity>0)
		fprintf(logFile,"Connection from:%u.%u.%u.%u on %s:%i at time:%s\n", lserver->asock_inHTTP.sin_addr.S_un.S_un_b.s_b1, lserver->asock_inHTTP.sin_addr.S_un.S_un_b.s_b2, lserver->asock_inHTTP.sin_addr.S_un.S_un_b.s_b3, lserver->asock_inHTTP.sin_addr.S_un.S_un_b.s_b4,serverName,lserver->port_HTTP,getHTTPFormattedTime());
	fflush(logFile);

	static DWORD local_nThreads=0;
	ClientsTHREAD *ct=&threads[local_nThreads];
	if(!ct->addConnection(s,protID))
	{
		ret=FALSE;
		closesocket(s);
		if(verbosity>0)
			fprintf(logFile,"Error connection from:%i.%i.%i.%i on %s:%i at time:%s\n", lserver->asock_inHTTP.sin_addr.S_un.S_un_b.s_b1, lserver->asock_inHTTP.sin_addr.S_un.S_un_b.s_b2, lserver->asock_inHTTP.sin_addr.S_un.S_un_b.s_b3, lserver->asock_inHTTP.sin_addr.S_un.S_un_b.s_b4,serverName,lserver->port_HTTP,getHTTPFormattedTime());
	}

	if(++local_nThreads>=nThreads)
		local_nThreads=0;
	return ret;
}

LPCONNECTION cserver::findConnection(SOCKET s)
{
	LPCONNECTION c=NULL;
	for(DWORD i=0;i<nThreads;i++)
	{
		c=threads[i].findConnection(s);
		if((i==nThreads-1) && (c!=NULL))
			return c;
	}
	return NULL;
}
