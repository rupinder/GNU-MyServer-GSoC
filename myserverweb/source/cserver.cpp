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
#include "../include/cserver.h"
#include "../include/security.h"
#include "../include/stringutils.h"
extern "C" {
#ifdef WIN32
#include <Ws2tcpip.h>
#include <direct.h>
#else
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef __linux__
#include <pthread.h>
#endif
#endif
}
#include "../include/sockets.h"
#include "../include/isapi.h"

#ifndef WIN32
#define LPINT int *
#define INVALID_SOCKET -1
#define WORD unsigned int
#define BYTE unsigned char
#define MAKEWORD(a, b) ((WORD) (((BYTE) (a)) | ((WORD) ((BYTE) (b))) << 8)) 
#endif

/*
*These variables are the unique istance of the class cserver in the application and the flag
*mustEndServer. When mustEndServer is true all the threads are stopped and the application stop
*its execution.
*/
cserver *lserver=0;
int mustEndServer;

/*
*These messages are loaded by the application on the startup.
*/
char msgSending[33];
char msgNewConnection[33];
char msgErrorConnection[33];
char msgAtTime[33];
char msgRunOn[33];
char msgFolderContents[33];
char msgFile[33];
char msgLModify[33];
char msgSize[33];

void cserver::start()
{
	u_long i;
	/*
	*Set the current working directory.
	*/
	ms_setcwdBuffer();
	mustEndServer=false;
	memset(this, 0, sizeof(cserver));


	/*
	*If another instance of the class exists destroy it.
	*/
	if(lserver)
	{
		mustEndServer=true;
		lserver->terminate();
		mustEndServer=false;
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
#else
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
	sprintf(software_signature,"************myServer %s************",versionOfSoftware);
	i=lstrlen(software_signature);
	while(i--)
		printf("*");
    printf("\n%s\n",software_signature);
	i=lstrlen(software_signature);
	while(i--)
		printf("*");
	printf("\n");
	free(software_signature);

	/*
	*Setup the server configuration.
	*/
    printf("Initializing server configuration...\n");

	int OSVer=ms_getOSVersion();

	initialize(OSVer);
	
	languageParser.open(languageFile);
	printf("%s\n",languageParser.getValue("MSG_LANGUAGE"));

	/*
	*These are the defaults values of these strings.
	*/
	lstrcpy(msgSending,"Sending");
	lstrcpy(msgRunOn,"Running on");
	lstrcpy(msgFolderContents,"Contents of folder");
	lstrcpy(msgNewConnection,"Connection from");
	lstrcpy(msgErrorConnection,"Error connection from");
	lstrcpy(msgAtTime,"at time");

#ifdef WIN32
	/*
	*Under WIN32 initialize ISAPI.
	*/
	initISAPI();
#endif	

	/*
	*Load the strings buffers with the right values.
	*We do this for don't call parser while the application execution
	*There are two good reasons to do this:
	*1)Application performance
	*2)Avoid of errors due to the parser
	*/
	if(lstrcmpi(languageParser.getValue("MSG_SENDING"),"NONE"))
		lstrcpy(msgSending,languageParser.getValue("MSG_SENDING"));
	if(lstrcmpi(languageParser.getValue("MSG_RUNON"),"NONE"))
		lstrcpy(msgRunOn,languageParser.getValue("MSG_RUNON"));
	if(lstrcmpi(languageParser.getValue("MSG_FOLDERCONT"),"NONE"))
		lstrcpy(msgFolderContents,languageParser.getValue("MSG_FOLDERCONT"));
	if(lstrcmpi(languageParser.getValue("MSG_FILE"),"NONE"))
		lstrcpy(msgFile,languageParser.getValue("MSG_FILE"));
	if(lstrcmpi(languageParser.getValue("MSG_LMODIFY"),"NONE"))
		lstrcpy(msgLModify,languageParser.getValue("MSG_LMODIFY"));
	if(lstrcmpi(languageParser.getValue("MSG_SIZE"),"NONE"))
		lstrcpy(msgSize,languageParser.getValue("MSG_SIZE"));
	if(lstrcmpi(languageParser.getValue("MSG_NEWCONNECTION"),"NONE"))
		lstrcpy(msgNewConnection,languageParser.getValue("MSG_NEWCONNECTION"));
	if(lstrcmpi(languageParser.getValue("MSG_ERRORCONNECTION"),"NONE"))
		lstrcpy(msgErrorConnection,languageParser.getValue("MSG_ERRORCONNECTION"));
	if(lstrcmpi(languageParser.getValue("MSG_ATTIME"),"NONE"))
		lstrcpy(msgAtTime,languageParser.getValue("MSG_ATTIME"));


	printf("%s\n",languageParser.getValue("MSG_SERVER_CONF"));

	/*
	*The guestLoginHandle value is filled by the call to cserver::initialize.
	*/
	if(guestLoginHandle==0)
	{
        preparePrintError();		
		printf("%s\n",languageParser.getValue("AL_NO_SECURITY"));
        endPrintError();
		useLogonOption=false;
	}

	/*
	*Startup the socket library.
	*/
	printf("%s\n",languageParser.getValue("MSG_ISOCK"));
	int err= ms_startupSocketLib(/*MAKEWORD( 2, 2 )*/MAKEWORD( 1, 1));
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
	ms_getComputerName(serverName,(u_long)sizeof(serverName));
	printf("%s: %s\n",languageParser.getValue("MSG_GETNAME"),serverName);

	/*
	*Determine all the IP addresses of the local machine.
	*/
	MYSERVER_HOSTENT *localhe=ms_gethostbyname(serverName);
	in_addr ia;
	for(i=0;localhe->h_addr_list[i];i++)
	{
#ifdef WIN32
		ia.S_un.S_addr = *((u_long FAR*) (localhe->h_addr_list[i]));
#else
		ia.s_addr = *((u_long *) (localhe->h_addr_list[i]));
#endif
		printf("%s #%u: %s\n",languageParser.getValue("MSG_ADDRESS"),i,inet_ntoa(ia));
	}
	

	/*
	*Load the MSCGI library.
	*/
	mscgiLoaded=loadMSCGILib();
	/*
	*Load the MIME types.
	*/
	printf("%s\n",languageParser.getValue("MSG_LOADMIME"));
	if(int nMIMEtypes=mimeManager.load("MIMEtypes.txt"))
		printf("%s: %i\n",languageParser.getValue("MSG_MIMERUN"),nMIMEtypes);
	else
	{
        preparePrintError();
		printf("%s\n",languageParser.getValue("ERR_LOADMIME"));
        endPrintError();
		return;
	}

	printf("%s %u\n",languageParser.getValue("MSG_NUM_CPU"),ms_getCPUCount());

#ifdef WIN32
	unsigned int ID;
#else
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
		threads[i].id=i;
#ifdef WIN32
		_beginthreadex(NULL,0,&::startClientsTHREAD,&threads[i].id,0,&ID);
#endif
#ifdef __linux__
		pthread_create(&ID, NULL, &::startClientsTHREAD, (void *)&(threads[i].id));
#endif
		printf("%s\n",languageParser.getValue("MSG_THREADR"));
	}

	ms_getdefaultwd(path,MAX_PATH);
	/*
	*Then we create here all the listens threads. Check that all the port used for listening
	*have a listen thread.
	*/
	printf("%s\n",languageParser.getValue("MSG_LISTENT"));

	vhostList.loadConfigurationFile("virtualhosts.txt");
	
	/*
	*Create the listens threads.
	*Every port uses a thread.
	*/
	for(vhostmanager::sVhostList *list=vhostList.getvHostList();list;list=list->next)
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
				default: 
					break; 
			} 
		}
