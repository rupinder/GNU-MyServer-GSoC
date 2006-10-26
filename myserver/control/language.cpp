/*
 MyServer
 Copyright (C) 2002, 2003, 2004 The MyServer Team
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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "language.h"

extern "C"
{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
}

// Debug flag.  Set this to print (to stdout) xml tags that were not found.
// This will generate the expected xml data that was not found that can
// be copied to the english file.
//#define DEBUG

// pre vector.cpp, but it works
struct TextNode
{
   char * Val;
   TextNode * next;
};

//
// Some internal functions
//
static void LanguageXMLload();
static inline const char * textmangler(const char *);
static void AddText(char *);
static void ClearList();
static void LanguageXMLinit();
static int LanguageXMLfile(const char *);

//
// State vars and file handels
//
static TextNode * TextList = NULL;
static bool loaded = false;
static bool loadok = true;
static XmlParser xmlFile;

//
// Const strings used for dialogs and messages
// 
const char * LanguageXMLLast_Change;
const char * LanguageXMLNot_Found;
const char * LanguageXMLReload_Prev;
const char * LanguageXMLEnter_Default;
const char * LanguageXMLAbout_Text;
const char * LanguageXMLEnter_Extension;
const char * LanguageXMLEnter_MIME;
const char * LanguageXMLEnter_Name;
const char * LanguageXMLEnter_Host;
const char * LanguageXMLEnter_Ip;
const char * LanguageXMLDownload_Config;
const char * LanguageXMLDownload_Failed;
const char * LanguageXMLKill_All;
const char * LanguageXMLSend_Config;
const char * LanguageXMLUpload_Failed;
const char * LanguageXMLNot_Save;
const char * LanguageXMLReboot_Failed;
const char * LanguageXMLConnect_Server;
const char * LanguageXMLLogin_Failed;
const char * LanguageXMLCannot_Connect;
const char * LanguageXMLServer_Closed;
const char * LanguageXMLYes;
const char * LanguageXMLNo;

///
/// Add the ptr to the list for latter deletetion.
///
void AddText(char * val)
{
   TextNode * current = new TextNode;
   current->Val = val;
   current->next = TextList;
   TextList = current;
}

///
/// Delete all alcotatied memory.
///
void ClearList()
{
   TextNode * current;
   while(TextList != NULL)
     {
	current = TextList;
	TextList = TextList->next;
	free(current->Val);
	delete current;
     }
}

///
/// Initilize pointers and logic values.
///
void LanguageXMLinit()
{
   LanguageXMLLast_Change = strdup("Last change not saved.  Continue anyway?");
   LanguageXMLNot_Found = strdup("Config files not found.");
   LanguageXMLReload_Prev = strdup("Reload prevous values?");
   LanguageXMLEnter_Default = strdup("Please enter a default file name:");
   LanguageXMLAbout_Text = strdup("MyServer Configure (FLTK)\nCopyright (C) 2002, 2003, 2004\nThe MyServer Team\nThis program is licensed under the GPL.");
   LanguageXMLEnter_Extension = strdup("Enter new extension (Example: html):");
   LanguageXMLEnter_MIME = strdup("Enter new MIME type (Example: text/html):");
   LanguageXMLEnter_Name = strdup("Enter a new name:");
   LanguageXMLEnter_Host = strdup("Enter a new host name:");
   LanguageXMLEnter_Ip = strdup("Enter a new IP:");
   LanguageXMLDownload_Config = strdup("Downloading config files:");
   LanguageXMLDownload_Failed = strdup("Download failed.  Code: ");
   LanguageXMLKill_All = strdup("This will kill all connections.  Are you sure?");
   LanguageXMLSend_Config = strdup("Sending config files:");
   LanguageXMLUpload_Failed = strdup("Upload failed.  Code: ");
   LanguageXMLNot_Save = strdup("Could not save.");
   LanguageXMLReboot_Failed = strdup("Reboot failed.");
   LanguageXMLConnect_Server = strdup("Connecting to server:");
   LanguageXMLLogin_Failed = strdup("Login failed.  Code: ");
   LanguageXMLCannot_Connect = strdup("Could not connect to server.");
   LanguageXMLServer_Closed = strdup("Server closed connection.  Code: ");
   LanguageXMLYes = strdup("Yes");
   LanguageXMLNo = strdup("No");
   loaded = false;
   loadok = true;
}

///
/// The cleanup function.
/// This function should be called before exiting the program.
///
void LanguageXMLend()
{
   free((char *)LanguageXMLLast_Change);
   free((char *)LanguageXMLNot_Found);
   free((char *)LanguageXMLReload_Prev);
   free((char *)LanguageXMLEnter_Default);
   free((char *)LanguageXMLAbout_Text);
   free((char *)LanguageXMLEnter_Extension);
   free((char *)LanguageXMLEnter_MIME);
   free((char *)LanguageXMLEnter_Name);
   free((char *)LanguageXMLEnter_Host);
   free((char *)LanguageXMLEnter_Ip);
   free((char *)LanguageXMLDownload_Config);
   free((char *)LanguageXMLDownload_Failed);
   free((char *)LanguageXMLKill_All);
   free((char *)LanguageXMLSend_Config);
   free((char *)LanguageXMLUpload_Failed);
   free((char *)LanguageXMLNot_Save);
   free((char *)LanguageXMLReboot_Failed);
   free((char *)LanguageXMLConnect_Server);
   free((char *)LanguageXMLLogin_Failed);
   free((char *)LanguageXMLCannot_Connect);
   free((char *)LanguageXMLServer_Closed);
   free((char *)LanguageXMLYes);
   free((char *)LanguageXMLNo);
   ClearList();
}

///
/// Internal function to automaticaly free memory when seting to a
/// new value.
///
static int SetValueXML(char ** dest, const char * tag)
{
   char * val;
   val = xmlFile.getValue((char *)tag);
   if(val != NULL)
     {
	free(*dest);
	*dest = strdup(val);
     }
   return 0;
}

///
/// Load the xml file and set const pointers.
///
int LanguageXMLfile(const char * filename)
{
   if(xmlFile.open((char *)filename))
     return -1;

   loaded = true;

   SetValueXML((char **)&LanguageXMLLast_Change, "LAST_CHANGE");
   SetValueXML((char **)&LanguageXMLNot_Found, "NOT_FOUND");
   SetValueXML((char **)&LanguageXMLReload_Prev, "RELOAD_PREV");
   SetValueXML((char **)&LanguageXMLEnter_Default, "ENTER_DEFAULT");
   SetValueXML((char **)&LanguageXMLAbout_Text, "ABOUT_TEXT");
   SetValueXML((char **)&LanguageXMLEnter_Extension, "ENTER_EXTENSION");
   SetValueXML((char **)&LanguageXMLEnter_MIME, "ENTER_MIME");
   SetValueXML((char **)&LanguageXMLEnter_Name, "ENTER_NAME");
   SetValueXML((char **)&LanguageXMLEnter_Host, "ENTER_HOST");
   SetValueXML((char **)&LanguageXMLEnter_Ip, "ENTER_IP");
   SetValueXML((char **)&LanguageXMLDownload_Config, "DOWNLAD_CONFIG");
   SetValueXML((char **)&LanguageXMLDownload_Failed, "DOWNLAD_FAILED");
   SetValueXML((char **)&LanguageXMLKill_All, "KILL_ALL");
   SetValueXML((char **)&LanguageXMLSend_Config, "SEND_CONFIG");
   SetValueXML((char **)&LanguageXMLUpload_Failed, "UPLOAD_FAILED");
   SetValueXML((char **)&LanguageXMLNot_Save, "NOT_SAVE");
   SetValueXML((char **)&LanguageXMLReboot_Failed, "REBOOT_FAILED");
   SetValueXML((char **)&LanguageXMLConnect_Server, "CONNECT_SERVER");
   SetValueXML((char **)&LanguageXMLLogin_Failed, "LOGIN_FAILED");
   SetValueXML((char **)&LanguageXMLCannot_Connect, "CANNOT_CONNECT");
   SetValueXML((char **)&LanguageXMLServer_Closed, "SERVER_CLOSED");
   SetValueXML((char **)&LanguageXMLYes, "YES");
   SetValueXML((char **)&LanguageXMLNo, "NO");

   return 0;
}

///
/// Close the xml file.
/// This is called before LanguageEnd at the end of the program.
///
void LanguageXMLclose()
{
   if(loaded)
     {
	xmlFile.close();
	loaded = false;
     }
}

///
/// The *hack* to load the correct xml file.
/// This looks for the myserver.xml file and grabs the name of the
/// lanugage file.  Then it tries to load that file in the default
/// locations.
///
void LanguageXMLload()
{
   char languages_path[MAX_PATH];
   char language_file[MAX_PATH];
   char main_configuration_file[MAX_PATH];
   char * chrptr;
   XmlParser xmlFile;
   bool langFound = true;
   bool confFound = true;
   int ret;

   LanguageXMLinit();

   // Find the language files:
#ifdef WIN32
   strncpy(languages_path, "languages/", MAX_PATH);
#else
   if(File::fileExists("languages"))
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
   if(!(File::fileExists(languages_path)))
     {
	loadok = false;
	langFound = false;
	return;
     }
#endif

   // Search for myserver.xml
   /* Under an *nix environment look for .xml files in the following order.
    * 1) myserver executable working directory
    * 2) ~/.myserver/
    * 3) /etc/myserver/
    */
