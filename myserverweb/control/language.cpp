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
#include "language.h"

extern "C"
{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
}

//#define DEBUG

// pre vector.cpp, but it works
struct TextNode
{
   char * Val;
   TextNode * next;
};

static void LanguageXMLload();
static inline const char * textmangler(const char *);
static void AddText(char *);
static void ClearList();
static void LanguageXMLinit();
static int LanguageXMLfile(const char *);

static TextNode * TextList = NULL;
static bool loaded = false;
static bool loadok = true;
static cXMLParser xmlFile;

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

// Add the ptr to the list for latter deletetion
void AddText(char * val)
{
   TextNode * current = new TextNode;
   current->Val = val;
   current->next = TextList;
   TextList = current;
}

// Delete all alcotatied memory
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

// Initilize pointers and logic values
void LanguageXMLinit()
{
   (char *)LanguageXMLLast_Change = strdup("Last change not saved.  Continue anyways?");
   (char *)LanguageXMLNot_Found = strdup("Config files not found.");
   (char *)LanguageXMLReload_Prev = strdup("Reload prevous values?");
   (char *)LanguageXMLEnter_Default = strdup("Please enter a default file name:");
   (char *)LanguageXMLAbout_Text = strdup("MyServer Configure (FLTK)\nCopyright (C) 2002, 2003, 2004\nThe MyServer Team\nThis program is licensed under the GPL.");
   (char *)LanguageXMLEnter_Extension = strdup("Enter new extension (Example: html):");
   (char *)LanguageXMLEnter_MIME = strdup("Enter new MIME type (Example: text/html):");
   (char *)LanguageXMLEnter_Name = strdup("Enter a new name:");
   (char *)LanguageXMLEnter_Host = strdup("Enter a new host name:");
   (char *)LanguageXMLEnter_Ip = strdup("Enter a new IP:");
   loaded = false;
   loadok = true;
}

// The cleanup function
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
   ClearList();
}

// load the xml file and set const pointers
int LanguageXMLfile(const char * filename)
{
   char * val;

   if(xmlFile.open((char *)filename))
     return -1;

   loaded = true;

   val = xmlFile.getValue("LAST_CHANGE");
   if(val != NULL)
     {
	free((char *)LanguageXMLLast_Change);
	(char *)LanguageXMLLast_Change = strdup(val);
     }
   val = xmlFile.getValue("NOT_FOUND");
   if(val != NULL)
     {
	free((char *)LanguageXMLNot_Found);
	(char *)LanguageXMLNot_Found = strdup(val);
     }
   val = xmlFile.getValue("RELOAD_PREV");
   if(val != NULL)
     {
	free((char *)LanguageXMLReload_Prev);
	(char *)LanguageXMLReload_Prev = strdup(val);
     }
   val = xmlFile.getValue("ENTER_DEFAULT");
   if(val != NULL)
     {
	free((char *)LanguageXMLEnter_Default);
	(char *)LanguageXMLEnter_Default = strdup(val);
     }
   val = xmlFile.getValue("ABOUT_TEXT");
   if(val != NULL)
     {
	free((char *)LanguageXMLAbout_Text);
	(char *)LanguageXMLAbout_Text = strdup(val);
     }
   val = xmlFile.getValue("ENTER_EXTENSION");
   if(val != NULL)
     {
	free((char *)LanguageXMLEnter_Extension);
	(char *)LanguageXMLEnter_Extension = strdup(val);
     }
   val = xmlFile.getValue("ENTER_MIME");
   if(val != NULL)
     {
	free((char *)LanguageXMLEnter_MIME);
	(char *)LanguageXMLEnter_MIME = strdup(val);
     }
   val = xmlFile.getValue("ENTER_NAME");
   if(val != NULL)
     {
	free((char *)LanguageXMLEnter_Name);
	(char *)LanguageXMLEnter_Name = strdup(val);
     }
   val = xmlFile.getValue("ENTER_HOST");
   if(val != NULL)
     {
	free((char *)LanguageXMLEnter_Host);
	(char *)LanguageXMLEnter_Host = strdup(val);
     }
   val = xmlFile.getValue("ENTER_IP");
   if(val != NULL)
     {
	free((char *)LanguageXMLEnter_Ip);
	(char *)LanguageXMLEnter_Ip = strdup(val);
     }

   return 0;
}

// close the xml file
void LanguageXMLclose()
{
   if(loaded)
     {
	xmlFile.close();
	loaded = false;
     }
}

// The *hack* to load the correct xml file
void LanguageXMLload()
{
   char languages_path[MAX_PATH];
   char languege_file[MAX_PATH];
   char main_configuration_file[MAX_PATH];
   char * chrptr;
   cXMLParser xmlFile;
   bool langFound = true;
   bool confFound = true;
   int ret;

   LanguageXMLinit();

   // Find the language files:
#ifdef WIN32
   strncpy(languages_path, "languages/", MAX_PATH);
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
   if(MYSERVER_FILE::fileExists("myserver.xml"))
     {
	strncpy(main_configuration_file,"myserver.xml", MAX_PATH);
     }
#ifndef WIN32
   else if(MYSERVER_FILE::fileExists(main_configuration_file))
     {
	// do nothing
     }
   else if(MYSERVER_FILE::fileExists("/etc/myserver/myserver.xml"))
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
	  snprintf(languege_file, MAX_PATH, "%sconfigure/%s", languages_path, chrptr);
	xmlFile.close();
	ret = LanguageXMLfile(languege_file);
	if(ret)
	  loadok = false;
     }

}

// The do all function for translation
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

// Convert text into a xml tag
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

