/*
 MyServer
 Copyright (C) 2002, 2003, 2004 The MyServer Team
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

// Compile with g++ -o textmangler textmangler.cpp `xml2-config --cflags --libs`
// Run with textmangler textfile.txt languagefile.xml
// Where textfile.txt is the english prases in the program to use
// gettext in language.cpp

#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>

extern "C" 
{   
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
}


using namespace std;

int main(int argn, char * argv[])
{
   char Buffer[255];
   char Bufferin[255];
   char Bufferout[255];
   int val;
   int i, len;
   
   xmlDocPtr doc;
   xmlNodePtr cur;
   
   if(argn < 3)
     return -1;
   
   fstream filein, fileout;
   
   filein.open(argv[1], ios::in);
   if(!filein)
     return -1;

   xmlInitParser();
   
   doc = xmlNewDoc((const xmlChar*)"1.0");
   cur = xmlNewDocNode(doc, NULL, (const xmlChar*)"MYSERVER_LANGUAGE_FILE", NULL);
   xmlDocSetRootElement(doc, cur);
   
   filein.getline(Bufferin, 255);
   while(!filein.eof())
     {
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
	
	xmlNewTextChild(cur, NULL, (const xmlChar*)Bufferout, (const xmlChar*)Bufferin);
	
	filein.getline(Bufferin, 255);
     }
   filein.close();
   xmlSaveFile(argv[2], doc);
   xmlFreeDoc(doc);
   return 0;
}