#else
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
	MYSERVER_SOCKET serverSocket=ms_socket(AF_INET,SOCK_STREAM,0);
	MYSERVER_SOCKADDRIN sock_inserverSocket;
	if(serverSocket==INVALID_SOCKET)
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

#ifndef WIN32
	/*
	*Under the unix environment the application needs some time before create a new socket
	*for the same address. To avoid this behavior we use the current code.
	*/
	int optvalReuseAddr=1;
	if(ms_setsockopt(serverSocket,SOL_SOCKET,SO_REUSEADDR,(const char *)&optvalReuseAddr,sizeof(optvalReuseAddr))<0)
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

	if(ms_bind(serverSocket,(sockaddr*)&sock_inserverSocket,sizeof(sock_inserverSocket))!=0)
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
	if (ms_listen(serverSocket,SOMAXCONN))
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
#else
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
		asock=ms_accept(serverSocket,(struct sockaddr*)&asock_in,(LPINT)&asock_inLen);
		if(asock==0)
			continue;
		if(asock==INVALID_SOCKET)
			continue;
		lserver->addConnection(asock,&asock_in);
	}
	/*
	*When the flag mustEndServer is true end current thread and clean the socket used for listening.
	*/

	ms_shutdown(serverSocket, 2);
	ms_closesocket(serverSocket);
#ifdef WIN32
	_endthread();
#endif
#ifdef __linux__
	pthread_exit(0);
#endif

	return 0;

}
/*
*Returns the numbers of active connections on all the threads.
*/
u_long cserver::getNumConnections()
{
	/*
	*Get the number of connections in all the threads.
	*/
	u_long nConnections=0;
	for(u_long i=0;i<nThreads;i++)
	{
		nConnections+=threads[i].nConnections;
	}
	/*
	Returns the number of all the connections.
	*/
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
	mustEndServer=true;
}

