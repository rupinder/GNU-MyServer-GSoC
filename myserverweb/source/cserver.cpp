/*
*MyServer
*Copyright (C) 2002, 2003, 2004 The MyServer Team
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
#include "../include/myserver_regex.h"

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
#define max(a, b) ((a>b)?a:b)
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
	u_long configsCheck=0;
	time_t myserver_main_conf;
	time_t myserver_hosts_conf;
	time_t myserver_mime_conf;
	/*! Set everything to 0.  */
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
	char *software_signature=new char[200];
	if(software_signature)
	{
		sprintf(software_signature, "************MyServer %s************", versionOfSoftware);
	
		i=(u_long)strlen(software_signature);
		while(i--)
			printf("*");
		printf("\n%s\n", software_signature);
		i=(u_long)strlen(software_signature);
		while(i--)
			printf("*");
		printf("\n");
		delete [] software_signature;
	}
	/*!
	*Set the current working directory.
	*/
	setcwdBuffer();
	
	cXMLParser::startXML();
	/*!
	*Setup the server configuration.
	*/
    printf("Initializing server configuration...\n");
  int err = 0;
	int os_ver=getOSVersion();

	err = initialize(os_ver);
  if(err)
    return;

	/*! Initialize the SSL library.  */
#ifndef DO_NOT_USE_SSL
	SSL_library_init();
	SSL_load_error_strings();
#endif
	printf("%s\n", languageParser.getValue("MSG_SERVER_CONF"));

	/*! *Startup the socket library.  */
	printf("%s\n", languageParser.getValue("MSG_ISOCK"));
	err= startupSocketLib(/*!MAKEWORD( 2, 2 )*/MAKEWORD( 1, 1));
	if (err != 0) 
	{ 

    preparePrintError();
		printf("%s\n", languageParser.getValue("ERR_ISOCK"));
		endPrintError();
		return; 
	} 
	printf("%s\n", languageParser.getValue("MSG_SOCKSTART"));

	/*!
	*Get the name of the local machine.
	*/
	MYSERVER_SOCKET::gethostname(serverName, (u_long)sizeof(serverName));
	printf("%s: %s\n", languageParser.getValue("MSG_GETNAME"), serverName);

	/*!
	*Determine all the IP addresses of the local machine.
	*/
	MYSERVER_HOSTENT *localhe=MYSERVER_SOCKET::gethostbyname(serverName);
	in_addr ia;
	ipAddresses[0]='\0';
	printf("Host: %s\r\n", serverName);
	if(localhe)
	{
		/*Print all the interfaces IPs*/
		for(i=0;(localhe->h_addr_list[i])&&(i< MAX_ALLOWED_IPs);i++)
		{
#ifdef WIN32
			ia.S_un.S_addr = *((u_long FAR*) (localhe->h_addr_list[i]));
#endif
#ifdef NOT_WIN
			ia.s_addr = *((u_long *) (localhe->h_addr_list[i]));
#endif
			printf("%s #%u: %s\n", languageParser.getValue("MSG_ADDRESS"), 
             (u_int)(i+1), inet_ntoa(ia));
			sprintf(&ipAddresses[strlen(ipAddresses)], "%s%s", 
              strlen(ipAddresses)?", ":"", inet_ntoa(ia));
		}
	}
	loadSettings();

	myserver_main_conf=MYSERVER_FILE::getLastModTime(main_configuration_file);
	myserver_hosts_conf=MYSERVER_FILE::getLastModTime(vhost_configuration_file);
	myserver_mime_conf=MYSERVER_FILE::getLastModTime(mime_configuration_file);
	
	/*!
	*Keep thread alive.
	*When the mustEndServer flag is set to True exit
	*from the loop and terminate the server execution.
	*/
	while(!mustEndServer)
	{
		wait(500);
		configsCheck++;
		/*! Do not check for modified configuration files every cycle. */
		if(configsCheck>10)
		{
			time_t myserver_main_conf_now=MYSERVER_FILE::getLastModTime(main_configuration_file);
			time_t myserver_hosts_conf_now=MYSERVER_FILE::getLastModTime(vhost_configuration_file);
			time_t myserver_mime_conf_now=MYSERVER_FILE::getLastModTime(mime_configuration_file);
			/*If a configuration file was modified reboot the server*/
			if((myserver_main_conf_now!=-1) && (myserver_hosts_conf_now!=-1)  && (myserver_mime_conf_now!=-1))
			{
				if( (myserver_main_conf_now != myserver_main_conf)  || (myserver_hosts_conf_now != myserver_hosts_conf)  || (myserver_mime_conf_now != myserver_mime_conf)  )
				{
					reboot();
					/*Store new mtime values*/
					myserver_main_conf = myserver_main_conf_now;
					myserver_hosts_conf=myserver_hosts_conf_now;
					myserver_mime_conf=myserver_mime_conf_now;
				}
				configsCheck=0;
			}
			else
				configsCheck=7;/*If there are problems in loading mtimes check again after a bit*/
			
		}
#ifdef WIN32
		DWORD eventsCount, cNumRead, i; 
		INPUT_RECORD irInBuf[128]; 
		/*ReadConsoleInput is a blocking call, so be sure that there are events before call it*/
		err = GetNumberOfConsoleInputEvents(GetStdHandle(STD_INPUT_HANDLE), &eventsCount);
		if(!err)
			eventsCount = 0;
		while(eventsCount--)
		{
			if(!mustEndServer)
				ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), irInBuf, 128, &cNumRead);
			else
				break;
			for (i = 0; i < cNumRead; i++) 
			{
				switch(irInBuf[i].EventType) 
				{ 
					case KEY_EVENT:
						if(irInBuf[i].Event.KeyEvent.wRepeatCount!=1)
							continue;
						if(irInBuf[i].Event.KeyEvent.wVirtualKeyCode=='c'||
               irInBuf[i].Event.KeyEvent.wVirtualKeyCode=='C')
						{
							if((irInBuf[i].Event.KeyEvent.dwControlKeyState & 
                  LEFT_CTRL_PRESSED)|
                 (irInBuf[i].Event.KeyEvent.dwControlKeyState & 
                  RIGHT_CTRL_PRESSED))
							{
								printf ("%s\n", languageParser.getValue("MSG_SERVICESTOP"));
								this->stop();
							}
						}
						break; 
				} 
			}
		}
