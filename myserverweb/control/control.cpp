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
#include "control.h"
int status;
#ifdef WIN32
DWORD WINAPI consoleWatchDogThread(LPVOID);
HANDLE consoleModeWatchDog;
STARTUPINFO si;
PROCESS_INFORMATION pi;
SERVICE_STATUS          MyServiceStatus; 
SERVICE_STATUS_HANDLE   MyServiceStatusHandle; 
SERVICE_STATUS queryStatus();
#endif
#include "..\source\cXMLParser.cpp"
#include "..\source\Filemanager.cpp"
#include "..\source\MIME_manager.cpp"
/*
*This is the version of the Control Center and can be different from the myServer version.
*/
const char VERSION_OF_SOFTWARE[]="0.3";

/*
*To compile this program you need the library wxwindows distributed under the GNU LGPL(http://www.gnu.org/copyleft/lgpl.html) license terms.
*You can download wxwindows from http://www.wxwindows.org/.
*You need to compile it before any use. MyServer Control Center uses the Release Version of this library.
*myServer Control Center starts to use the wxwindows library from its 0.3 version, before it uses
*Windows API to create and manage a window.
*/

enum
{
    ControlCenter_Quit = 1,
	ControlCenter_RunConsole,
	ControlCenter_RunService,
	ControlCenter_RegisterService,
	ControlCenter_StopService,
	ControlCenter_StopConsole,
	ControlCenter_RemoveService,
	ControlCenter_Configure,
	ControlCenter_ConfigureMIME,
	PU_RESTORE,
	PU_EXIT,
    ControlCenter_About = wxID_ABOUT
};
enum
{
	MYSERVER_ALL_OFF=0,
	MYSERVER_CONSOLE_ON,
	MYSERVER_SERVICE_ON,
};
/*
*Unique instance of the class mainFrame.
*/
mainFrame *lmainFrame;

BEGIN_EVENT_TABLE(taskBarIcon, wxTaskBarIcon)
    EVT_MENU(PU_RESTORE, taskBarIcon::OnMenuRestore)
    EVT_MENU(PU_EXIT,    taskBarIcon::OnMenuExit)
    EVT_MENU(ControlCenter_Quit,  mainFrame::OnQuit)
    EVT_MENU(ControlCenter_RunConsole,  mainFrame::runConsole)
    EVT_MENU(ControlCenter_RunService,  mainFrame::runService)
    EVT_MENU(ControlCenter_RegisterService,  mainFrame::registerService)
    EVT_MENU(ControlCenter_StopService,  mainFrame::stopService)
    EVT_MENU(ControlCenter_StopConsole,  mainFrame::stopConsole)
	EVT_MENU(ControlCenter_Configure,mainFrame::configureWnd)
	EVT_MENU(ControlCenter_ConfigureMIME,mainFrame::configureMIME)
	EVT_MENU(ControlCenter_About, mainFrame::OnAbout)
    EVT_MENU(ControlCenter_RemoveService, mainFrame::removeService)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(mainFrame, wxFrame)
    EVT_MENU(ControlCenter_Quit,  mainFrame::OnQuit)
    EVT_MENU(ControlCenter_RunConsole,  mainFrame::runConsole)
    EVT_MENU(ControlCenter_RunService,  mainFrame::runService)
    EVT_MENU(ControlCenter_RegisterService,  mainFrame::registerService)
    EVT_MENU(ControlCenter_StopService,  mainFrame::stopService)
    EVT_MENU(ControlCenter_StopConsole,  mainFrame::stopConsole)
	EVT_MENU(ControlCenter_ConfigureMIME,mainFrame::configureMIME)
    EVT_MENU(ControlCenter_About, mainFrame::OnAbout)
	EVT_MENU(ControlCenter_Configure,mainFrame::configureWnd)
    EVT_MENU(ControlCenter_RemoveService, mainFrame::removeService)
	EVT_ICONIZE(mainFrame::iconize)
END_EVENT_TABLE()

IMPLEMENT_APP(myServerControl)

bool myServerControl::OnInit()
{
#ifdef WIN32
	ZeroMemory(&si,sizeof(si));
	ZeroMemory(&pi,sizeof(pi));
#endif
	status=MYSERVER_ALL_OFF;
    wndFrame = new mainFrame(_T("myServer Control"),wxPoint(50, 50), wxSize(320, 240));
	wndFrame->myServerControlApp=(void*)this;
    wndFrame->Show(TRUE);
    return TRUE;
}


