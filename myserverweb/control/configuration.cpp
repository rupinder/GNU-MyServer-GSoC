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
enum
{
    Configuration_Quit = 1,
	Configuration_MIME
};


BEGIN_EVENT_TABLE(configurationFrame, wxFrame)
EVT_BUTTON(Configuration_MIME,  configurationFrame::configureMIME)
END_EVENT_TABLE()




configurationFrame::configurationFrame(const wxString& title, const wxPoint& pos, const wxSize& size, long style): wxFrame(NULL, -1, title, pos, size, style)
{
	char version[50];
	sprintf(version,"myServer Control Center %s\n",VERSION_OF_SOFTWARE);
	wxPanel *panel = new wxPanel(this, -1);
	wxButton* btn= new wxButton(panel,Configuration_MIME,"Configure MIME");

}
void configurationFrame::configureMIME(wxCommandEvent& event)
{
	configureMIMEWnd=new configurationFrameMIME(_T("Configure myServer MIME types"),wxPoint(70, 70), wxSize(320, 240));
	configureMIMEWnd->Show(TRUE);
}
void configurationFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	Show(FALSE);
}