#endif
	}
	this->terminate();
	finalCleanup();
}
/*!
 *Do the final cleanup. Called only once.
 */
void cserver::finalCleanup()
{
	cXMLParser::cleanXML();
  freecwdBuffer();
}
/*!
 *This function is used to create a socket server and a thread listener for a protocol.
 */
int cserver::createServerAndListener(u_long port)
{
	/*!
   *Create the server socket.
   */
	printf("%s\n", languageParser.getValue("MSG_SSOCKCREATE"));
	MYSERVER_SOCKET serverSocket;
	serverSocket.socket(AF_INET, SOCK_STREAM, 0);
	MYSERVER_SOCKADDRIN sock_inserverSocket;
	if(serverSocket.getHandle()==(MYSERVER_SOCKET_HANDLE)INVALID_SOCKET)
	{
		preparePrintError();
		printf("%s\n", languageParser.getValue("ERR_OPENP"));
		endPrintError();
		return 0;
	}
	printf("%s\n", languageParser.getValue("MSG_SSOCKRUN"));
	sock_inserverSocket.sin_family=AF_INET;
	sock_inserverSocket.sin_addr.s_addr=htonl(INADDR_ANY);
	sock_inserverSocket.sin_port=htons((u_short)port);

#ifdef NOT_WIN
	/*!
   *Under the unix environment the application needs some time before
   * create a new socket
   *for the same address. To avoid this behavior we use the current code.
   */
	int optvalReuseAddr=1;
	if(serverSocket.setsockopt(SOL_SOCKET, SO_REUSEADDR, 
                             (const char *)&optvalReuseAddr, 
                             sizeof(optvalReuseAddr))<0)
  {
    preparePrintError();
		printf("%s setsockopt\n", languageParser.getValue("ERR_ERROR"));
    endPrintError();
		return 0;
	}
#endif
	/*!
   *Bind the port.
   */
	printf("%s\n", languageParser.getValue("MSG_BIND_PORT"));

	if(serverSocket.bind((sockaddr*)&sock_inserverSocket, sizeof(sock_inserverSocket))!=0)
	{
		preparePrintError();
		printf("%s\n", languageParser.getValue("ERR_BIND"));
    endPrintError();
		return 0;
	}
	printf("%s\n", languageParser.getValue("MSG_PORT_BINDED"));

	/*!
   *Set connections listen queque to max allowable.
   */
	printf("%s\n", languageParser.getValue("MSG_SLISTEN"));
	if (serverSocket.listen(SOMAXCONN))
	{ 
    preparePrintError();
		printf("%s\n", languageParser.getValue("ERR_LISTEN"));
    endPrintError();	
		return 0; 
	}

	printf("%s: %u\n", languageParser.getValue("MSG_LISTEN"), (u_int)port);

	printf("%s\n", languageParser.getValue("MSG_LISTENTR"));
	/*!
   *Create the listen thread.
   */
	listenThreadArgv* argv=new listenThreadArgv;
	argv->port=port;
	argv->serverSocket=serverSocket;

	myserver_thread_ID t_id;
	
	myserver_thread::create(&t_id,  &::listenServer,  (void *)(argv));
	
	return (t_id)?1:0;
}

