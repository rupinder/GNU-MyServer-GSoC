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

#include "../include/cXMLParser.h"
#include "../include/utility.h"



extern "C" {
#include <string.h>
}

#ifndef lstrlen
#define lstrlen strlen
#endif

#ifdef WIN32
/*!
*Libxml2.lib is the dynamic version of the libxml2 while libxml2_a.lib is static.
*With static version use the linker options: /NODEFAULTLIB:LIBCMT /NODEFAULTLIB:LIBCMTD.
*/

//#pragma comment (lib,"libxml2_a.lib")
#pragma comment (lib,"libxml2.lib")

#endif
/*!
*Initialize the libxml2 library
*/
int cXMLParser::startXML()
{
	xmlInitParser();
	return 1;
}
/*!
*Cleanup the libxml2 library.
*/
int cXMLParser::cleanXML()
{
	xmlCleanupParser();
	return 1;
}
/*!
*With the open function we open a file and store it in memory.
*Return nonzero on errors.
*/
int cXMLParser::open(char* filename)
{
	cur=0;
	if(!MYSERVER_FILE::fileExists(filename))
		return -1;
	if(doc==0)
		doc = xmlParseFile(filename);
	else
		close();
	if(!doc)
		return -1;
	cur = xmlDocGetRootElement(doc);
	if(!cur)
	{
		close();
		return -1;
	}
	return 0;
}
/*!
*Constructor of the cXMLParser class.
*/
cXMLParser::cXMLParser()
{
	doc=0;
	cur=0;
}
/*!
*Return the xml Document.
*/
xmlDocPtr cXMLParser::getDoc()
{
	return doc;
}
/*!
*Only get the value of the vName root children element.
*/
char *cXMLParser::getValue(char* vName)
{
	if(!cur)
		return 0;
	xmlNodePtr lcur=cur->xmlChildrenNode;
	buffer[0]='\0';
	while(lcur)
	{
		if(!xmlStrcmp(lcur->name, (const xmlChar *)vName))
		{
			if(lcur->children->content)
				strncpy(buffer,(char*)lcur->children->content,250);
			break;
		}
		lcur=lcur->next;
	}
	
	return buffer;
}
/*!
*Set the value of the vName root children element.
*/
int cXMLParser::setValue(char* vName,char *value)
{
	xmlNodePtr lcur=cur->xmlChildrenNode;
	buffer[0]='\0';
	while(lcur)
	{
		if(!xmlStrcmp(lcur->name, (const xmlChar *)vName))
		{
			if(lcur->children->content)
				strcpy((char*)lcur->children->content,value);
			return 1;
		}
		lcur=lcur->next;
	}
	return 0;
}
/*!
*Get the attribute attr for the node field
*/
char *cXMLParser::getAttr(char* field,char *attr)
{
	xmlNodePtr lcur=cur->xmlChildrenNode;
	buffer[0]='\0';
	while(lcur)
	{
		if(!xmlStrcmp(lcur->name, (const xmlChar *)field))
		{
			xmlAttr *attrs =  lcur->properties;
			while(attrs)
			{
				if(!xmlStrcmp(attrs->name, (const xmlChar *)attr))
					return (char*)attrs->children->content;
				
				attrs=attrs->next;
			}
		}
		lcur=lcur->next;
	}
	return 0;
}
/*!
*Free the memory used by the class.
*/
int cXMLParser::close()
{
	xmlFreeDoc(doc);
	doc=0;
	cur=0;
	return 0;
}
/*!
*Save the XML tree to a file
*/
int cXMLParser::save(char *filename)
{
	return xmlSaveFile(filename,doc);
}
