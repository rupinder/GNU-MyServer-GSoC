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

#ifdef WIN32
# include <windows.h>
# include <io.h>
#endif

#include <Fl/Fl.H>
#include <Fl/fl_ask.H>

#include "language.h"
#include "../include/stringutils.h"
#include "../include/cXMLParser.h"
#include "../include/filemanager.h"
#include "fltkconfig.h"

#ifndef WIN32
#include "../include/lfind.h"
#endif

extern "C"
{
#include <stdio.h>
}

#ifndef intptr_t
#define intptr_t int
#endif

int main(int argc, char * argv[])
{
   char languages_path[MAX_PATH];
   char languege_file[MAX_PATH];
   char main_configuration_file[MAX_PATH];
   char * chrptr;
   int conf_location = 0;
   cXMLParser xmlFile;
   _finddata_t fd;
   intptr_t ff;
   bool langFound = true;
   bool confFound = true;
   int ret;

   // Find the language files:
#ifdef WIN32
   strncpy(languages_path, "languages/", MAX_PATH);
   ff=_findfirst("languages/*.xml", &fd);
#else
   if(MYSERVER_FILE::fileExists("languages"))
     {
	strncpy(languages_path, "languages/", MAX_PATH);
     }
   else
     {
#ifdef PREFIX
	snprintf(languages_path, MAX_PATH, "%s/share/myserver/languages/", PREFIX);
#else
	strncpy(languages_path, "/usr/share/myserver/languages/", MAX_PATH);
#endif
     }
   if(!(MYSERVER_FILE::fileExists(languages_path)))
     {
	fl_alert("Languages directory not found.");
	langFound = false;
     }
   if(langFound)
     ff=_findfirst(languages_path ,&fd);
#endif

   // Search for myserver.xml
   /* Under an *nix environment look for .xml files in the following order.
    * 1) myserver executable working directory
    * 2) ~/.myserver/
    * 3) /etc/myserver/
    * 4) default files will be copied in myserver executable working
    */
#ifndef WIN32
   // just a little hack
   snprintf(main_configuration_file, MAX_PATH, "%s/.myserver/myserver.xml", getenv("HOME"));
#endif
   if(MYSERVER_FILE::fileExists("myserver.xml"))
     {
	conf_location = 1;
	strncpy(main_configuration_file,"myserver.xml", MAX_PATH);
     }
#ifndef WIN32
   else if(MYSERVER_FILE::fileExists(main_configuration_file))
     {
	conf_location = 2;
     }
   else if(MYSERVER_FILE::fileExists("/etc/myserver/myserver.xml"))
     {
	conf_location = 3;
	strncpy(main_configuration_file,"/etc/myserver/myserver.xml", MAX_PATH);
     }
#endif
     /*
      * If the myserver.xml files doesn't exist copy it from the default one.
      */
   else
     {
	conf_location = 1;
	strncpy(main_configuration_file,"myserver.xml", MAX_PATH);
	MYSERVER_FILE inputF;
	MYSERVER_FILE outputF;
	if(!MYSERVER_FILE::fileExists("myserver.xml.default"))
	  {
	     fl_alert("Default configuration files not found.  Loading empty values.");
	     confFound = false;
	     conf_location = 0;
	  }
	else
	  {
	     fl_alert("Configuration files not found.  Loading default files.");
	     inputF.openFile("myserver.xml.default", MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_IFEXISTS);
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

	     if(MYSERVER_FILE::fileExists("MIMEtypes.xml.default"))
	       {
		  inputF.openFile("MIMEtypes.xml.default", MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_IFEXISTS);
		  outputF.openFile("MIMEtypes.xml", MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_OPEN_ALWAYS);
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

	     if(MYSERVER_FILE::fileExists("virtualhosts.xml.default"))
	       {
		  inputF.openFile("virtualhosts.xml.default", MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_IFEXISTS);
		  outputF.openFile("virtualhosts.xml", MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_OPEN_ALWAYS);
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
	  }
     }

   // Initilize APIs
   cXMLParser::startXML();
   // LanguageXMLinit handeled internaly
   //
   // Load the language file for configure
   // Handeled internaly
   //
   //
   // Initilize the Dialogs
   MainDlg Configure;
   Configure.make_window();
   Configure.make_about();
   Configure.make_type();
   Configure.make_login();

   // Show the main window
   Configure.ConfDlg->show(argc, argv);

   // Load the language file names
   Configure.Language->clear();
   if(langFound)
     {
	do
	  {
	     char dir[MAX_PATH];
	     char filename[MAX_PATH];
	     if(fd.name[0]=='.')
	       continue;
	     MYSERVER_FILE::splitPath(fd.name,dir,filename);
	     if(strcmpi(&(filename[strlen(filename) - 3]), "xml") == 0)
	       Configure.Language->add(filename, 0, 0, 0, 0);
	  }
	while(!_findnext(ff,&fd));
	_findclose(ff);
     }

   // Load, if found, fist avaible configuration
   Configure.ConfType = conf_location;
   switch(conf_location)
     {
      case 1 :
	Configure.ConfTypeDlgLocal->setonly();
	break;
      case 2 :
	Configure.ConfTypeDlgUser->setonly();
	break;
      case 3 :
	Configure.ConfTypeDlgGlobal->setonly();
	break;
      default :
	break;
     }
   Configure.load_config();

   // Start the app
   ret = Fl::run();

   // Close the language xml file
   LanguageXMLclose();

   // Cleanup
   LanguageXMLend();
   cXMLParser::cleanXML();

   // Exit
   return ret;
}
