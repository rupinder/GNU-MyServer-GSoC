/*
 * *MyServer
 * *Copyright (C) 2002,2003,2004 The MyServer Team
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * */
#include <string.h>
extern "C"
{
#include <stdlib.h>
}

#include "../include/filemanager.h"
#include "../include/stringutils.h"
#include "../include/cXMLParser.h"
#include "mimetype.h"

using namespace std;

//#define DEBUG

const char * NONE = "NONE";

MIMEtypeXML::~MIMEtypeXML()
{
   Mime.clear();
   ClearExt();
}

void MIMEtypeXML::clear()
{
   Mime.clear();
   ClearExt();
}

int MIMEtypeXML::load(const char * filename)
{
   XmlParser parser;
   if(parser.open((char *)filename)) // but I promis I wont modify
     return -1;
   int ret = load_core(parser);
   parser.close();
   return ret;
}

int MIMEtypeXML::loadMemBuf(CMemBuf & buffer)
{
   XmlParser parser;
   if(parser.openMemBuf(buffer))
     return -1;
   int ret = load_core(parser);
   parser.close();
   return ret;
}

// Copied and modified from MIME_manager.cpp
// TODO: Change to use libxml2 or XmlParser more "proper"
int MIMEtypeXML::load_core(XmlParser & parser)
{
   int extNumber = 0;

   clear();

   xmlDocPtr doc = parser.getDoc();
   xmlNodePtr node=doc->children->children;
   for(;node;node=node->next )
     {
	if(xmlStrcmp(node->name, (const xmlChar *)"MIMETYPE"))
	  continue;
	xmlNodePtr lcur=node->children;
	while(lcur)
	  {
	     if(!xmlStrcmp(lcur->name, (const xmlChar *)"EXT"))
	       {
		  if(lcur->children->content)
		    extNumber = addExt((char*)lcur->children->content);
#ifdef DEBUG
		  printf("EXT: %s, No: %d\n", (char*)lcur->children->content, extNumber);
#endif
	       }
	     else if(!xmlStrcmp(lcur->name, (const xmlChar *)"MIME"))
	       {
		  if(lcur->children->content)
		    setType(extNumber, addMime((char*)lcur->children->content));
#ifdef DEBUG
		  printf("MIME: %s, No: %d, Result: %s, %d\n", (char*)lcur->children->content, extNumber, Mime.at(getType(extNumber))->Text, getType(extNumber));
#endif
	       }
	     else if(!xmlStrcmp(lcur->name, (const xmlChar *)"CMD"))
	       {
		  if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"SEND"))
		    setCmd(extNumber, CMD_SEND);
		  else if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"RUNCGI"))
		    setCmd(extNumber, CMD_RUNCGI);
		  else if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"RUNMSCGI"))
		    setCmd(extNumber, CMD_RUNMSCGI);
		  else if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"EXECUTE"))
		    setCmd(extNumber, CMD_EXECUTE);
		  else if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"RUNISAPI"))
		    setCmd(extNumber, CMD_RUNISAPI);
		  else if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"EXECUTEISAPI"))
		    setCmd(extNumber, CMD_EXECUTEISAPI);
		  else if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"SENDLINK"))
		    setCmd(extNumber, CMD_SENDLINK);
		  else if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"EXECUTEWINCGI"))
		    setCmd(extNumber, CMD_EXECUTEWINCGI);
		  else if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"RUNFASTCGI"))
		    setCmd(extNumber, CMD_RUNFASTCGI);
		  else if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"EXECUTEFASTCGI"))
		    setCmd(extNumber, CMD_EXECUTEFASTCGI);
#ifdef DEBUG
		  printf("CMD: %s, No: %d, Result: %d\n", (char*)lcur->children->content, extNumber, getCmd(extNumber));
#endif
	       }
	     else if(!xmlStrcmp(lcur->name, (const xmlChar *)"MANAGER"))
	       {
		  if(lcur->children->content)
		    {
		       if(lstrcmpi((char*)lcur->children->content,"NONE") == 0)
			 setManager(extNumber, NONE);
		       else
			 setManager(extNumber, (char*)lcur->children->content);
#ifdef DEBUG
		       printf("MANAGER: %s, No: %d, Result: %s\n", (char*)lcur->children->content, extNumber, getManager(extNumber));
#endif
		    }
	       }
	     lcur=lcur->next;
	  }
	// while(lcur)
     }
   // for(;node;node=node->next )
   Mime.sort();
   
   return 0;
}

int MIMEtypeXML::save(const char * filename)
{
   XmlParser xmlFile;
   int ret = save_core(xmlFile);
   xmlFile.save((char *)filename); // But, but, but I promis
   xmlFile.close();
   return ret;
}

int MIMEtypeXML::saveMemBuf(CMemBuf & buffer)
{
   XmlParser xmlFile;
   int ret = save_core(xmlFile);
   xmlFile.saveMemBuf(buffer);
   xmlFile.close();
   return ret;
}
   
