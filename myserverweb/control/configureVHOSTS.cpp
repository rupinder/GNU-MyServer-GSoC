/*
*MyServer
*Copyright (C) 2002 The MyServer Team
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
	Configuration_UpVHost,
	Configuration_DownVHost,
	Configuration_AddHost,
	Configuration_RemoveHost,
	Configuration_AddIP,
	Configuration_RemoveIP,
	Configuration_hostPortMod,
	Configuration_hostDocMod,
	Configuration_hostSysMod,
	Configuration_hostAccMod,
	Configuration_hostWarningsMod,
	Configuration_protocolMod
};
#define DO_NOT_USE_SSL

#include "../source/lfind.cpp"
#include "../source/vhosts.cpp"
#include "../source/utility.cpp"
#include "../source/threads.cpp"
#include "../source/processes.cpp"

BEGIN_EVENT_TABLE(configurationFrameVHOSTS, wxFrame)
EVT_BUTTON(Configuration_Ok,  configurationFrameVHOSTS::ok)
EVT_CLOSE(configurationFrameVHOSTS::OnQuit)
EVT_BUTTON(Configuration_UpVHost,  configurationFrameVHOSTS::upVHost)  
EVT_BUTTON(Configuration_DownVHost,  configurationFrameVHOSTS::downVHost)  
EVT_BUTTON(Configuration_Cancel,  configurationFrameVHOSTS::cancel)  
EVT_BUTTON(Configuration_VHOSTLIST,  configurationFrameVHOSTS::vhostChange)  
EVT_BUTTON(Configuration_Save,  configurationFrameVHOSTS::save)  
EVT_BUTTON(Configuration_AddHost,  configurationFrameVHOSTS::addHost)  
EVT_BUTTON(Configuration_RemoveHost,  configurationFrameVHOSTS::removeHost)  
EVT_BUTTON(Configuration_AddIP,  configurationFrameVHOSTS::addIP)  
EVT_TEXT(Configuration_hostPortMod,configurationFrameVHOSTS::hostPortMod)
EVT_TEXT(Configuration_hostDocMod,configurationFrameVHOSTS::hostDocMod)
EVT_TEXT(Configuration_hostSysMod,configurationFrameVHOSTS::hostSysMod)
EVT_TEXT(Configuration_hostAccMod,configurationFrameVHOSTS::hostAccMod)
EVT_TEXT(Configuration_hostWarningsMod,configurationFrameVHOSTS::hostWarningsMod)
EVT_LISTBOX(Configuration_protocolMod,  configurationFrameVHOSTS::protocolMod)  
EVT_BUTTON(Configuration_RemoveIP,  configurationFrameVHOSTS::removeIP)  
EVT_BUTTON(Configuration_AddVHost,  configurationFrameVHOSTS::addVHost)  
EVT_LISTBOX(Configuration_VHOSTLIST,  configurationFrameVHOSTS::loadVHost)  
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

	vhostsLB=new wxListBox(panel,Configuration_VHOSTLIST,wxPoint(0,10), wxSize(100,100),0, NULL,wxLB_HSCROLL);
	btnADDVHOST= new wxButton(panel,Configuration_AddVHost,"+",wxPoint(0,110),wxSize(25,15));
	btnREMOVEVHOST= new wxButton(panel,Configuration_RemoveVHost,"-",wxPoint(25,110),wxSize(25,15));
	btnUPVHOST= new wxButton(panel,Configuration_UpVHost,"<",wxPoint(50,110),wxSize(25,15));
	btnDOWNVHOST= new wxButton(panel,Configuration_DownVHost,">",wxPoint(75,110),wxSize(25,15));

	
	hostsLB=new wxListBox(panel,Configuration_HOSTLIST,wxPoint(100,10), wxSize(110,100),0, NULL,wxLB_HSCROLL);
	btnADDHOST= new wxButton(panel,Configuration_AddHost,"+host",wxPoint(100,110),wxSize(55,25));
	btnREMOVEHOST= new wxButton(panel,Configuration_RemoveHost,"-host",wxPoint(155,110),wxSize(55,25));

	ipLB=new wxListBox(panel,Configuration_IPLIST,wxPoint(210,10), wxSize(110,100),0, NULL,wxLB_HSCROLL);
	btnADDIP= new wxButton(panel,Configuration_AddIP,"+IP",wxPoint(210,110),wxSize(55,25));
	btnREMOVEIP= new wxButton(panel,Configuration_RemoveIP,"-IP",wxPoint(265,110),wxSize(55,25));

	btnOK= new wxButton(panel,Configuration_Ok,"OK",wxPoint(170,200),wxSize(50,25));
	btnCNL= new wxButton(panel,Configuration_Cancel,"Cancel",wxPoint(270,200),wxSize(50,25));
	btnSAVE= new wxButton(panel,Configuration_Save,"Save",wxPoint(220,200),wxSize(50,25));

	wxStaticText *hostPortS= new wxStaticText(panel, -1, "Listening port",wxPoint(320,10), wxSize(100,40));
	hostPort=new wxTextCtrl(panel,Configuration_hostPortMod,"",wxPoint(400,10), wxSize(40,20));

	wxStaticText *hostDocS= new wxStaticText(panel, -1, "Document root",wxPoint(320,30), wxSize(100,40));
	hostDoc=new wxTextCtrl(panel,Configuration_hostDocMod,"",wxPoint(400,30), wxSize(230,20));

	wxStaticText *hostSysS= new wxStaticText(panel, -1, "System folder",wxPoint(320,50), wxSize(100,40));
	hostSys=new wxTextCtrl(panel,Configuration_hostSysMod,"",wxPoint(400,50), wxSize(230,20));

	wxStaticText *hostAccS= new wxStaticText(panel, -1, "Accesses log",wxPoint(320,70), wxSize(100,40));
	hostAcc=new wxTextCtrl(panel,Configuration_hostAccMod,"",wxPoint(400,70), wxSize(230,20));

	wxStaticText *hostWarningsS= new wxStaticText(panel, -1, "Warnings log",wxPoint(320,90), wxSize(100,40));
	hostWarnings=new wxTextCtrl(panel,Configuration_hostWarningsMod,"",wxPoint(400,90), wxSize(230,20));

	wxStaticText *hostProtocolS= new wxStaticText(panel, -1, "Protocol used",wxPoint(320,110), wxSize(100,40));
	hostProtocol=new wxComboBox(panel,Configuration_protocolMod,"HTTP",wxPoint(400,110), wxSize(230,20),0, NULL,wxCB_DROPDOWN|wxCB_READONLY);
	hostProtocol->Append(_T("HTTP"));
	hostProtocol->Append(_T("HTTPS"));

	vhostmanager::sVhostList *sl = hostmanager.getvHostList();
	while(sl)
	{
		vhostsLB->Append(_T(sl->host->name));
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
void configurationFrameVHOSTS::ok(wxCommandEvent& event)
{
	save(event);
	cancel(event);
}

void configurationFrameVHOSTS::save(wxCommandEvent& event)
{
	hostmanager.saveXMLConfigurationFile("virtualhosts.xml");

}
void configurationFrameVHOSTS::addHost(wxCommandEvent& event)
{
	wxString host=wxGetTextFromUser("Insert the name of the host","Add a new host","",this);
	if(currentVHost)
		currentVHost->addHost((char*)(host.ToAscii()));
	hostsLB->Append(host);
}
void configurationFrameVHOSTS::addIP(wxCommandEvent& event)
{
	wxString ip=wxGetTextFromUser("Insert the IP address","Add a new IP address","",this);
	if(currentVHost)
		currentVHost->addIP((char*)(ip.ToAscii()));
	ipLB->Append(ip);
}
void configurationFrameVHOSTS::removeHost(wxCommandEvent& event)
{
	wxString host=hostsLB->GetString(hostsLB->GetSelection());
	hostsLB->Delete(hostsLB->GetSelection());
	if(currentVHost)
		currentVHost->removeHost((char*)(host.ToAscii()));
	
}
void configurationFrameVHOSTS::removeIP(wxCommandEvent& event)
{
	wxString ip=ipLB->GetString(ipLB->GetSelection());
	ipLB->Delete(ipLB->GetSelection());
	if(currentVHost)
		currentVHost->removeIP((char*)(ip.ToAscii()));
}
void configurationFrameVHOSTS::addVHost(wxCommandEvent& event)
{
	wxString vhostS=wxGetTextFromUser("Insert a description for your new virtual host","Add a new virtual host","",this);
	vhost* vh=new vhost();
	strcpy(vh->name,(char*)(vhostS.ToAscii()));
	hostmanager.addvHost(vh);
	vhostsLB->Append(vhostS);
}
void configurationFrameVHOSTS::removeVHost(wxCommandEvent& event)
{
	int sel=vhostsLB->GetSelection();
	hostmanager.removeVHost(sel);
	vhostsLB->Delete(sel);
	if(sel)
	{
		vhostsLB->SetSelection(sel-1);
		loadVHost(event);
	}

}
void configurationFrameVHOSTS::upVHost(wxCommandEvent& event)
{
	int sel=vhostsLB->GetSelection();
	if(sel)
	{
		wxString buffer=vhostsLB->GetString(sel);
		vhostsLB->SetString(sel,vhostsLB->GetString(sel-1));
		vhostsLB->SetString(sel-1,buffer);
		hostmanager.switchVhosts(sel,sel-1);
		vhostsLB->SetSelection(sel-1);
	}
}
void configurationFrameVHOSTS::downVHost(wxCommandEvent& event)
{
	int sel=vhostsLB->GetSelection();
	if(sel+1<hostmanager.getHostsNumber())
	{
		wxString buffer=vhostsLB->GetString(sel);
		vhostsLB->SetString(sel,vhostsLB->GetString(sel+1));
		vhostsLB->SetString(sel+1,buffer);
		hostmanager.switchVhosts(sel,sel+1);
		vhostsLB->SetSelection(sel+1);
	}
}
void configurationFrameVHOSTS::loadVHost(wxCommandEvent& event)
{
	int sel=vhostsLB->GetSelection();
	hostsLB->Clear();
	ipLB->Clear();
	currentVHost=hostmanager.getVHostByNumber(sel);
	char port[6];
	sprintf(port,"%i",currentVHost->port);
	hostPort->SetValue((wxString)port);
	hostDoc->SetValue((wxString)currentVHost->documentRoot);
	hostSys->SetValue((wxString)currentVHost->systemRoot);
	hostAcc->SetValue((wxString)currentVHost->accessesLogFileName);
	hostWarnings->SetValue((wxString)currentVHost->warningsLogFileName);
	hostProtocol->SetSelection(currentVHost->protocol);
	vhost::sHostList *hl = currentVHost->hostList;
	int i=0;
	while(hl)
	{
		hostsLB->Insert(hl->hostName,i++);
		hl=hl->next;
	}
	vhost::sIpList *il = currentVHost->ipList;
	i=0;
	while(il)
	{
		ipLB->Insert(il->hostIp,i++);
		il=il->next;
	}
}
void configurationFrameVHOSTS::hostPortMod(wxCommandEvent& event)
{
	wxString str = hostPort->GetValue();
	if(currentVHost)
		currentVHost->port = atoi((char*)str.ToAscii());
}
void configurationFrameVHOSTS::hostDocMod(wxCommandEvent& event)
{
	wxString str = hostDoc->GetValue();
	if(currentVHost)
		sprintf(currentVHost->documentRoot,"%s",(char*)str.ToAscii());
}
void configurationFrameVHOSTS::hostSysMod(wxCommandEvent& event)
{
	wxString str = hostSys->GetValue();
	if(currentVHost)
		sprintf(currentVHost->systemRoot,"%s",(char*)str.ToAscii());
}
void configurationFrameVHOSTS::hostAccMod(wxCommandEvent& event)
{
	wxString str = hostAcc->GetValue();
	if(currentVHost)
		strcpy(currentVHost->accessesLogFileName,(char*)str.ToAscii());
}
void configurationFrameVHOSTS::protocolMod(wxCommandEvent& event)
{
	if(currentVHost)
		currentVHost->protocol=hostProtocol->GetSelection();
}
void configurationFrameVHOSTS::hostWarningsMod(wxCommandEvent& event)
{
	wxString str = hostWarnings->GetValue();
	if(currentVHost)
		strcpy(currentVHost->warningsLogFileName,(char*)str.ToAscii());
}
