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
static int yetVisible=0;
enum
{
    Configuration_Quit = 1,
	Configuration_MIME,
	Configuration_Exit
};


BEGIN_EVENT_TABLE(configurationFrame, wxFrame)
EVT_BUTTON(Configuration_MIME,  configurationFrame::configureMIME)
EVT_MENU(Configuration_MIME,configurationFrame::configureMIME)
EVT_MENU(Configuration_Exit,configurationFrame::OnQuit)
EVT_WINDOW_DESTROY(configurationFrame::OnQuit)
EVT_CLOSE(configurationFrame::OnQuit)
END_EVENT_TABLE()




configurationFrame::configurationFrame(wxWindow *parent,const wxString& title, const wxPoint& pos, const wxSize& size, long style): wxFrame(parent, -1, title, pos, size, style)
{
	if(yetVisible)
	{
		Destroy();
		return;
	}
	char version[50];
	sprintf(version,"myServer Control Center %s\n",VERSION_OF_SOFTWARE);
/*	wxPanel *panel = new wxPanel(this, -1);
	wxButton* btn= new wxButton(panel,Configuration_MIME,"Configure MIME",wxPoint(10,10));*/
	wxMenu* mainMenu = new wxMenu;
    mainMenu->Append(Configuration_MIME, _T("&Configure MIME"), _T("Configure MIME types"));
    mainMenu->Append(Configuration_Exit, _T("&Exit"), _T("Exit from the current configuration window"));

	wxMenuBar *menuBar = new wxMenuBar();
    menuBar->Append(mainMenu, _T("&myServer configuration"));
    SetMenuBar(menuBar);
	yetVisible=1;
}
void configurationFrame::configureMIME(wxCommandEvent& event)
{
	configureMIMEWnd=new configurationFrameMIME(this,_T("Configure myServer MIME types"),wxPoint(70, 70), wxSize(MIMEWNDSIZEX, MIMEWNDSIZEY));
	configureMIMEWnd->Show(TRUE);
}
void configurationFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	Show(FALSE);
	yetVisible=0;
}