/*!
 *Create a listen thread for every port used by MyServer.
 */
void cserver::createListenThreads()
{
	/*!
   *Create the listens threads.
   *Every port uses a thread.
   */
	for(vhostmanager::sVhostList *list=vhostList->getvHostList();list;list=list->next )
	{
		int needThread=1;
		vhostmanager::sVhostList *list2=vhostList->getvHostList();
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
				printf("%s: Listen threads\n", languageParser.getValue("ERR_ERROR"));
				endPrintError();
				return;
			}
		}
	}


}

/*!
 *This is the thread that listens for a new connection on the 
 *port specified by the protocol.
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
#ifdef WIN32
		if(serverSocket.dataOnRead()==0)
		{
			wait(10);
			continue;
		}
#else
		wait(10);
#endif
		asock = serverSocket.accept((struct sockaddr*)&asock_in, 
                                (LPINT)&asock_inLen);
		if(asock.getHandle()==0)
			continue;
		if(asock.getHandle()==(MYSERVER_SOCKET_HANDLE)INVALID_SOCKET)
			continue;
		asock.setServerSocket(&serverSocket);
		lserver->addConnection(asock, &asock_in);
	}
	
	/*!
   *When the flag mustEndServer is 1 end current thread and clean
   *the socket used for listening.
   */
	serverSocket.shutdown( SD_BOTH);
	char buffer[256];
	int err;
	do
	{
		err=serverSocket.recv(buffer, 256, 0);
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
			printf("%s\n", languageParser.getValue("MSG_STOPT"));
		threads[i].stop();
		if(verbosity>1)
			printf("%s\n", languageParser.getValue("MSG_TSTOPPED"));
	}
	i=0;
	while(lserver->getListeningThreadCount() && (i++<10))
	{
		wait(1000);
	}

	if(verbosity>1)
	{
		printf("%s\n", languageParser.getValue("MSG_MEMCLEAN"));
	}
	stopThreads();
	
	/*!
   *If there are open connections close them.
   */
	if(connections)
	{
		clearAllConnections();
	}
  delete [] path;
  delete [] languages_path;
  delete [] languageFile;
	delete vhostList;
  path = 0;
  languages_path = 0;
  vhostList = 0;
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
		delete [] threads;
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
		wait(200);
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
int cserver::initialize(int /*!os_ver*/)
{
#ifdef WIN32
	envString=GetEnvironmentStrings();
#endif
	/*!
   *Create the mutex for the connections.
   */
	c_mutex = new myserver_mutex();
	/*!
   *Store the default values.
   */
	u_long nThreadsA=1;
	u_long nThreadsB=0;
	socketRcvTimeout = 10;
	connectionTimeout = SEC(25);
	mustEndServer=0;
	verbosity=1;
	maxConnections=0;
	serverAdmin[0]='\0';
	
#ifndef WIN32
	/*! Do not use the files in the directory /usr/share/myserver/languages if 
   *exists a local directory.*/
	if(MYSERVER_FILE::fileExists("languages"))
	{
    int languages_pathLen = strlen ("languages/") + 1 ;
    languages_path = new char[languages_pathLen];
    if(languages_path == 0)
    {
      preparePrintError();
			printf("%s: Alloc memory\n", languageParser.getValue("ERR_ERROR"));
			endPrintError();
    }
		strcpy(languages_path,"languages/");
	}
	else
	{
#ifdef PREFIX
    int languages_pathLen = strlen(PREFIX)+strlen("/share/myserver/languages/") + 1 ;
    languages_path = new char[languages_pathLen];
    if(languages_path == 0)
    {
      preparePrintError();
			printf("%s: Alloc memory\n", languageParser.getValue("ERR_ERROR"));
			endPrintError();
      return 1;
    }
    sprintf(languages_path,"%s/share/myserver/languages/", PREFIX ) ;
#else
    /*! Default PREFIX is /usr/. */
    int languages_pathLen = strlen("/usr/share/myserver/languages/") + 1 ;
    languages_path = new char[languages_pathLen];
    if(languages_path == 0)
    {
      preparePrintError();
			printf("%s: Alloc memory\n", languageParser.getValue("ERR_ERROR"));
			endPrintError();
      return 1;
    }
		strcpy(languages_path,"/usr/share/myserver/languages/");
#endif
	}
#endif

#ifdef WIN32
  int languages_pathLen = strlen("languages/") + 1 ;
  languages_path = new char[languages_pathLen];
  if(languages_path == 0)
  {
    preparePrintError();
    printf("%s: Alloc memory\n", languageParser.getValue("ERR_ERROR"));
    endPrintError();
    return 1;
  }
  strcpy(languages_path, "languages/" );
#endif 

#ifndef WIN32
/* Under an *nix environment look for .xml files in the following order.
*1) myserver executable working directory
*2) ~/.myserver/
*3) /etc/myserver/
*4) default files will be copied in myserver executable working	
*/
	if(MYSERVER_FILE::fileExists("myserver.xml"))
	{
		strcpy(main_configuration_file,"myserver.xml");
	}
	else if(MYSERVER_FILE::fileExists("~/.myserver/myserver.xml"))
	{
		strcpy(main_configuration_file,"~/.myserver/myserver.xml");
	}
	else if(MYSERVER_FILE::fileExists("/etc/myserver/myserver.xml"))
	{
		strcpy(main_configuration_file,"/etc/myserver/myserver.xml");
	}
	else
#endif
	/*! If the myserver.xml files doesn't exist copy it from the default one. */
	if(!MYSERVER_FILE::fileExists("myserver.xml"))
	{
		strcpy(main_configuration_file,"myserver.xml");
		MYSERVER_FILE inputF;
		MYSERVER_FILE outputF;
		int ret=inputF.openFile("myserver.xml.default", 
                            MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_IFEXISTS);
		if(ret)
		{
			preparePrintError();
			printf("%s\n", languageParser.getValue("ERR_LOADED"));
			endPrintError();
			return 1;
		}
		ret = outputF.openFile("myserver.xml", MYSERVER_FILE_OPEN_WRITE | 
                     MYSERVER_FILE_OPEN_ALWAYS);
		if(ret)
		{
			preparePrintError();
			printf("%s\n", languageParser.getValue("ERR_LOADED"));
			endPrintError();
			return 1;
		}
		char buffer[512];
		u_long nbr, nbw;
		for(;;)
		{
			inputF.readFromFile(buffer, 512, &nbr );
			if(nbr==0)
				break;
			outputF.writeToFile(buffer, nbr, &nbw);
		}
		inputF.closeFile();
		outputF.closeFile();
	}
	else
  {
		strcpy(main_configuration_file,"myserver.xml");
  }
	configurationFileManager.open(main_configuration_file);
	char *data;

	data=configurationFileManager.getValue("VERBOSITY");
	if(data)
	{
		verbosity=(u_long)atoi(data);
	}
	data=configurationFileManager.getValue("LANGUAGE");
	if(data)
	{
    int languageFileLen = strlen(languages_path) + strlen(data) + 2 ;
    languageFile = new char[languageFileLen];
    if(languageFile == 0)
    {
      preparePrintError();
			printf("%s\n", languageParser.getValue("ERR_LOADED"));
			endPrintError();
			return 1;    
    }
		sprintf(languageFile, "%s/%s", languages_path, data);	
	}
	else
	{
    int languageFileLen = strlen("languages/english.xml");
    languageFile = new char[languageFileLen];
    if(languageFile == 0)
    {
      preparePrintError();
			printf("%s\n", languageParser.getValue("ERR_LOADED"));
			endPrintError();
			return 1;    
    }
		strcpy(languageFile, "languages/english.xml");
	}

	data=configurationFileManager.getValue("BUFFER_SIZE");
	if(data)
	{
		buffersize=buffersize2=max((u_long)atol(data), 81920);
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
	* The number of the threads used by the server is:
	* N_THREADS=nThreadsForCPU*CPU_COUNT+nThreadsAlwaysActive.
	*/
	nThreads=nThreadsA*getCPUCount()+nThreadsB;

	/*! Get the max connections number to allow. */
	data=configurationFileManager.getValue("MAX_CONNECTIONS");
	if(data)
	{
		maxConnections=atoi(data);
	}
	/*! Load the server administrator e-mail. */
	data=configurationFileManager.getValue("SERVER_ADMIN");
	if(data)
	{
		lstrcpy(serverAdmin, data);
	}

	data=configurationFileManager.getValue("MAX_LOG_FILE_SIZE");
	if(data)
	{
		maxLogFileSize=(u_long)atol(data);
	}
	
	configurationFileManager.close();
	
	if(languageParser.open(languageFile))
  {
      printf("Error loading %s\n", languageFile);
      return 1;
  }
	printf("%s\n", languageParser.getValue("MSG_LANGUAGE"));
	return 0;
	
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
int cserver::addConnection(MYSERVER_SOCKET s, MYSERVER_SOCKADDRIN *asock_in)
{
	if(s.getHandle()==0)
		return 0;
	int ret=1;
	/*!
	*ip is the string containing the address of the remote host connecting to the server
	*local_ip is the address of the network adapter connected to.
	*/
	char ip[MAX_IP_STRING_LEN];
	char local_ip[MAX_IP_STRING_LEN];
	MYSERVER_SOCKADDRIN  localsock_in;

	memset(&localsock_in,  0,  sizeof(localsock_in));

	int dim=sizeof(localsock_in);
	s.getsockname((MYSERVER_SOCKADDR*)&localsock_in, &dim);

	strncpy(ip,  inet_ntoa(asock_in->sin_addr),  MAX_IP_STRING_LEN); // NOTE: inet_ntop supports IPv6
	strncpy(local_ip,  inet_ntoa(localsock_in.sin_addr),  MAX_IP_STRING_LEN); // NOTE: inet_ntop supports IPv6
  /*! Port used by the client. */
	int port=ntohs((*asock_in).sin_port);
  /*! Port connected to. */
	int myport=ntohs(localsock_in.sin_port);

	if(!addConnectionToList(s, asock_in, &ip[0], &local_ip[0], port, myport, 1))
	{
		/*! If we report error to add the connection to the thread. */
		ret=0;
    /*! Shutdown the socket both on receive that on send. */
		s.shutdown(2);
    /*! Then close it. */
		s.closesocket();
	}
	return ret;
}

/*!
*Add a new connection.
*A connection is defined using a CONNECTION struct.
*/
LPCONNECTION cserver::addConnectionToList(MYSERVER_SOCKET s, MYSERVER_SOCKADDRIN* /*asock_in*/, char *ipAddr, char *localIpAddr, int port, int localPort, int /*id*/)
{
	u_long conection_size=sizeof(CONNECTION);
	LPCONNECTION new_connection=new CONNECTION;
	if(!new_connection)
	{
		return NULL;
	}
	new_connection->check_value = CONNECTION::check_value_const;
	new_connection->connectionBuffer[0]='\0';
	new_connection->socket=s;
	new_connection->toRemove=0;
	new_connection->forceParsing=0;
	new_connection->parsing=0;
	new_connection->port=(u_short)port;
	new_connection->timeout=get_ticks();
	new_connection->dataRead = 0;
	new_connection->localPort=(u_short)localPort;
	strncpy(new_connection->ipAddr, ipAddr, MAX_IP_STRING_LEN);
	strncpy(new_connection->localIpAddr, localIpAddr, MAX_IP_STRING_LEN);
	new_connection->host = (void*)lserver->vhostList->getvHost(0, localIpAddr, (u_short)localPort);
	new_connection->login[0]='\0';
	new_connection->nTries=0;
	new_connection->password[0]='\0';
	new_connection->protocolBuffer=0;
  /*! No vhost for the connection so bail. */
	if(new_connection->host == 0) 
	{
		delete [] new_connection;
		return 0;
	}
	int doSSLhandshake=0;
	if(((vhost*)new_connection->host)->protocol > 1000	)
	{
		doSSLhandshake=1;
	}
	else if(((vhost*)new_connection->host)->protocol==PROTOCOL_UNKNOWN)
	{	
		dynamic_protocol* dp=lserver->getDynProtocol(((vhost*)(new_connection->host))->protocol_name);	
		if(dp->getOptions() & PROTOCOL_USES_SSL)
			doSSLhandshake=1;	
	}
	
	/*!Do the SSL handshake if required. */
	if(doSSLhandshake)
	{
		int ret =0;
#ifndef DO_NOT_USE_SSL
		SSL_CTX* ctx=((vhost*)new_connection->host)->getSSLContext();
		new_connection->socket.setSSLContext(ctx);
		ret=new_connection->socket.sslAccept();
		
#endif		
		if(ret<0)
		{
			/*! Free the connection on errors. */
			delete  new_connection;
			return 0;
		}
	}
	if(new_connection->host==0)
	{
		delete new_connection;
		return 0;
	}
	/*! Update the list. */
	lserver->connections_mutex_lock();
	new_connection->next = connections;
   	connections=new_connection;
	nConnections++;
	lserver->connections_mutex_unlock();
	/*
	If defined maxConnections and the number of active connections is bigger than it
	*say to the protocol that will parse the connection to remove it from the active
	*connections list.
	*/
	if(maxConnections && (nConnections>maxConnections))
		new_connection->toRemove=CONNECTION_REMOVE_OVERLOAD;

	return new_connection;
}

/*!
*Delete a connection from the list.
*/
int cserver::deleteConnection(LPCONNECTION s, int /*id*/)
{
	if(!s)
	{
		return 0;
	}
	if(s->check_value != CONNECTION::check_value_const)
	{
		return 0;
	}
	MYSERVER_SOCKET socket=s->socket;
	/*!
   *Get the access to the  connections list.
   */
	int ret=0,  err;
	/*!
   * Remove the connection from the active connections list. 
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
		delete [] s->protocolBuffer;
	delete s;

	/*! Close the socket communication. */
	socket.shutdown(SD_BOTH);
	char buffer[256];
	int buffersize=256;
	do
	{
		err=socket.recv(buffer, buffersize, 0);
	}while(err!=-1);
	socket.closesocket();

	return ret;
}

/*!
*Get a connection to parse. Be sure to have the connections access for the caller thread before use this.
*Using this without the right permissions can cause wrong data returned to the client.
*/
LPCONNECTION cserver::getConnectionToParse(int /*id*/)
{
	/*! Do nothing if there are not connections. */
	if(connections==0)
		return 0;
	/*! Stop the thread if the server is pausing.*/
	while(pausing)
	{
		wait(5);
	}
	if(connectionToParse)
	{
		/*! Be sure that connectionToParse is a valid connection struct. */
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
	connections_mutex_lock();	
	LPCONNECTION c=connections;
	LPCONNECTION next=0;
	while(c)
	{
		next=c->next;
		deleteConnection(c, 1);
		c=next;
	}
	connections_mutex_unlock();
	/*! Reset everything.	*/
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
*Gets the number of threads.
*/
u_long cserver::getNumThreads()
{
	return nThreads;
}

/*!
*Returns a comma-separated local machine IPs list.
*For example: 192.168.0.1, 61.62.63.64, 65.66.67.68.69
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

	u_long i;

#ifndef WIN32
/* Under an *nix environment look for .xml files in the following order.
 *1) myserver executable working directory
 *2) ~/.myserver/
 *3) /etc/myserver/
 *4) default files will be copied in myserver executable working	
 */
	if(MYSERVER_FILE::fileExists("MIMEtypes.xml"))
	{
		strcpy(mime_configuration_file,"MIMEtypes.xml");
	}
	else if(MYSERVER_FILE::fileExists("~/.myserver/MIMEtypes.xml"))
	{
		strcpy(mime_configuration_file,"~/.myserver/MIMEtypes.xml");
	}
	else if(MYSERVER_FILE::fileExists("/etc/myserver/MIMEtypes.xml"))
	{
		strcpy(mime_configuration_file,"/etc/myserver/MIMEtypes.xml");
	}
	else
#endif
	/*! If the MIMEtypes.xml files doesn't exist copy it from the default one. */
	if(!MYSERVER_FILE::fileExists("MIMEtypes.xml"))
	{
		strcpy(mime_configuration_file,"MIMEtypes.xml");
		MYSERVER_FILE inputF;
		MYSERVER_FILE outputF;
		int ret=inputF.openFile("MIMEtypes.xml.default", MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_IFEXISTS);
		if(ret<1)
		{
			preparePrintError();
			printf("%s\n", languageParser.getValue("ERR_LOADMIME"));
			endPrintError();	
			return;
		}
		outputF.openFile("MIMEtypes.xml", MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_OPEN_ALWAYS);
		char buffer[512];
		u_long nbr, nbw;
		for(;;)
		{
			inputF.readFromFile(buffer, 512, &nbr );
			if(nbr==0)
				break;
			outputF.writeToFile(buffer, nbr, &nbw);
		}
		inputF.closeFile();
		outputF.closeFile();
	}
	else
	{
		strcpy(mime_configuration_file,"MIMEtypes.xml");
	}
	/*! Load the MIME types. */
	printf("%s\n", languageParser.getValue("MSG_LOADMIME"));
	if(int nMIMEtypes=mimeManager.loadXML(mime_configuration_file))
	{
		printf("%s: %i\n", 
           languageParser.getValue("MSG_MIMERUN"), nMIMEtypes);
	}
	else
	{
    preparePrintError();
		printf("%s\n", languageParser.getValue("ERR_LOADMIME"));
    endPrintError();
		return;
	}
	printf("%s %u\n", languageParser.getValue("MSG_NUM_CPU"), 
         (u_int)getCPUCount());

	
#ifndef WIN32
/* Under an *nix environment look for .xml files in the following order.
*1) myserver executable working directory
*2) ~/.myserver/
*3) /etc/myserver/
*4) default files will be copied in myserver executable working	
*/
	if(MYSERVER_FILE::fileExists("virtualhosts.xml"))
	{
		strcpy(vhost_configuration_file,"virtualhosts.xml");
	}
	else if(MYSERVER_FILE::fileExists("~/.myserver/virtualhosts.xml"))
	{
		strcpy(vhost_configuration_file,"~/.myserver/virtualhosts.xml");
	}
	else if(MYSERVER_FILE::fileExists("/etc/myserver/virtualhosts.xml"))
	{
		strcpy(vhost_configuration_file,"/etc/myserver/virtualhosts.xml");
	}
	else
#endif
	/*! If the virtualhosts.xml file doesn't exist copy it from the default one. */
	if(!MYSERVER_FILE::fileExists("virtualhosts.xml"))
	{
		strcpy(vhost_configuration_file,"virtualhosts.xml");
		MYSERVER_FILE inputF;
		MYSERVER_FILE outputF;
		int ret = inputF.openFile("virtualhosts.xml.default",  MYSERVER_FILE_OPEN_READ | MYSERVER_FILE_OPEN_IFEXISTS );
		if(ret<1)
		{
			preparePrintError();
			printf("%s\n", languageParser.getValue("ERR_LOADMIME"));
			endPrintError();	
			return;
		}
		outputF.openFile("virtualhosts.xml", 
                     MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_OPEN_ALWAYS);
		char buffer[512];
		u_long nbr, nbw;
		for(;;)
		{
			inputF.readFromFile(buffer, 512, &nbr );
			if(nbr==0)
				break;
			outputF.writeToFile(buffer, nbr, &nbw);
		}
		
		inputF.closeFile();
		outputF.closeFile();
	}	
	else
	{
		strcpy(vhost_configuration_file,"virtualhosts.xml");
	}
  vhostList = new vhostmanager();
  if(vhostList == 0)
  {
    return;
  }
	/*! Load the virtual hosts configuration from the xml file. */
	vhostList->loadXMLConfigurationFile(vhost_configuration_file, this->getMaxLogFileSize());

	http::loadProtocol(&languageParser, "myserver.xml");
	https::loadProtocol(&languageParser, "myserver.xml");
#ifdef NOT_WIN
	if(MYSERVER_FILE::fileExists("external"))
	{
		protocols.loadProtocols("external", &languageParser, "myserver.xml", this);
	}
  else
  {
#ifdef PREFIX
    char *path;
    int pathlen = strlen(PREFIX)+strlen("/lib/myserver/external/protocols")+1;
    path = new char[pathlen];
    
    if(path == 0)
    {
			preparePrintError();
			printf("%s: Allocating path memory\n", 
             languageParser.getValue("ERR_ERROR"));
			endPrintError(); 
    }

    sprintf(path,"%s/lib/myserver/external/protocols",PREFIX);
    protocols.loadProtocols(path, &languageParser, "myserver.xml", this);
#else
    protocols.loadProtocols("/usr/lib/myserver/external/protocols", 
                            &languageParser, "myserver.xml", this);
#endif
  }

#endif 
#if WIN32
    protocols.loadProtocols("external/protocols", &languageParser, 
                            "myserver.xml", this);
#endif

	myserver_thread_ID ID;
	if(threads)
		delete [] threads;
	threads=new ClientsTHREAD[nThreads];
	memset(threads, 0, sizeof(ClientsTHREAD)*nThreads);
	if(threads==NULL)
	{
		preparePrintError();
		printf("%s: Threads creation\n", languageParser.getValue("ERR_ERROR"));
   	endPrintError();	
	}
	for(i=0;i<nThreads;i++)
	{
		printf("%s %u...\n", languageParser.getValue("MSG_CREATET"), (u_int)i);
		threads[i].id=(u_long)(i+ClientsTHREAD::ID_OFFSET);
		myserver_thread::create(&ID,   &::startClientsTHREAD, 
                            (void *)&(threads[i].id));
		printf("%s\n", languageParser.getValue("MSG_THREADR"));
	}
  int pathlen = getdefaultwdlen();
  path = new char[pathlen];
  /*! Return 1 if we had an allocation problem.  */
  if(path == 0)
    return;
	getdefaultwd(path, pathlen);
	/*!
	*Then we create here all the listens threads. 
  *Check that all the port used for listening have a listen thread.
	*/
	printf("%s\n", languageParser.getValue("MSG_LISTENT"));
	
	createListenThreads();

	printf("%s\n", languageParser.getValue("MSG_READY"));
	printf("%s\n", languageParser.getValue("MSG_BREAK"));

}

/*!
*Reboot the server.
*/
void cserver::reboot()
{
	printf("%c\n\nRebooting.......\n\n", 0x7);/*0x7 is the beep*/
	if(mustEndServer)
		return;
	mustEndServer=1;
	terminate();
	mustEndServer=0;
	wait(5000);
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
	++listingThreads;
	connections_mutex_unlock();
}

/*!
*Decrease of one the number of active listening threads.
*/
void cserver::decreaseListeningThreadCount()
{
	connections_mutex_lock();
	--listingThreads;
	connections_mutex_unlock();
}
