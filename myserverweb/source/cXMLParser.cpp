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

#include "../include/cXMLParser.h"
#include "../include/utility.h"



extern "C" {
#include <string.h>
}

#ifndef WIN32
#define lstrlen strlen
#endif

#ifdef WIN32
/*
*By default use the static version of libxml2.
*/
#pragma comment (lib,"libxml2_a.lib")
/*
*Use this to use the dynamic version.
#pragma comment (lib,"libxml2.lib")
*/
#endif

/*
*This code is used to parse a pseudo-xml file.
*With the open function we open a file and store it in memory.
*/
int cXMLParser::open(char* filename)
{
	doc = xmlParseFile(filename);
	if(!doc)
		return -1;
    cur = xmlDocGetRootElement(doc);
	if(!cur)
		return -1;
	return 0;
}
/*
*Constructor of the cXMLParser class.
*/
cXMLParser::cXMLParser()
{
	doc=0;
	cur=0;
}
/*
*Return the xml Document.
*/
xmlDocPtr cXMLParser::getDoc()
{
	return doc;
}
/*
*Only get the value of the vName root children element.
*/
char *cXMLParser::getValue(char* vName)
{
	xmlNodePtr lcur=cur->xmlChildrenNode;
	buffer[0]='\0';
	while(lcur)
	{
		if(!xmlStrcmp(lcur->name, (const xmlChar *)vName))
		{
			if(lcur->children->content)
				strcpy(buffer,(char*)lcur->children->content);
			break;
		}
		lcur=lcur->next;
	}
	
	return buffer;
}

/*
*Free the memory used by the class.
*/
int cXMLParser::close()
{
	xmlFreeDoc(doc);
	return 0;
}
int cXMLParser::save(char *filename)
{
	return xmlSaveFile(filename,doc);
}
