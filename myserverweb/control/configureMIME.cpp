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
#include "configuration.h"
#include "configureMIME.h"
static int yetVisible=0;
enum
{
    Configuration_Quit = 1,
	Configuration_Ok,
	Configuration_EXTtypeListEvt,
	Configuration_Cancel
};


BEGIN_EVENT_TABLE(configurationFrameMIME, wxFrame)
EVT_BUTTON(Configuration_Ok,  configurationFrameMIME::ok)
EVT_WINDOW_DESTROY(configurationFrameMIME::OnQuit)
EVT_LISTBOX_DCLICK(Configuration_EXTtypeListEvt,configurationFrameMIME::EXTtypeListEvt)
EVT_CLOSE(configurationFrameMIME::OnQuit)
EVT_BUTTON(Configuration_Cancel,  configurationFrameMIME::cancel)  
END_EVENT_TABLE()




configurationFrameMIME::configurationFrameMIME(wxWindow *parent,const wxString& title, const wxPoint& pos, const wxSize& size, long style): wxFrame(parent, -1, title, pos, size, style)
{
	if(yetVisible)
	{
		Destroy();
		return;
	}
	char version[50];
	sprintf(version,"myServer Control Center %s\n",VERSION_OF_SOFTWARE);
	wxPanel *panel = new wxPanel(this, -1);
	actiontodoLB=new wxListBox(panel,-1,wxPoint(0,100), wxSize(120,100),0,NULL,wxLB_HSCROLL);
	extensionsLB=new wxListBox(panel,Configuration_EXTtypeListEvt,wxPoint(0,0), wxSize(120,100),0, NULL,wxLB_HSCROLL);
    mimeTypesLB=new wxListBox(panel,-1,wxPoint(120,0), wxSize(190,100),0, NULL,wxLB_HSCROLL);


	actiontodoLB->Insert("SEND",CGI_CMD_SEND);
	actiontodoLB->Insert("RUNCGI",CGI_CMD_RUNCGI);
	actiontodoLB->Insert("RUNISAPI",CGI_CMD_RUNISAPI);
	actiontodoLB->Insert("RUNMSCGI",CGI_CMD_RUNMSCGI);
	actiontodoLB->Insert("EXECUTE",CGI_CMD_EXECUTE);
	actiontodoLB->Insert("SENDLINK",CGI_CMD_SENDLINK);

	mm.load("MIMEtypes.txt");
	u_long nelements=mm.getNumMIMELoaded();
	for(int i=0;i<nelements;i++)
	{
		char ext[10];/*File extension*/
		char dest[60];/*MIME type*/
		char dest2[MAX_PATH];/*CGI manager if any*/
		int cmd=mm.getMIME(i,ext,dest,dest2);/*Action to do with this file type*/
		if(mimeTypesLB->FindString(_T(dest))==wxNOT_FOUND)
			mimeTypesLB->Insert(_T(dest),0);
		extensionsLB->Insert(_T(ext),0);
	}

	wxButton* btnOK= new wxButton(panel,Configuration_Ok,"OK",wxPoint(160,160),wxSize(50,25));
	wxButton* btnCNL= new wxButton(panel,Configuration_Cancel,"Cancel",wxPoint(210,160),wxSize(50,25));
	yetVisible=1;
}
void configurationFrameMIME::cancel(wxCommandEvent& WXUNUSED(event))
{
	mm.clean();
	yetVisible=0;
	Destroy();
}
void configurationFrameMIME::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	mm.clean();
	Show(FALSE);
	yetVisible=0;
}
void configurationFrameMIME::ok(wxCommandEvent& WXUNUSED(event))
{
	/*
	*Save the configuration....
	*/
	mm.clean();
	Destroy();
	yetVisible=0;
}
void configurationFrameMIME::EXTtypeListEvt(wxCommandEvent& event)
{
	int sel=event.GetSelection();
	wxString str=extensionsLB->GetString(sel);
/*	MessageBox(0,str.mb_str(),"",0);*/
	char EXT[10];
	wxCharBuffer cb=str.ToAscii();
	sprintf(EXT,"%s",(const char*)cb);
	char MIME[60];
	int cmd=mm.getMIME(EXT,MIME,NULL);
	mimeTypesLB->SetSelection(mimeTypesLB->GetSelection(),FALSE);
	mimeTypesLB->SetSelection(mimeTypesLB->FindString(MIME),TRUE);

	
	actiontodoLB->SetSelection(actiontodoLB->GetSelection(),FALSE);
	actiontodoLB->SetSelection(cmd,TRUE);
}
