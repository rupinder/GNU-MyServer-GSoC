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
#ifndef CONFIGURATION_H
#define CONFIGURATION_H
#include "stdafx.h"
#include "resource.h" 
#include <wx/wx.h> 
#include <wx/taskbar.h>
#include <wx/notebook.h>
#include <wx/combobox.h>

#ifndef WIN32
#include "../include/lfind.h"
#endif

#define SOCKETLIBINCLUDED/*Prevent include socket headers file*/
#include "../include/MIME_manager.h"
#include "../include/cXMLParser.h"
        
#include "configureMIME.h"
extern const char VERSION_OF_SOFTWARE[];

#define N_PAGES 7
class configurationFrame : public wxFrame
{
	wxTextCtrl *adminEmail;
	wxTextCtrl *timeOut;
	wxTextCtrl *bufferSize;
	wxTextCtrl *logFileSize;
	wxTextCtrl *browseFolderCSS;
	wxComboBox *languageFile;
	wxTextCtrl *nthreadsA;
	wxTextCtrl *nthreadsB;
	wxComboBox *verbosity;
	wxComboBox *useErrFiles;


	configurationFrameMIME* configureMIMEWnd;
	wxButton* btnSAVE;	
	cXMLParser confparser;
	wxPanel *pPage[N_PAGES];
	wxNotebook *m_notebook;
public:
	void configureMIME(wxCommandEvent& event);
	void configureSave(wxCommandEvent& event);
	void OnQuit(wxCommandEvent& event);
	void initNotebook();
	configurationFrame(wxWindow *parent,const wxString& title, const wxPoint& pos, const wxSize& size,long style=wxDEFAULT_FRAME_STYLE);
private:
	DECLARE_EVENT_TABLE()
};

#endif
