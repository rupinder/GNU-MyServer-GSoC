/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
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
#include "configuration.h"

#ifndef WIN32
#include "../include/lfind.h"
#endif

static int yetVisible=0;
enum
{
    Configuration_Quit = 1,
	Configuration_LOADDEF,
	Configuration_MIME,
	Configuration_VHOSTS,
	Configuration_Save,
	Configuration_Exit
};


BEGIN_EVENT_TABLE(configurationFrame, wxFrame)
EVT_BUTTON(Configuration_MIME,  configurationFrame::configureMIME)
EVT_BUTTON(Configuration_Save,  configurationFrame::configureSave)
EVT_BUTTON(Configuration_LOADDEF,  configurationFrame::loadDefault)
EVT_BUTTON(Configuration_Exit,  configurationFrame::OnQuit)
EVT_MENU(Configuration_MIME,configurationFrame::configureMIME)
EVT_BUTTON(Configuration_VHOSTS,configurationFrame::configureVHOSTS)
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

#ifndef WIN32
/* Under an *nix environment look for .xml files in the following order.
*1) myserver executable working directory
*2) ~/.myserver/
*3) /etc/myserver/
*4) default files will be copied in myserver executable working	
*/
	if(MYSERVER_FILE::fileExists("myserver.xml"))
	{
		strcpy(main_configuration_file,"myserver.xml");
	}
	else if(MYSERVER_FILE::fileExists("~/.myserver/myserver.xml"))
	{
		strcpy(main_configuration_file,"~/.myserver/myserver.xml");
	}
	else if(MYSERVER_FILE::fileExists("/etc/myserver/myserver.xml"))
	{
		strcpy(main_configuration_file,"/etc/myserver/myserver.xml");
	}
	else
