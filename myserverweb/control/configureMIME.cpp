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
	Configuration_Save,
	Configuration_AddExt,
	Configuration_AddMime,
	Configuration_EXTtypeListEvt,
	Configuration_Cancel
};


BEGIN_EVENT_TABLE(configurationFrameMIME, wxFrame)
EVT_BUTTON(Configuration_Ok,  configurationFrameMIME::ok)
EVT_WINDOW_DESTROY(configurationFrameMIME::OnQuit)
EVT_LISTBOX_DCLICK(Configuration_EXTtypeListEvt,configurationFrameMIME::EXTtypeListEvt)
EVT_LISTBOX(Configuration_EXTtypeListEvt,configurationFrameMIME::EXTtypeListEvt)
EVT_CLOSE(configurationFrameMIME::OnQuit)
EVT_BUTTON(Configuration_Cancel,  configurationFrameMIME::cancel)  
EVT_BUTTON(Configuration_Save,  configurationFrameMIME::save)  
EVT_BUTTON(Configuration_AddExt,  configurationFrameMIME::addExt)
EVT_BUTTON(Configuration_AddMime,  configurationFrameMIME::addMime)
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

	actiontodoLB=new wxListBox(panel,-1,wxPoint(0,110), wxSize(210,MIMEWNDSIZEY-140),0,NULL,wxLB_HSCROLL);
	cgiManagerTB=new wxTextCtrl(panel,-1,"",wxPoint(210,110), wxSize(MIMEWNDSIZEX-220,20));
	mimeTypesLB=new wxListBox(panel,-1,wxPoint(160,10), wxSize(MIMEWNDSIZEX-170,100),0, NULL,wxLB_HSCROLL);
	extensionsLB=new wxListBox(panel,Configuration_EXTtypeListEvt,wxPoint(0,10), wxSize(160,100),0, NULL,wxLB_HSCROLL);

	btnOK= new wxButton(panel,Configuration_Ok,"OK",wxPoint(220,200),wxSize(50,25));
	btnCNL= new wxButton(panel,Configuration_Cancel,"Cancel",wxPoint(270,200),wxSize(50,25));
	btnSAVE= new wxButton(panel,Configuration_Save,"Save",wxPoint(220,135),wxSize(50,25));
	btnADDEXT= new wxButton(panel,Configuration_AddExt,"Add a new extension",wxPoint(270,135),wxSize(130,25));
	btnADDMIME= new wxButton(panel,Configuration_AddExt,"Add a new MIME type",wxPoint(270,160),wxSize(130,25));

	actiontodoLB->Insert("SEND THE FILE AS IT IS",CGI_CMD_SEND);
	actiontodoLB->Insert("RUN THE CGI(SPECIFY A PATH)",CGI_CMD_RUNCGI);
	actiontodoLB->Insert("RUN THE ISAPI(SPECIFY A PATH)",CGI_CMD_RUNISAPI);
	actiontodoLB->Insert("RUN AS A MSCGI",CGI_CMD_RUNMSCGI);
	actiontodoLB->Insert("HANDLE AS AN EXECUTABLE",CGI_CMD_EXECUTE);
	actiontodoLB->Insert("HANDLE AS A LINK",CGI_CMD_SENDLINK);

	if(mm.load("MIMEtypes.txt"))
	{
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
	}

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
	mm.save(mm.getFilename());
	mm.clean();
	Destroy();
	yetVisible=0;
}
void configurationFrameMIME::EXTtypeListEvt(wxCommandEvent& event)
{
	wxString str=extensionsLB->GetString(event.GetSelection());
	char EXT[10];
	sprintf(EXT,"%s",(const char*)(str.ToAscii()));
	char CGIMANAGER[MAX_PATH];
	char MIME[60];
	int cmd=mm.getMIME(EXT,MIME,CGIMANAGER);
	mimeTypesLB->SetSelection(mimeTypesLB->GetSelection(),FALSE);
	mimeTypesLB->SetSelection(mimeTypesLB->FindString(MIME),TRUE);

	
	actiontodoLB->SetSelection(actiontodoLB->GetSelection(),FALSE);
	actiontodoLB->SetSelection(cmd,TRUE);
	cgiManagerTB->SetValue((wxString)CGIMANAGER);
}
void configurationFrameMIME::save(wxCommandEvent& event)
{
	/*
	*Save the record.
	*/
	wxString str=extensionsLB->GetString(extensionsLB->GetSelection());
	char EXT[10];
	sprintf(EXT,"%s",(const char*)(str.ToAscii()));

	MIME_Manager::mime_record *record=mm.getRecord(EXT);
	str=mimeTypesLB->GetString(mimeTypesLB->GetSelection());
	sprintf(record->mime_type,"%s",(const char*)(str.ToAscii()));

	str=cgiManagerTB->GetValue();
	sprintf(record->cgi_manager,"%s",(const char*)(str.ToAscii()));

	record->command=actiontodoLB->GetSelection();

}
void configurationFrameMIME::addExt(wxCommandEvent& event)
{
	wxString ext=wxGetTextFromUser("Insert the extension to register(If the extension is already registered all the informations will be removed)","Register a new extension","",this);
	char EXT[10];
	sprintf(EXT,"%s",(const char*)(ext.ToAscii()));
	MIME_Manager::mime_record record;
	ZeroMemory(&record,sizeof(record));
	strcpy(record.extension,EXT);
	mm.addRecord(record);
	if(extensionsLB->FindString(_T(ext))==wxNOT_FOUND)
		extensionsLB->Insert(_T(ext),0);
}
void configurationFrameMIME::addMime(wxCommandEvent& event)
{
	wxString ext=wxGetTextFromUser("Insert the MIME type to register","Register a new MIME type","",this);
	char MIME[60];
	sprintf(MIME,"%s",(const char*)(ext.ToAscii()));
	if(mimeTypesLB->FindString(_T(MIME))==wxNOT_FOUND)
		mimeTypesLB->Insert(_T(MIME),0);
}