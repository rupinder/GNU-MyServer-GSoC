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
enum
{
    Configuration_Quit = 1,
	Configuration_Ok,
	Configuration_Cancel
};


BEGIN_EVENT_TABLE(configurationFrameMIME, wxFrame)
EVT_BUTTON(Configuration_Ok,  configurationFrameMIME::ok)
EVT_BUTTON(Configuration_Cancel,  configurationFrameMIME::cancel)  
END_EVENT_TABLE()




configurationFrameMIME::configurationFrameMIME(const wxString& title, const wxPoint& pos, const wxSize& size, long style)
       : wxFrame(NULL, -1, title, pos, size, style)
{
	char version[50];
	sprintf(version,"myServer Control Center %s\n",VERSION_OF_SOFTWARE);
	wxPanel *panel = new wxPanel(this, -1);
    wxListBox *m_lbox=new wxListBox(panel,3,wxPoint(160,0), wxSize(180,100),0, NULL,wxLB_HSCROLL);
    wxArrayString items;
	MIME_Manager mm;
	mm.load("MIMEtypes.txt");
	u_long nelements=mm.getNumMIMELoaded();
	for(int i=0;i<nelements;i++)
	{
		char ext[10];/*File extension*/
		char dest[60];/*MIME type*/
		char dest2[MAX_PATH];/*CGI manager if any*/
		int cmd=mm.getMIME(i,ext,dest,dest2);/*Action to do with this file type*/
		items.Add(_T(ext),1);
	}
	mm.clean();
	m_lbox->InsertItems(items,0);
	wxButton* btnOK= new wxButton(panel,Configuration_Ok,"OK",wxPoint(160,160),wxSize(50,25));
	wxButton* btnCNL= new wxButton(panel,Configuration_Cancel,"Cancel",wxPoint(210,160),wxSize(50,25));
}
void configurationFrameMIME::cancel(wxCommandEvent& WXUNUSED(event))
{
	Destroy();
}
void configurationFrameMIME::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	Destroy();
}
void configurationFrameMIME::ok(wxCommandEvent& WXUNUSED(event))
{
	/*
	*Save the configuration....
	*/
	Destroy();
}
