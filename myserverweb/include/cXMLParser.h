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

#ifndef CXMLPARSER_IN
#define CXMLPARSER_IN

#include "../stdafx.h"
#include "../include/filemanager.h"
#include "../include/MemBuf.h"
extern "C" 
{
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h> 
}
/*!
 *This class is used to open a .xml file and read informations from it.
 */
class cXMLParser
{
	xmlDocPtr doc;
	char buffer[250];
	xmlNodePtr cur;
	xmlNodePtr prev_cur;
	xmlNodePtr last_node;
public:
	static int startXML();
	static int cleanXML();
	cXMLParser();
	xmlDocPtr getDoc();
	int open(char* filename);
	int openMemBuf(CMemBuf &);
	char *getValue(char* field);
	char *getAttr(char* field,char *attr);
	int setValue(char* field,char *value);
	int close();
	int save(char *filename,int *nbytes=0);
	int saveMemBuf(CMemBuf &,int *nbytes=0);
	void newfile(const char * root);
	void addChild(const char * name, const char * value);
	void addGroup(const char * name);
	void endGroup();
	void setAttr(const char * name, const char * value);
};

#endif