// Copied and modified from MIME_manager.cpp
// Old text way remove to make use of CMemBuf
int MIMEtypeXML::save_core(XmlParser & xmlFile)
{
   if(Ext.isempty())
     return -1;

   // New cXMLParser way...
   int i;
   
   xmlFile.newfile("MIMETYPES");
   
   for(i = 0; i < Ext.size(); i++)
     {
	xmlFile.addGroup("MIMETYPE");
	xmlFile.addChild("EXT", Ext.at(i)->Text);
	xmlFile.addChild("MIME", Mime.at(getType(i))->Text);
	char command[16];
	switch(getCmd(i))
	  {
	   case CMD_SEND:
	     strcpy(command,"SEND");
	     break;
	   case CMD_RUNCGI:
	     strcpy(command,"RUNCGI");
	     break;
	   case CMD_RUNMSCGI:
	     strcpy(command,"RUNMSCGI");
	     break;
	   case CMD_EXECUTE:
	     strcpy(command,"EXECUTE");
	     break;
	   case CMD_SENDLINK:
	     strcpy(command,"SENDLINK");
	     break;
	   case CMD_RUNISAPI:
	     strcpy(command,"RUNISAPI");
	     break;
	   case CMD_EXECUTEISAPI:
	     strcpy(command,"EXECUTEISAPI");
	     break;
	   case CMD_EXECUTEWINCGI:
	     strcpy(command,"EXECUTEWINCGI");
	     break;
	   case CMD_RUNFASTCGI:
	     strcpy(command,"RUNFASTCGI");
	     break;
	   case CMD_EXECUTEFASTCGI:
	     strcpy(command,"EXECUTEFASTCGI");
	     break;
	   default:
	     break;
	  }
	xmlFile.addChild("CMD", command);
	xmlFile.addChild("MANAGER", getManager(i));
	xmlFile.endGroup();
     }
   
   return 0;
}

void MIMEtypeXML::populateExt(Fl_Browser * o)
{
   o->clear();
   for(int i = 0; i < Ext.size(); i++)
     o->add(Ext.at(i)->Text);
}

void MIMEtypeXML::populateMime(Fl_Choice * o)
{
   o->clear();
   int len, index;
   char * chrptr;
   for(int i = 0; i < Mime.size(); i++)
     {  // Fl_Choice uses '/' as a seperatior so replace with ' '
        chrptr = Mime.at(i)->Text;
        len = strlen(chrptr) + 1;
	char temp[len];
	for(index = 0; index < len; index++)
	  if(chrptr[index] == '/')
	    temp[index] = ' ';
	else
	  temp[index] = chrptr[index];

        o->add(temp, 0, 0, 0, 0);
     }
}

int MIMEtypeXML::addExt(const char * name)
{
   VectorNode * ret;
   int i;
   i = Ext.get(name);
   if(i != -1)
     return i;
   MimeNode * NewNode = new MimeNode;
   NewNode->Type = NULL;
   NewNode->Cmd = 0;
   NewNode->Manager = (char *)NONE;
   ret = Ext.add(name, (void *)NewNode);
   Ext.sort();
   return ret->Number;
}

int MIMEtypeXML::addMime(const char * name)
{
   int i;
   VectorNode * ret;
   i = Mime.get(name);
   if(i != -1)
     return i;
   ret = Mime.add(name);
   Mime.sort();
   return ret->Number;
}

void MIMEtypeXML::removeExt(int extNumber)
{
   DeleteMimeNode(((MimeNode *)(Ext.at(extNumber)->Data)));
   Ext.remove(extNumber);
}

void MIMEtypeXML::setType(int extNumber, int mimeNumber)
{
   ((MimeNode *)(Ext.at(extNumber)->Data))->Type = Mime.at(mimeNumber);
}

void MIMEtypeXML::setCmd(int extNumber, int cmdNumber)
{
   ((MimeNode *)(Ext.at(extNumber)->Data))->Cmd = cmdNumber;
}

void MIMEtypeXML::setManager(int extNumber, const char * name)
{
   char * chrptr = ((MimeNode *)(Ext.at(extNumber)->Data))->Manager;
   if(chrptr != NULL && chrptr != NONE)
     free(chrptr);
   if(name != NONE)
     ((MimeNode *)(Ext.at(extNumber)->Data))->Manager = strdup(name);
   else
     ((MimeNode *)(Ext.at(extNumber)->Data))->Manager = (char *)NONE;
}

int MIMEtypeXML::getType(int extNumber)
{
   VectorNode * ret = ((MimeNode *)(Ext.at(extNumber)->Data))->Type;
   if(ret == NULL)
     return 0;
   return ret->Number;
}

int MIMEtypeXML::getCmd(int extNumber)
{
   return ((MimeNode *)(Ext.at(extNumber)->Data))->Cmd;
}

const char * MIMEtypeXML::getManager(int extNumber)
{
   return ((MimeNode *)(Ext.at(extNumber)->Data))->Manager;
}

void MIMEtypeXML::ClearExt()
{
   int i;
   for(i = 0; i < Ext.size(); i++)
     {
	DeleteMimeNode((MimeNode *)(Ext.at(i)->Data));
     }
   Ext.clear();
}

void MIMEtypeXML::DeleteMimeNode(MimeNode * Node)
{
   if(Node->Manager != NULL && Node->Manager != NONE)
     free(Node->Manager);
   delete Node;
}