#endif	
	/*!
	*If the myserver.xml files doesn't exist copy it from the default one.
	*/
	if(!MYSERVER_FILE::fileExists("myserver.xml"))
	{
			strcpy(main_configuration_file,"myserver.xml");
			MYSERVER_FILE inputF;
			MYSERVER_FILE outputF;
			int ret=inputF.openFile("myserver.xml.default", MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_IFEXISTS);
			if(ret<1)
			{
				return;
			}
			outputF.openFile("myserver.xml", MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_OPEN_ALWAYS);
			char buffer[512];
			u_long nbr, nbw;
			for(;;)
			{
				inputF.readFromFile(buffer, 512, &nbr );
				if(nbr==0)
					break;
				outputF.writeToFile(buffer, nbr, &nbw);
			}
			inputF.closeFile();
			outputF.closeFile();
	}	
	
	
	confparser.open(main_configuration_file);
	char version[50];
	sprintf(version,"MyServer Control Center %s\n",VERSION_OF_SOFTWARE);
	wxPanel *panel = new wxPanel(this, -1);
	m_notebook = new wxNotebook(panel,-1,wxPoint(10,0),wxSize(610,390));
	initNotebook();
	btnLOADDEF= new wxButton(panel,Configuration_LOADDEF,"Reset",wxPoint(120,390),wxSize(100,25));
	btnCfgVhosts= new wxButton(panel,Configuration_VHOSTS,"Configure vhosts",wxPoint(220,390),wxSize(100,25));
	btnSAVE= new wxButton(panel,Configuration_MIME,"Configure MIME",wxPoint(320,390),wxSize(100,25));
	btnCfgMime= new wxButton(panel,Configuration_Save,"Save configuration",wxPoint(420,390),wxSize(100,25));
	btnExit= new wxButton(panel,Configuration_Exit,"Exit",wxPoint(520,390),wxSize(100,25));

	yetVisible=1;
}
void configurationFrame::loadDefault(wxCommandEvent& event)
{
	bufferSize->SetValue(confparser.getValue("BUFFER_SIZE"));
	verbosity->SetValue(confparser.getValue("VERBOSITY"));
	adminEmail->SetValue(confparser.getValue("SERVER_ADMIN"));
	timeOut->SetValue(confparser.getValue("CONNECTION_TIMEOUT"));
	nthreadsA->SetValue(confparser.getValue("NTHREADS_A"));
	nthreadsB->SetValue(confparser.getValue("NTHREADS_B"));
	browseFolderCSS->SetValue(confparser.getValue("BROWSEFOLDER_CSS"));
	languageFile->SetValue(confparser.getValue("LANGUAGE"));
	useErrFiles->SetValue(confparser.getValue("USE_ERRORS_FILES"));

}
void configurationFrame::initNotebook()
{
	char *verbosityValues[]={"0","1","2","3","4","5","6","7","8","9"};
	pPage[0]= new wxPanel(m_notebook, -1);
	pPage[1]= new wxPanel(m_notebook, -1);
	/*!-------------------------FIRST PAGE-------------------------------------------*/
	bufferSize = new wxTextCtrl(pPage[0],-1,confparser.getValue("BUFFER_SIZE"),wxPoint(10,10),wxSize(75,20));
    wxStaticText *bufferSizeStat= new wxStaticText(pPage[0], -1, "Set the size of the memory buffer(in bytes)",wxPoint(85,12), wxSize(250,20));

	verbosity = new wxComboBox(pPage[0],-1,_T(confparser.getValue("VERBOSITY")),wxPoint(10,40),wxSize(75,20),0,0,wxCB_DROPDOWN|wxCB_READONLY);
	int n;
	for ( n = 0; n < 10; n++ )
	{
        verbosity->Append(_T(verbosityValues[n]));
	}
	verbosity->SetSelection(atoi(confparser.getValue("VERBOSITY")));
    wxStaticText *verbosityStat= new wxStaticText(pPage[0], -1, "Set the verbosity on the logs files",wxPoint(85,42), wxSize(250,20));

	timeOut = new wxTextCtrl(pPage[0],-1,confparser.getValue("CONNECTION_TIMEOUT"),wxPoint(10,70),wxSize(75,20));
    wxStaticText *timeOutStat= new wxStaticText(pPage[0], -1, "Set the connection time-out",wxPoint(85,72), wxSize(250,20));

	nthreadsA = new wxTextCtrl(pPage[0],-1,confparser.getValue("NTHREADS_A"),wxPoint(10,100),wxSize(75,20));
    wxStaticText *nthreadsAStat= new wxStaticText(pPage[0], -1, "Set the number of threads for every CPU",wxPoint(85,102), wxSize(250,20));

	nthreadsB = new wxTextCtrl(pPage[0],-1,confparser.getValue("NTHREADS_B"),wxPoint(10,130),wxSize(75,20));
    wxStaticText *nthreadsBStat= new wxStaticText(pPage[0], -1, "Set the number of threads always active",wxPoint(85,132), wxSize(250,20));
	
	useErrFiles = new wxComboBox(pPage[0],-1,_T(confparser.getValue("USE_ERRORS_FILES")),wxPoint(10,160),wxSize(75,20),0,0,wxCB_DROPDOWN|wxCB_READONLY);
	useErrFiles->Append(_T("YES"));
	useErrFiles->Append(_T("NO"));
	useErrFiles->SetSelection(atoi(confparser.getValue("USE_ERRORS_FILES")));
    wxStaticText *useErrFilesStat= new wxStaticText(pPage[0], -1, "Use personalized error pages",wxPoint(85,162), wxSize(250,20));

	browseFolderCSS = new wxTextCtrl(pPage[0],-1,confparser.getValue("BROWSEFOLDER_CSS"),wxPoint(10,190),wxSize(150,20));
    wxStaticText *browseFolderCSSStat= new wxStaticText(pPage[0], -1, "Define the stylesheet file for a folder browsing",wxPoint(160,192), wxSize(250,20));

	languageFile = new wxComboBox(pPage[0],-1,_T(confparser.getValue("LANGUAGE")),wxPoint(10,220),wxSize(75,20),0,0,wxCB_DROPDOWN|wxCB_READONLY);
	_finddata_t fd;
	intptr_t ff;
	

#ifdef WIN32
	strcpy(languages_path,"languages/");
	ff=_findfirst("languages/*.xml" ,&fd);
#else
	/*! If the directory /usr/share/myserver/languages exists use this.*/
	if(!MYSERVER_FILE::fileExists("languages"))
	{
		strcpy(languages_path,"languages/");
	}
	else
	{
		strcpy(languages_path,"/usr/share/myserver/languages/");
	}
	ff=_findfirst(languages_path ,&fd);
 #endif
	n=0;
	int c=0;
	do
	{
		char dir[MAX_PATH];
		char filename[MAX_PATH];
		if(fd.name[0]=='.')
			continue;
		MYSERVER_FILE::splitPath(fd.name,dir,filename);
		languageFile->Append(_T(filename));
		if(!strcmp(confparser.getValue("LANGUAGE"),filename))
			n=c;
		else 
			c++;
	}while(!_findnext(ff,&fd));
        languageFile->SetSelection(n);
    wxStaticText *languageFileStat= new wxStaticText(pPage[0], -1, "Set the language used by MyServer",wxPoint(85,222), wxSize(250,20));

	/*!-------------------------SECOND PAGE-------------------------------------------*/
	adminEmail = new wxTextCtrl(pPage[1],-1,confparser.getValue("SERVER_ADMIN"),wxPoint(10,10),wxSize(150,20));
    wxStaticText *adminEmailStat= new wxStaticText(pPage[1], -1, "Set the administrator e-mail",wxPoint(160,12), wxSize(250,20));


	/*!-------------------------ADD THE PAGES-------------------------------------------*/
	m_notebook->AddPage(pPage[0],_T("Server configuration"));
	m_notebook->AddPage(pPage[1],_T("Administrator"));
}
void configurationFrame::configureMIME(wxCommandEvent& event)
{
	configureMIMEWnd=new configurationFrameMIME(this,_T("Configure MyServer MIME types"),wxPoint(70, 70), wxSize(MIMEWNDSIZE_X, MIMEWNDSIZE_Y));
	configureMIMEWnd->Show(TRUE);
}
void configurationFrame::configureVHOSTS(wxCommandEvent& event)
{
	configureVHOSTSWnd=new configurationFrameVHOSTS(this,_T("Configure Virtual hosts"),wxPoint(70, 70), wxSize(VHOSTSWNDSIZE_X, VHOSTSWNDSIZE_Y));
	configureVHOSTSWnd->Show(TRUE);
}
void configurationFrame::configureSave(wxCommandEvent& event)
{
	confparser.setValue("SERVER_ADMIN",(char*)(adminEmail->GetValue().ToAscii()));
	confparser.setValue("CONNECTION_TIMEOUT",(char*)(timeOut->GetValue().ToAscii()));
	confparser.setValue("NTHREADS_A",(char*)(nthreadsA->GetValue().ToAscii()));
	confparser.setValue("NTHREADS_B",(char*)(nthreadsB->GetValue().ToAscii()));
	confparser.setValue("VERBOSITY",(char*)(verbosity->GetValue().ToAscii()));
	confparser.setValue("BROWSEFOLDER_CSS",(char*)(browseFolderCSS->GetValue().ToAscii()));
	confparser.setValue("BUFFER_SIZE",(char*)(bufferSize->GetValue().ToAscii()));
	confparser.setValue("LANGUAGE",(char*)(languageFile->GetValue().ToAscii()));
	confparser.setValue("USE_ERRORS_FILES",(char*)(useErrFiles->GetValue().ToAscii()));
	confparser.save(main_configuration_file);
}
void configurationFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	confparser.close();
	Show(FALSE);
	yetVisible=0;
}

