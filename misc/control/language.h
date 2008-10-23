/*
 MyServer
 Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
  */
#ifndef LANGUAGE_H
#define LANGUAGE_H

#include <include/base/xml/xml_parser.h>

void LanguageXMLinit();
void LanguageXMLend();
//int LanguageXMLfile(const char *);  // handeled internaly
void LanguageXMLclose();

extern const char * LanguageXMLLast_Change;
extern const char * LanguageXMLNot_Found;
extern const char * LanguageXMLReload_Prev;
extern const char * LanguageXMLEnter_Default;
extern const char * LanguageXMLAbout_Text;
extern const char * LanguageXMLEnter_Extension;
extern const char * LanguageXMLEnter_MIME;
extern const char * LanguageXMLEnter_Name;
extern const char * LanguageXMLEnter_Host;
extern const char * LanguageXMLEnter_Ip;
extern const char * LanguageXMLDownload_Config;
extern const char * LanguageXMLDownload_Failed;
extern const char * LanguageXMLKill_All;
extern const char * LanguageXMLSend_Config;
extern const char * LanguageXMLUpload_Failed;
extern const char * LanguageXMLNot_Save;
extern const char * LanguageXMLReboot_Failed;
extern const char * LanguageXMLConnect_Server;
extern const char * LanguageXMLLogin_Failed;
extern const char * LanguageXMLCannot_Connect;
extern const char * LanguageXMLServer_Closed;
extern const char * LanguageXMLYes;
extern const char * LanguageXMLNo;

extern "C" char * ctrl_gettext(const char *) ;

#endif
