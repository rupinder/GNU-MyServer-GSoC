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
	Configuration_VHOSTLIST
};
#include "../source/vhosts.cpp"
#include "../source/utility.cpp"

BEGIN_EVENT_TABLE(configurationFrameVHOSTS, wxFrame)
EVT_BUTTON(Configuration_Ok,  configurationFrameVHOSTS::ok)
EVT_CLOSE(configurationFrameVHOSTS::OnQuit)
EVT_BUTTON(Configuration_Cancel,  configurationFrameVHOSTS::cancel)  
EVT_BUTTON(Configuration_VHOSTLIST,  configurationFrameVHOSTS::vhostChange)  
EVT_BUTTON(Configuration_Save,  configurationFrameVHOSTS::save)  
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
	hostsLB=new wxListBox(panel,Configuration_HOSTLIST,wxPoint(30,10), wxSize(160,100),0, NULL,wxLB_HSCROLL);
	ipLB=new wxListBox(panel,Configuration_IPLIST,wxPoint(190,10), wxSize(130,100),0, NULL,wxLB_HSCROLL);

	btnOK= new wxButton(panel,Configuration_Ok,"OK",wxPoint(170,200),wxSize(50,25));
	btnCNL= new wxButton(panel,Configuration_Cancel,"Cancel",wxPoint(270,200),wxSize(50,25));
	btnSAVE= new wxButton(panel,Configuration_Save,"Save",wxPoint(220,200),wxSize(50,25));
	wxStaticText *verbosityStat= new wxStaticText(panel, -1, "Set the port used",wxPoint(320,10), wxSize(50,40));
	hostPort=new wxTextCtrl(panel,-1,"",wxPoint(370,10), wxSize(40,20));
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