#ifndef WIN32
   // just a little hack
   snprintf(main_configuration_file, MAX_PATH, "%s/.myserver/myserver.xml", getenv("HOME"));
#endif
   if(File::fileExists("myserver.xml"))
     {
	strncpy(main_configuration_file,"myserver.xml", MAX_PATH);
     }
#ifndef WIN32
   else if(File::fileExists(main_configuration_file))
     {
	// do nothing
     }
   else if(File::fileExists("/etc/myserver/myserver.xml"))
     {
	strncpy(main_configuration_file,"/etc/myserver/myserver.xml", MAX_PATH);
     }
#endif
   else
     {
	confFound = false;
	loadok = false;
	return;
     }

   // Load the language file for configure
   if(confFound && langFound)
     {
	xmlFile.open(main_configuration_file);
	chrptr = xmlFile.getValue("LANGUAGE");
	if(chrptr != NULL)
	  snprintf(language_file, MAX_PATH, "%sconfigure/%s", languages_path, chrptr);
	xmlFile.close();
	ret = LanguageXMLfile(language_file);
	if(ret)
	  loadok = false;
     }

}

///
/// The do all function for translation.
/// This automaticaly loads the correct language file on the first call.
/// The tag is the english string that is mangled to an xml tag that is
/// used to return the translated vr using the XmlParser class.
///
extern "C" char * gettext(const char * tag)
{
   char * val;
   char * text;

   if(!loaded && loadok) // first call
     {
        LanguageXMLload();
     }
   if(!loadok) // failed first call
     {
#ifdef DEBUG
	printf("Not Loaded, TAG: %s\n", tag);
#endif
	return (char *)tag;
     }

   // Get the translation
   val = xmlFile.getValue((char *)textmangler(tag));
   if(val == NULL)
     {
#ifdef DEBUG
	printf("XML TAG <%s> Not Found!\n",textmangler(tag));
	printf("<%s>%s</%s>\n",textmangler(tag),tag,textmangler(tag));
#endif
	return (char *)tag;
     }
#ifdef DEBUG
   printf("TAG: %s VAL: %s\n", textmangler(tag), val);
#endif

   // allocate new memory and add to list
   text = strdup(val);
   AddText(text);

   return text;
}

///
/// Convert text into a xml tag.
/// This mangles the english text to make an xml tag.
/// Bufferin is the english string and returns a pointer to an internal
/// string of the xml tag.
/// This uses the frist 20 chars of bufferin with letters to upper case
/// and nonletters to underscore (_).
///
inline const char * textmangler(const char * Bufferin)
{
   char Buffer[255];
   static char Bufferout[255];
   int val, i, len;

   strncpy(Buffer, Bufferin, 255);
   Buffer[20] = '\0';
   len = strlen(Buffer);
   strncat(Buffer, "%d", 255);
   for(i = 0; i < len; i++)
     {
	if(Buffer[i] >= 'a' && Buffer[i] <= 'z')
	  Buffer[i] -= 0x20;
	else if(!(Buffer[i] >= 'A' && Buffer[i] <= 'Z'))
	  Buffer[i] = '_';
     }
   len = strlen(Bufferin);
   val = 0;
   for(i = 0; i < len; i++)
     {
	val += Bufferin[i];
	if(val > 255)
	  val -= 255;
     }
   snprintf(Bufferout, 255, Buffer, val);

   return Bufferout;
}

