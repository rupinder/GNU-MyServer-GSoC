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
#include "../include/vector.h"
#include "fltkconfig.h"

#ifndef WIN32
# include "../include/lfind.h"
#endif

extern "C"
{
#include <stdio.h>
#ifdef WIN32
# include <direct.h>
#elif HAVE_DL
# include <dlfcn.h>
# define HMODULE void *
#else
# define HMODULE void *
#endif
}

#ifndef intptr_t
# define intptr_t int
#endif

typedef char* (*registerNamePROC)(char*,int);

static void GetDynamicProtocols(const char *, Vector &);

int main(int argc, char * argv[])
{
   char languages_path[MAX_PATH];
   char main_configuration_file[MAX_PATH];
   int conf_location = 0;
   cXMLParser xmlFile;
   _finddata_t fd;
   long ff;
   bool langFound = true;
   bool confFound = true;
   int ret;

   /*! Initialize the SSL library. */
#ifndef DO_NOT_USE_SSL
   SSL_library_init();
   SSL_load_error_strings();
#endif

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
# ifdef PREFIX
	snprintf(languages_path, MAX_PATH, "%s/share/myserver/languages/", PREFIX);
# else
	strncpy(languages_path, "/usr/share/myserver/languages/", MAX_PATH);
# endif
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
	     char buffer[512];
	     u_long nbr, nbw;
	     fl_alert("Configuration files not found.  Loading default files.");
	     inputF.openFile("myserver.xml.default", MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_IFEXISTS);
	     outputF.openFile("myserver.xml", MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_OPEN_ALWAYS);
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
		  char buffer[512];
		  u_long nbr, nbw;
		  inputF.openFile("MIMEtypes.xml.default", MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_IFEXISTS);
		  outputF.openFile("MIMEtypes.xml", MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_OPEN_ALWAYS);
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
		  char buffer[512];
		  u_long nbr, nbw;
		  inputF.openFile("virtualhosts.xml.default", MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_IFEXISTS);
		  outputF.openFile("virtualhosts.xml", MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_OPEN_ALWAYS);
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
   Configure.make_status();
   Configure.make_connections();

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
   
   // Load the dynamic protocol names
   Vector list;
   if(MYSERVER_FILE::fileExists("external/protocols"))
     {
	GetDynamicProtocols("external/protocols", list);
     }
#ifndef WIN32
#ifdef PREFIX
   else if(MYSERVER_FILE::fileExists(PREFIX "/lib/myserver/external/protocols"))
     {
	GetDynamicProtocols(PREFIX "/lib/myserver/external/protocols", list);
     }
#else
   else if(MYSERVER_FILE::fileExists("/usr/lib/myserver/external/protocols"))
     {
	GetDynamicProtocols("/usr/lib/myserver/external/protocols", list);
     }
#endif
#endif
   Configure.setDynamic(list);
   list.clear();

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

// get the local dynamic protocols
// parts taken from protocols_manager.cpp 
static void GetDynamicProtocols(const char * folder, Vector & list)
{
   list.clear();

   HMODULE module;
   registerNamePROC name;
   
   int filenamelen = 0;
   char *filename = 0;
#ifdef WIN32
   filenamelen=strlen(folder)+6;
   filename=new char[filenamelen];
   if(filename == 0)
     return;
   sprintf(filename,"%s/*.*",folder);
#endif
#ifdef NOT_WIN
   filenamelen=strlen(folder)+2;
   filename=new char[filenamelen];
   if(filename == 0)
     return;
   strncpy(filename,folder, filenamelen);
#endif
   _finddata_t fd;
   long ff;
   ff=_findfirst(filename,&fd);
#ifdef WIN32
   if(ff==-1)
#endif
#ifdef NOT_WIN
     if((int)ff==-1)
#endif
       {
	  delete [] filename;
	  filename = 0;
	  return;
       }
   char *completeFileName = 0;
   int completeFileNameLen = 0;
   do
     {
	if(fd.name[0]=='.')
	  continue;
	/*
	 *Do not consider file other than dynamic libraries.
	 */
#ifdef WIN32
	if(!strstr(fd.name,".dll"))
#endif
#ifdef NOT_WIN
	  if(!strstr(fd.name,".so"))
#endif
	    continue;
	completeFileNameLen = strlen(folder) + strlen(fd.name) + 2;
	completeFileName = new char[completeFileNameLen];
	if(completeFileName == 0)
	  {
	     delete [] filename;
	     filename = 0;
	     return;
	  }
	sprintf(completeFileName,"%s/%s",folder,fd.name);
#ifdef WIN32
	module = LoadLibrary(completeFileName);
#endif
#ifdef HAVE_DL
	module = dlopen(completeFileName, RTLD_LAZY);
#endif
	if(module != NULL)
	  {
#ifdef WIN32
	     name = (registerNamePROC)GetProcAddress((HMODULE)module, "registerName");
#endif
#ifdef HAVE_DL
	     name = (registerNamePROC)dlsym(module, "registerName");
#endif
	     if(name != NULL)
	       {
		  list.add(name(NULL, 0));
	       }
	  }
#ifdef WIN32
	FreeLibrary((HMODULE)module);
#endif
#ifdef HAVE_DL
	dlclose(module);
#endif
	delete [] completeFileName;
     }
   while(!_findnext(ff,&fd));
   _findclose(ff);
   delete [] filename;
   filename = 0;
}
