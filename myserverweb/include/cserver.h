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
#ifndef CSERVER_IN
#define CSERVER_IN

#include "../stdafx.h"
#include "../include/clientsThread.h"
#include "../include/utility.h"
#include "../include/cXMLParser.h"
#include "../include/utility.h"
#include "../include/HTTPmsg.h"
#include "../include/Response_RequestStructs.h"
#include "../include/connectionstruct.h"
#include "../include/sockets.h"
#include "../include/MIME_manager.h"
#include "../include/vhosts.h"
#include "../include/connectionstruct.h"


#ifdef WIN32
unsigned int __stdcall listenServer(void* pParam);
#else
void* listenServer(void* pParam);
#endif

extern char msgSending[33];
extern char msgRunOn[33];
extern char msgFolderContents[33];
extern char msgFile[33];
extern char msgLModify[33];
extern char msgSize[33];
extern char msgNewConnection[33];
extern char msgErrorConnection[33];
extern char msgAtTime[33];

struct listenThreadArgv
{
	u_long protID;
	u_long port;
	MYSERVER_SOCKET serverSocket;
};

class cserver
{
#ifdef WIN32
	friend  unsigned int __stdcall listenServer(void* pParam);
	friend  unsigned int __stdcall startClientsTHREAD(void* pParam);
#else
	friend  void* listenServer(void* pParam);
	friend  void* startClientsTHREAD(void* pParam);
#endif
	friend class ClientsTHREAD;
#ifdef WIN32
	friend LRESULT CALLBACK MainWndProc(HWND,UINT,WPARAM,LPARAM);
#endif
#ifdef WIN32
	friend int __stdcall control_handler (u_long control_type);
#else
	friend int control_handler (u_long control_type);
#endif
private:
	cXMLParser configurationFileManager;
	cXMLParser languageParser;
	char serverName[MAX_COMPUTERNAME_LENGTH+1];
	char languageFile[MAX_PATH];
	char systemPath[MAX_PATH];
	char path[MAX_PATH];
	char browseDirCSSpath[MAX_PATH];
	char *defaultFilename;
	int nDefaultFilename;
	char warningsFileLogName[MAX_PATH];
	char accessesFileLogName[MAX_PATH];
	char serverAdmin[32];
	ClientsTHREAD threads[MAXIMUM_PROCESSORS];
	u_long nThreads;
	u_long verbosity;
	int useMessagesFiles;
	u_long buffersize;
	u_long buffersize2;
	u_long getNumConnections();
	void initialize(int);
	int addConnection(MYSERVER_SOCKET,MYSERVER_SOCKADDRIN*,CONNECTION_PROTOCOL);
	LPCONNECTION findConnection(MYSERVER_SOCKET);
	u_long connectionTimeout;
	u_long socketRcvTimeout;
	u_long maxLogFileSize;
	void controlSizeLogFile();
	void createServerAndListener(u_long,u_long);
	vhostmanager vhostList;
public:
	u_short port_HTTP;
	MIME_Manager mimeManager;
	MYSERVER_FILE_HANDLE warningsLogFile;
	MYSERVER_FILE_HANDLE accessesLogFile;
	char  *getSystemPath();
	char  *getPath();
	u_long getNumThreads();
	char  *getDefaultFilenamePath(u_long ID=0);
	char *getBrowseDirCSS();
	char  *getServerName();
	u_long  getVerbosity();
	char *getServerAdmin();
	int  mustUseMessagesFiles();
	int  mustUseLogonOption();
	void  setVerbosity(u_long);
	void start();
	void stop();
	void terminate();
}; 
extern class cserver *lserver;
#ifdef WIN32
LRESULT CALLBACK MainWndProc(HWND,UINT,WPARAM,LPARAM); 
#endif

#endif
