/*
*myServer
*Copyright (C) 2002 Giuseppe Scrivano
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
#include "control.h"
#include "configuration.h"
#include "configureVHOSTS.h"
static int yetVisible=0;
enum
{
    Configuration_Quit = 1,
	Configuration_Ok,
	Configuration_Save,
	Configuration_Cancel,
	Configuration_HOSTLIST,
	Configuration_IPLIST,
	Configuration_VHOSTLIST,
	Configuration_AddVHost,
	Configuration_RemoveVHost,
	Configuration_AddHost,
	Configuration_RemoveHost,
	Configuration_AddIP,
	Configuration_RemoveIP
};
#include "../source/vhosts.cpp"
#include "../source/utility.cpp"

BEGIN_EVENT_TABLE(configurationFrameVHOSTS, wxFrame)
EVT_BUTTON(Configuration_Ok,  configurationFrameVHOSTS::ok)
EVT_CLOSE(configurationFrameVHOSTS::OnQuit)
EVT_BUTTON(Configuration_Cancel,  configurationFrameVHOSTS::cancel)  
EVT_BUTTON(Configuration_VHOSTLIST,  configurationFrameVHOSTS::vhostChange)  
EVT_BUTTON(Configuration_Save,  configurationFrameVHOSTS::save)  
EVT_BUTTON(Configuration_AddHost,  configurationFrameVHOSTS::addHost)  
EVT_BUTTON(Configuration_RemoveHost,  configurationFrameVHOSTS::removeHost)  
EVT_BUTTON(Configuration_AddIP,  configurationFrameVHOSTS::addIP)  
EVT_BUTTON(Configuration_RemoveIP,  configurationFrameVHOSTS::removeIP)  
EVT_BUTTON(Configuration_AddVHost,  configurationFrameVHOSTS::addVHost)  
EVT_BUTTON(Configuration_RemoveVHost,  configurationFrameVHOSTS::removeVHost)  
END_EVENT_TABLE()

configurationFrameVHOSTS::configurationFrameVHOSTS(wxWindow *parent,const wxString& title, const wxPoint& pos, const wxSize& size, long style): wxFrame(parent, -1, title, pos, size, style)
{
	if(yetVisible)
	{
		Destroy();
		return;
	}
	setcwdBuffer();
	hostmanager.loadXMLConfigurationFile("virtualhosts.xml");
	wxPanel *panel = new wxPanel(this, -1);
	vhostsLB=new wxListBox(panel,Configuration_VHOSTLIST,wxPoint(0,10), wxSize(30,100),0, NULL,wxLB_HSCROLL);
	btnADDVHOST= new wxButton(panel,Configuration_AddVHost,"+",wxPoint(0,110),wxSize(15,15));
	btnREMOVEVHOST= new wxButton(panel,Configuration_RemoveVHost,"-",wxPoint(15,110),wxSize(15,15));

	hostsLB=new wxListBox(panel,Configuration_HOSTLIST,wxPoint(30,10), wxSize(160,100),0, NULL,wxLB_HSCROLL);
	ipLB=new wxListBox(panel,Configuration_IPLIST,wxPoint(190,10), wxSize(130,100),0, NULL,wxLB_HSCROLL);
	
	btnADDHOST= new wxButton(panel,Configuration_AddHost,"Add host",wxPoint(30,110),wxSize(80,25));
	btnREMOVEHOST= new wxButton(panel,Configuration_RemoveHost,"Remove host",wxPoint(110,110),wxSize(80,25));

	btnADDIP= new wxButton(panel,Configuration_AddIP,"Add IP",wxPoint(190,110),wxSize(65,25));
	btnREMOVEIP= new wxButton(panel,Configuration_RemoveIP,"Remove IP",wxPoint(255,110),wxSize(65,25));

	btnOK= new wxButton(panel,Configuration_Ok,"OK",wxPoint(170,200),wxSize(50,25));
	btnCNL= new wxButton(panel,Configuration_Cancel,"Cancel",wxPoint(270,200),wxSize(50,25));
	btnSAVE= new wxButton(panel,Configuration_Save,"Save",wxPoint(220,200),wxSize(50,25));

	wxStaticText *hostPortS= new wxStaticText(panel, -1, "Set the port used",wxPoint(320,10), wxSize(100,40));
	hostPort=new wxTextCtrl(panel,-1,"",wxPoint(420,10), wxSize(40,20));

	wxStaticText *hostDocS= new wxStaticText(panel, -1, "Document root",wxPoint(320,30), wxSize(100,40));
	hostDoc=new wxTextCtrl(panel,-1,"",wxPoint(420,30), wxSize(210,20));

	wxStaticText *hostSysS= new wxStaticText(panel, -1, "System folder",wxPoint(320,50), wxSize(100,40));
	hostSys=new wxTextCtrl(panel,-1,"",wxPoint(420,50), wxSize(210,20));

	wxStaticText *hostAccS= new wxStaticText(panel, -1, "Accesses log file",wxPoint(320,70), wxSize(100,40));
	hostAcc=new wxTextCtrl(panel,-1,"",wxPoint(420,70), wxSize(210,20));

	wxStaticText *hostWarningsS= new wxStaticText(panel, -1, "Warnings log file",wxPoint(320,90), wxSize(100,40));
	hostWarnings=new wxTextCtrl(panel,-1,"",wxPoint(420,90), wxSize(210,20));

	wxStaticText *hostProtocolS= new wxStaticText(panel, -1, "Protocol used",wxPoint(320,110), wxSize(100,40));
	hostProtocol=new wxTextCtrl(panel,-1,"",wxPoint(420,110), wxSize(210,20));

	vhostmanager::sVhostList *sl = hostmanager.getvHostList();
	int i=0;
	while(sl)
	{
		char buff[6];
		sprintf(buff,"%i",i);
		vhostsLB->Insert(_T(buff),i++);
		sl=sl->next;
	}

	yetVisible=1;
}
void configurationFrameVHOSTS::vhostChange(wxCommandEvent& event)
{
	wxString str=vhostsLB->GetString(vhostsLB->GetSelection());
	int sel=atoi((const char*)(str.ToAscii()));

}
void configurationFrameVHOSTS::cancel(wxCommandEvent& WXUNUSED(event))
{
	yetVisible=0;
	Destroy();
}
void configurationFrameVHOSTS::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	hostmanager.clean();
	Show(FALSE);
	yetVisible=0;
}
void configurationFrameVHOSTS::ok(wxCommandEvent& WXUNUSED(event))
{
	Destroy();
	yetVisible=0;
}

void configurationFrameVHOSTS::save(wxCommandEvent& event)
{


}
void configurationFrameVHOSTS::addHost(wxCommandEvent& event)
{

}
void configurationFrameVHOSTS::addIP(wxCommandEvent& event)
{

}
void configurationFrameVHOSTS::removeHost(wxCommandEvent& event)
{

}
void configurationFrameVHOSTS::removeIP(wxCommandEvent& event)
{

}
void configurationFrameVHOSTS::addVHost(wxCommandEvent& event)
{

}
void configurationFrameVHOSTS::removeVHost(wxCommandEvent& event)
{

}