void cserver::terminate()
{
	/*
	*If the guestLoginHandle is allocated close it.
	*/
	if(useLogonOption)
		ms_cleanLogonUser(&guestLoginHandle);

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
	cleanupISAPI();
#endif	
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
		printf("myServer is stopped\n\n");
	}
}
/*
*Get the server administrator e-mail address.
*/
char *cserver::getServerAdmin()
{
	return serverAdmin;
}
/*
*Here is loaded the configuration of the server.
*The configuration file is a pseudo-XML file.
*/
void cserver::initialize(int OSVer)
{
	/*
	*Store the default values.
	*/
	u_long nThreadsA=1;
	u_long nThreadsB=0;
	socketRcvTimeout = 10;
	useLogonOption = true;
	guestLoginHandle=0;
	connectionTimeout = SEC(25);
	lstrcpy(guestLogin,"myServerUnknown");
	lstrcpy(languageFile,"languages/english.xml");
	lstrcpy(guestPassword,"myServerUnknown");
	browseDirCSSpath[0]='\0';
	mustEndServer=false;
	verbosity=1;
	buffersize=1024*1024;
	buffersize2=1024*1024;
	serverAdmin[0]='\0';

	useMessagesFiles=true;
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
		buffersize=buffersize2=(u_long)atol(data);
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

	data=configurationFileManager.getValue("GUEST_LOGIN");
	if(data)
	{
		lstrcpy(guestLogin,data);
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
	nThreads=nThreadsA*ms_getCPUCount()+nThreadsB;

	data=configurationFileManager.getValue("GUEST_PASSWORD");
	if(data)
	{
		lstrcpy(guestPassword,data);
	}
	/*
	*Determine the number of default filenames written in the configuration file.
	*/
	nDefaultFilename=0;
	for(;;)
	{
		char xmlMember[21];
		sprintf(xmlMember,"DEFAULT_FILENAME%i",nDefaultFilename);
		if(configurationFileManager.getValue(xmlMember)==NULL)
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
		int i;
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


	data=configurationFileManager.getValue("USE_ERRS_FILES");
	if(data)
	{
		if(!lstrcmpi(data,"YES"))
			useMessagesFiles=true;
		else
			useMessagesFiles=false;
	}

	data=configurationFileManager.getValue("USE_LOGON_OPTIONS");
	if(data)
	{
		if(!lstrcmpi(data,"YES"))
			useLogonOption=true;
		else
			useLogonOption=false;
	}

	data=configurationFileManager.getValue("MAX_LOG_FILE_SIZE");
	if(data)
	{
		maxLogFileSize=(u_long)atol(data);
	}
	
	configurationFileManager.close();
	
	/*
	*Actually is supported only the WinNT access
	*security options.
	*The useLogonOption variable is used like a intean,
	*in the future this can be used like an ID of the
	*security access engine used.
	*Logon options cannot be used onto Win9X family.
	*/
	useLogonOption=useLogonOption && (OSVer!=OS_WINDOWS_9X);
	/*
	*Do the ms_logon of the guest user.
	*/
	ms_logonGuest();

}
/*
*Get the max size of the logs file
*/
int cserver::getMaxLogFileSize()
{
	return maxLogFileSize;
}

/*
*This function dispatch a new connection to a thread.
*/
int cserver::addConnection(MYSERVER_SOCKET s,MYSERVER_SOCKADDRIN *asock_in)
{
	if(s==0)
		return false;
	static int ret;
	ret=true;
	
	char ip[32];
	char myIp[32];
	MYSERVER_SOCKADDRIN  localsock_in;
#ifdef WIN32
	ZeroMemory(&localsock_in,sizeof(localsock_in));
#else
	memset(&localsock_in, 0, sizeof(localsock_in));
#endif
	int dim=sizeof(localsock_in);
	ms_getsockname(s,(MYSERVER_SOCKADDR*)&localsock_in,&dim);

	strncpy(ip, inet_ntoa(asock_in->sin_addr), 32); // NOTE: inet_ntop supports IPv6
	strncpy(myIp, inet_ntoa(localsock_in.sin_addr), 32); // NOTE: inet_ntop supports IPv6


	int port=ntohs((*asock_in).sin_port);
	int myport=ntohs(localsock_in.sin_port);


	static u_long local_nThreads=0;
	ClientsTHREAD *ct=&threads[local_nThreads];
	if(!ct->addConnection(s,asock_in,&ip[0],&myIp[0],port,myport))
	{
		ret=false;
		ms_closesocket(s);
	}

	if(++local_nThreads>=nThreads)
		local_nThreads=0;
	return ret;
}
/*
*Find a connection passing its socket.
*/
LPCONNECTION cserver::findConnection(MYSERVER_SOCKET s)
{
	LPCONNECTION c=NULL;
	for(u_long i=0;i<nThreads;i++)
	{
		c=threads[i].findConnection(s);
		if((i==nThreads-1) && (c!=NULL))
			return c;
	}
	return NULL;
}
/*
*Returns the full path of the web folder.
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
*Returns true if we use personalized errors page
*false if we don't use personalized errors page.
*/
int cserver::mustUseMessagesFiles()
{
	return useMessagesFiles;
}
/*
*Returns if we use the ms_logon.
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
