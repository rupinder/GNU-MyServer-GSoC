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
#pragma once
#ifndef cserver_IN
#define cserver_IN

#include "..\include\ClientsTHREAD.h"
#include "..\include\utility.h"
#include "..\include\cXMLParser.h"
#include "..\include\utility.h"
#include "..\include\HTTPmsg.h"
#include "..\include\Response_RequestStructs.h"
#include "..\include\ConnectionStruct.h"
#include "..\include\sockets.h"
#include "..\include\MIME_manager.h"

unsigned int __stdcall listenServer(void* pParam);

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
	DWORD protID;
	DWORD port;
	MYSERVER_SOCKET serverSocket;
};
class cserver
{
	friend  unsigned int __stdcall listenServer(void* pParam);
	friend  unsigned int __stdcall startClientsTHREAD(void* pParam);
	friend class ClientsTHREAD;
	friend LRESULT CALLBACK MainWndProc(HWND,UINT,WPARAM,LPARAM);
	friend BOOL __stdcall control_handler (DWORD control_type);
private:
	cXMLParser configurationFileManager;
	cXMLParser languageParser;
	char serverName[MAX_COMPUTERNAME_LENGTH+1];
	char languageFile[MAX_PATH];
	char systemPath[MAX_PATH];
	char path[MAX_PATH];
	char browseDirCSSpath[MAX_PATH];
	char defaultFilename[MAX_PATH];
	char warningsFileLogName[MAX_PATH];
	char accessesFileLogName[MAX_PATH];
	ClientsTHREAD threads[MAXIMUM_PROCESSORS];
	DWORD nThreads;
	DWORD verbosity;
	BOOL useMessagesFiles;
	DWORD buffersize;
	DWORD buffersize2;
	DWORD getNumConnections();
	void initialize(int);
	BOOL addConnection(MYSERVER_SOCKET,MYSERVER_SOCKADDRIN*,CONNECTION_PROTOCOL);
	LPCONNECTION findConnection(MYSERVER_SOCKET);
	DWORD connectionTimeout;
	DWORD socketRcvTimeout;
	DWORD maxLogFileSize;
	VOID controlSizeLogFile();
	VOID createServerAndListener(DWORD,DWORD);
public:
	WORD port_HTTP;
	MIME_Manager mimeManager;
	MYSERVER_FILE_HANDLE warningsLogFile;
	MYSERVER_FILE_HANDLE accessesLogFile;
	char  *getSystemPath();
	char  *getPath();
	char  *getDefaultFilenamePath(DWORD=0);
	char *getBrowseDirCSS();
	char  *getServerName();
	DWORD  getVerbosity();
	BOOL  mustUseMessagesFiles();
	BOOL  mustUseLogonOption();
	void  setVerbosity(DWORD);
	void start(int);
	void stop();
	void terminate();
	int hInst;

}; 

LRESULT CALLBACK MainWndProc(HWND,UINT,WPARAM,LPARAM); 
#endif
