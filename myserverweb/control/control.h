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
#include "resource.h" 
#include <wx/wx.h> 
#include <wx/taskbar.h>
#include "configuration.h"
#ifdef WIN32
#include <windows.h>
#define SOCKETLIBINCLUDED/*Prevent include socket headers file*/
#include "..\include\MIME_Manager.h"
#include "..\include\cXMLParser.h"
#endif          
#pragma comment(lib,"odbc32.lib")
#pragma comment(lib,"odbccp32.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"rpcrt4.lib")
#pragma comment(lib,"wsock32.lib")
#pragma comment(lib,"zlib.lib")
#pragma comment(lib,"regex.lib")
#pragma comment(lib,"png.lib")
#pragma comment(lib,"jpeg.lib")
#pragma comment(lib,"tiff.lib")
#pragma comment(lib,"wxmsw.lib")


class taskBarIcon: public wxTaskBarIcon
{
public:
    taskBarIcon() {};
    virtual void OnMouseMove(wxEvent&);
    virtual void OnLButtonDown(wxEvent&);
    virtual void OnLButtonUp(wxEvent&);
    virtual void OnRButtonDown(wxEvent&);
    virtual void OnRButtonUp(wxEvent&);
    virtual void OnLButtonDClick(wxEvent&);
    virtual void OnRButtonDClick(wxEvent&);

    void OnMenuRestore(wxCommandEvent&);
    void OnMenuExit(wxCommandEvent&);

DECLARE_EVENT_TABLE()
};
class mainFrame : public wxFrame
{
public:
#if wxUSE_MENUS
    wxMenu *menuFile;
    wxMenu *helpMenu;
	wxMenu *serviceMenu;
	wxMenu *configureMenu;
	configurationFrame* configurationWnd;
#endif
	mainFrame(const wxString& title, const wxPoint& pos, const wxSize& size,
		long style = wxDEFAULT_FRAME_STYLE);
    taskBarIcon   m_taskBarIcon;
	void *myServerControlApp;
	void OnQuit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void runConsole(wxCommandEvent& event);
	void removeService(wxCommandEvent& event);
	void iconize(wxCommandEvent& event);
	void configureWnd(wxCommandEvent& event);
	void runService(wxCommandEvent& event);
	void registerService(wxCommandEvent& event);
	void stopService(wxCommandEvent& event);
	void stopConsole(wxCommandEvent& event);

private:
	DECLARE_EVENT_TABLE()
};
extern mainFrame *lmainFrame;
class myServerControl : public wxApp
{
	mainFrame *wndFrame;
public:
	virtual bool OnInit();
};