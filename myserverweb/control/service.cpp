/*
 * MyServer
 * Copyright (C) 2002, 2003, 2004 The MyServer Team
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

// Helper functions for the configure program

#ifdef WIN32
#include <windows.h>
#else
extern "C" { 
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
}
#endif

#include "../include/file.h"

#ifdef WIN32
static DWORD WINAPI consoleWatchDogThread(LPVOID);
static HANDLE consoleModeWatchDog;
static STARTUPINFO si;
static PROCESS_INFORMATION pi;
static SERVICE_STATUS          MyServiceStatus;
static SERVICE_STATUS_HANDLE   MyServiceStatusHandle;
#else
static int ConsolePid = 0;
#endif

void RunAsConsole()
{
#ifdef WIN32
   CreateProcess(NULL,"myserver.exe CONSOLE",NULL,NULL,FALSE,0,0,0,&si,&pi);
   DWORD id;
   consoleModeWatchDog=CreateThread(0,0,consoleWatchDogThread,0,0,&id);
#else
   int pid = fork();
   if(pid == 0)
     {
	if(File::fileExists("myserver"))
	  execlp("xterm", "xterm", "-e", "./myserver", NULL);
	else
	  execlp("xterm", "xterm", "-e", "myserver", NULL);
	exit(0);
     }
   ConsolePid = pid;
#endif
}

void RunAsService()
{
#ifdef WIN32
   SC_HANDLE service,manager;

   manager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
   if (manager)
     {
	service = OpenService (manager, "MyServer", SERVICE_ALL_ACCESS);
	if (service)
	  {
	     StartService(service,0,NULL);
	     while (QueryServiceStatus (service, &MyServiceStatus))
	       if (MyServiceStatus.dwCurrentState != SERVICE_START_PENDING)
		 break;
	     CloseServiceHandle (service);
	     CloseServiceHandle (manager);
	  }
     }
#endif
}

void StopConsole()
{
#ifdef WIN32
   if(pi.hProcess)
     TerminateProcess(pi.hProcess,0);
#else
   if(ConsolePid != 0)
     kill(ConsolePid, SIGTERM);
#endif
}

void StopService()
{
#ifdef WIN32
   SC_HANDLE service,manager;
   manager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
   if (manager)
     {
	service = OpenService (manager, "MyServer", SERVICE_ALL_ACCESS);
	if (service)
	  {
	     ControlService (service,SERVICE_CONTROL_STOP,&MyServiceStatus);
	     while (QueryServiceStatus (service, &MyServiceStatus))
	       if (MyServiceStatus.dwCurrentState != SERVICE_STOP_PENDING)
		 break;
	     CloseServiceHandle (service);
	     CloseServiceHandle (manager);
	  }
     }
#endif
}

void InstallService()
{
#ifdef WIN32
   SC_HANDLE service,manager;
   char path [MAX_PATH];
   GetCurrentDirectory(MAX_PATH,path);
   lstrcat(path,"\\");
   lstrcat(path,"myServer.exe SERVICE");

   manager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
   if (manager)
     {
	service = CreateService(manager,"MyServer","MyServer",SERVICE_ALL_ACCESS,SERVICE_WIN32_OWN_PROCESS,SERVICE_AUTO_START, SERVICE_ERROR_IGNORE, path,0, 0, 0, 0, 0);
	if (service)
	  {
	     CloseServiceHandle (service);
	  }
	CloseServiceHandle (manager);
     }
     /*Run the service after its installation. */
     RunAsService();
#endif
}

void RemoveService()
{
#ifdef WIN32
   SC_HANDLE service,manager;
   manager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
   if (manager)
     {
	service = OpenService (manager, "MyServer", SERVICE_ALL_ACCESS);
	if (service)
	  {
	     ControlService (service, SERVICE_CONTROL_STOP,&MyServiceStatus);
	     while (QueryServiceStatus (service, &MyServiceStatus))
	       if (MyServiceStatus.dwCurrentState != SERVICE_STOP_PENDING)
		 break;
	     DeleteService(service);
	     CloseServiceHandle (service);
	     CloseServiceHandle (manager);
	  }
     }
#endif
}

#ifdef WIN32
/*!
 * *Wait if the myServer application is ended by itself.
 * */
DWORD WINAPI consoleWatchDogThread(LPVOID param)
{
   
   WaitForSingleObject(pi.hProcess,INFINITE);
   TerminateThread(GetCurrentThread(),0);
   return 0;
}

#endif
