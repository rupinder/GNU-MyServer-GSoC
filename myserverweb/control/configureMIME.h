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
#include "stdafx.h"
#include "resource.h" 
#include <wx/wx.h> 
#include <wx/taskbar.h>
#ifdef WIN32
#include <windows.h>
#define SOCKETLIBINCLUDED/*Prevent include socket headers file*/
#include "..\include\MIME_Manager.h"
#include "..\include\cXMLParser.h"
#endif          
extern const char VERSION_OF_SOFTWARE[];


class configurationFrameMIME : public wxFrame
{
public:
	wxListBox *mimeTypesLB;
	wxListBox *extensionsLB;
	configurationFrameMIME(wxWindow *parent,const wxString& title, const wxPoint& pos, const wxSize& size,long style = wxDEFAULT_FRAME_STYLE);
	void OnQuit(wxCommandEvent& event);
	void cancel(wxCommandEvent& event);
	void ok(wxCommandEvent& event);

private:
	DECLARE_EVENT_TABLE()
};