mainFrame::mainFrame(const wxString& title, const wxPoint& pos, const wxSize& size, long style)
       : wxFrame(NULL, -1, title, pos, size, style)
{
	lmainFrame=this;/*Unique instance of this class*/
    wxIcon icon(_T("myserver.ico"),wxBITMAP_TYPE_ICO);
    SetIcon(icon);
	char version[50];
	sprintf(version,"myServer Control Center %s\n",VERSION_OF_SOFTWARE);
    m_taskBarIcon.SetIcon(icon, version);
#if wxUSE_MENUS
    menuFile = new wxMenu;
    helpMenu = new wxMenu;
	serviceMenu = new wxMenu;
	configureMenu = new wxMenu;
    helpMenu->Append(ControlCenter_About, _T("&About...\tF1"), _T("Show about dialog"));

    menuFile->Append(ControlCenter_RunConsole, _T("Run as console\t"), _T("Run myServer in console mode\t"));
    menuFile->Append(ControlCenter_StopConsole, _T("Stop the console\t"), _T("Stop myServer if running in console mode\t"));
	menuFile->AppendSeparator();
	menuFile->Append(ControlCenter_RunService, _T("Run as service\t"),_T("Run myServer as an OS service\t"));
    menuFile->Append(ControlCenter_StopService, _T("Stop the service\t"),_T("Stop the service if it is running\t"));
	menuFile->AppendSeparator();
	menuFile->Append(ControlCenter_Quit, _T("E&xit\t"), _T("Quit this program(this doesn't stop myServer execution)"));

	serviceMenu->Append(ControlCenter_RegisterService, _T("Register service\t"), _T("Register the OS service\t"));
	serviceMenu->Append(ControlCenter_RemoveService, _T("Remove service\t"), _T("Remove the OS service\t"));
	
	configureMenu->Append(ControlCenter_Configure,_T("Configure myServer"),_T("Configure myServer"));
	configureMenu->Append(ControlCenter_ConfigureMIME,_T("Configure MIME types"),_T("Configure myServer MIME types"));

    wxMenuBar *menuBar = new wxMenuBar();
    menuBar->Append(menuFile, _T("&File"));
	menuBar->Append(serviceMenu, _T("&Install/Remove service"));
	menuBar->Append(configureMenu, _T("&Configure myServer"));
    menuBar->Append(helpMenu, _T("&Help"));
    SetMenuBar(menuBar);
#endif

#if wxUSE_STATUSBAR
    CreateStatusBar(1);
    SetStatusText(_T("myServer Control Panel"));
#endif
}
void mainFrame::configureWnd(wxCommandEvent& WXUNUSED(event))
{
	configurationWnd=new configurationFrame(this,_T("Configure myServer"),wxPoint(60, 60), wxSize(640, 480));
	configurationWnd->Show(TRUE);
}

void mainFrame::iconize(wxCommandEvent& WXUNUSED(event))
{
	lmainFrame->Show(FALSE);
}

void mainFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	lmainFrame->Show(TRUE);
    lmainFrame->Close(TRUE);
}
/*
*Unregister the OS service.
*/
void mainFrame::removeService(wxCommandEvent& WXUNUSED(event))
{
    SetStatusText(_T("Removing the service"));

	SC_HANDLE service,manager;
    manager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
	if (manager)
	{
		service = OpenService (manager, "myServer", SERVICE_ALL_ACCESS);
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

	lmainFrame->SetStatusText(_T("Service removed"));
}
/*
*Display the about window.
*/
void mainFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxString msg;
    msg.Printf( _T("myServer Control Center %s\n"),VERSION_OF_SOFTWARE);
    wxMessageBox(msg, _T("About"), wxOK | wxICON_INFORMATION, this);
}
#ifdef WIN32
/*
*Wait if the myServer application is ended by itself.
*/
DWORD WINAPI consoleWatchDogThread(LPVOID param)
{
	WaitForSingleObject(pi.hProcess,INFINITE);
	if(status!=MYSERVER_ALL_OFF)
		((mainFrame*)param)->SetStatusText(_T("MyServer console mode terminated by itself"));
	TerminateThread(GetCurrentThread(),0);
	return 0;
}
#endif
/*
*Run myServer like a console.
*/
void mainFrame::runConsole(wxCommandEvent& event)
{
	lmainFrame->SetStatusText(_T("Starting myServer in console mode"));
#ifdef WIN32
	CreateProcess(NULL,"myserver.exe CONSOLE",NULL,NULL,FALSE,0,0,0,&si,&pi);
	DWORD id;
	consoleModeWatchDog=CreateThread(0,0,consoleWatchDogThread,this,0,&id);
#endif
	status=MYSERVER_CONSOLE_ON;
	lmainFrame->SetStatusText(_T("MyServer started in console mode"));
}
/*
*Run myServer like an OS service.
*/
void mainFrame::runService(wxCommandEvent& event)
{
	lmainFrame->SetStatusText(_T("Starting myServer in service mode"));
#ifdef WIN32
	SC_HANDLE service,manager;

	manager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
	if (manager)
	{
		service = OpenService (manager, "myServer", SERVICE_ALL_ACCESS);
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
	status=MYSERVER_SERVICE_ON;
	lmainFrame->SetStatusText(_T("MyServer started in service mode"));
}
/*
*Register the OS service.
*/
void mainFrame::registerService(wxCommandEvent& event)
{
	lmainFrame->SetStatusText(_T("Installing the myServer service"));

	SC_HANDLE service,manager;
	char path [MAX_PATH];
	GetCurrentDirectory(MAX_PATH,path);
	lstrcat(path,"\\");
	lstrcat(path,"myServer.exe SERVICE");
	
	manager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
	if (manager)
	{
		service = CreateService(manager,"myServer","myServer",SERVICE_ALL_ACCESS,SERVICE_WIN32_OWN_PROCESS,SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE, path,0, 0, 0, 0, 0);
		if (service)
		{
			CloseServiceHandle (service);
		}

		CloseServiceHandle (manager);
	}

	lmainFrame->SetStatusText(_T("MyServer service installed"));
}
void mainFrame::configureMIME(wxCommandEvent& event)
{
	configurationFrameMIME *configureMIMEWnd=new configurationFrameMIME(this,_T("Configure myServer MIME types"),wxPoint(70, 70), wxSize(320, 240));
	configureMIMEWnd->Show(TRUE);
}
/*
*Stop the application if run in service mode
*/
void mainFrame::stopService(wxCommandEvent& event)
{
	lmainFrame->SetStatusText(_T("Stopping the myServer service"));
#ifdef WIN32
	SC_HANDLE service,manager;
	manager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
	if (manager)
	{
		service = OpenService (manager, "myServer", SERVICE_ALL_ACCESS);
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
	status=MYSERVER_ALL_OFF;
	lmainFrame->SetStatusText(_T("MyServer service stopped"));

}
#ifdef WIN32
/*
*Retrieve the service status.
*/
SERVICE_STATUS queryStatus()
{
	SC_HANDLE service,manager;
	manager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
	if (manager)
	{
		service = OpenService (manager, "myServer", SERVICE_ALL_ACCESS);
		if (service)
		{
			QueryServiceStatus (service, &MyServiceStatus);
			CloseServiceHandle (service);
			CloseServiceHandle (manager);
		}
	}
	return MyServiceStatus;
}
#endif

/*
*Stop myServer if it is running in console mode.
*/
void mainFrame::stopConsole(wxCommandEvent& event)
{
	lmainFrame->SetStatusText(_T("Stopping myServer console mode"));
#ifdef WIN32
	if(pi.hProcess)
		TerminateProcess(pi.hProcess,0);
#endif
	status=MYSERVER_ALL_OFF;
	lmainFrame->SetStatusText(_T("MyServer console mode stopped"));
}




/*
*Functions to handle the taskbar icon
*/
void taskBarIcon::OnMouseMove(wxEvent&)
{

}
void taskBarIcon::OnLButtonDown(wxEvent&)
{

}
void taskBarIcon::OnLButtonUp(wxEvent&)
{
	lmainFrame->Show(TRUE);
}
void taskBarIcon::OnRButtonDown(wxEvent&)
{

}
void taskBarIcon::OnRButtonUp(wxEvent&)
{
	PopupMenu(lmainFrame->menuFile);
}
void taskBarIcon::OnLButtonDClick(wxEvent&)
{

}
void taskBarIcon::OnRButtonDClick(wxEvent&)
{
}

void taskBarIcon::OnMenuRestore(wxCommandEvent&)
{
}
void taskBarIcon::OnMenuExit(wxCommandEvent&)
{
}
