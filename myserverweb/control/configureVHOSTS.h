/*
*myServer
*Copyright (C) 2002 The MyServer team
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
#ifndef CONFIGUREVHOSTS_H
#define CONFIGUREVHOSTS_H
#include "stdafx.h"
#include "resource.h" 
#include <wx/wx.h> 
#include <wx/taskbar.h>
#include <wx/textctrl.h>
#ifdef WIN32
#include <windows.h>
#endif
#define SOCKETLIBINCLUDED/*Prevent include socket headers file*/
#include "../include/MIME_manager.h"
#include "../include/cXMLParser.h"
#include "../include/vhosts.h"
#include "../include/utility.h"
          
extern const char VERSION_OF_SOFTWARE[];
#define VHOSTSWNDSIZEX	640
#define VHOSTSWNDSIZEY	260

class configurationFrameVHOSTS : public wxFrame
{
public:
	vhostmanager hostmanager;
	wxButton* btnOK;
	wxButton* btnCNL;
	wxButton* btnSAVE;
	wxButton* btnADDVHOST;
	wxButton* btnREMOVEVHOST;
	wxButton* btnUPVHOST;
	wxButton* btnDOWNVHOST;
	wxButton* btnADDHOST;
	wxButton* btnADDIP;
	wxButton* btnREMOVEHOST;
	wxButton* btnREMOVEIP;
	wxListBox *vhostsLB;
	wxListBox *hostsLB;
	wxListBox *ipLB;
	wxTextCtrl *hostPort;
	wxTextCtrl *hostDoc;
	wxTextCtrl *hostSys;
	wxTextCtrl *hostAcc;
	wxTextCtrl *hostWarnings;
	wxListBox *hostProtocol;
	vhost *currentVHost;
	configurationFrameVHOSTS(wxWindow *parent,const wxString& title, const wxPoint& pos, const wxSize& size,long style = wxDEFAULT_FRAME_STYLE);
	void OnQuit(wxCommandEvent& event);
	void cancel(wxCommandEvent& event);
	void addVHost(wxCommandEvent& event);
	void upVHost(wxCommandEvent& event);
	void addHost(wxCommandEvent& event);
	void addIP(wxCommandEvent& event);
	void removeVHost(wxCommandEvent& event);
	void downVHost(wxCommandEvent& event);
	void removeHost(wxCommandEvent& event);
	void removeIP(wxCommandEvent& event);
	void ok(wxCommandEvent& event);
	void vhostChange(wxCommandEvent& event);
	void save(wxCommandEvent& event);
	void loadVHost(wxCommandEvent& event);
	void hostPortMod(wxCommandEvent& event);
	void hostDocMod(wxCommandEvent& event);
	void hostSysMod(wxCommandEvent& event);
	void hostAccMod(wxCommandEvent& event);
	void protocolMod(wxCommandEvent& event);
	void hostWarningsMod(wxCommandEvent& event);
private:
	DECLARE_EVENT_TABLE()
};

#endif
