/*
 * MyServer
 * Copyright (C) 2002,2003,2004 The MyServer Team
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
 */
#include <string.h>
extern "C"
{
#include <stdlib.h>
}

#include "../include/filemanager.h"
#include "../include/stringutils.h"
#include "../include/cXMLParser.h"
#include "vhost.h"

using namespace std;

const char * EMPTY = "";

VHostXML::~VHostXML()
{
   clear();
   Dynamic.clear();
}

void VHostXML::clear()
{
   for(int i = 0; i < VHosts.size(); i++)
     DeleteVHostNode((VHostNode *)(VHosts.at(i)->Data));
   VHosts.clear();
}

int VHostXML::load(const char * filename)
{
   XmlParser parser;
   if(parser.open((char *)filename))  // But I promis not to change filename
     return -1;
   
   int ret = load_core(parser);
   parser.close();
   return ret;
}

int VHostXML::loadMemBuf(CMemBuf & buffer)
{
   XmlParser parser;
   if(parser.openMemBuf(buffer))
     return -1;
   
   int ret = load_core(parser);
   parser.close();
   return ret;
}

// from vhosts.cpp with modification
// TODO: Change to use libxml2 or cXMLParser more "proper"
int VHostXML::load_core(XmlParser & parser)
{
   int NameNo = 0;

   clear();
   
   xmlDocPtr doc = parser.getDoc();
   xmlNodePtr node=doc->children->children;
   for(;node;node=node->next )
     {
	if(xmlStrcmp(node->name, (const xmlChar *)"VHOST"))
	  continue;

	xmlNodePtr lcur=node->children;

	while(lcur)
	  {
	     if(!xmlStrcmp(lcur->name, (const xmlChar *)"HOST"))
	       {
		  bool useRegex = false;
		  xmlAttr *attrs = lcur->properties;
		  while(attrs)
		    {
		       if(!xmlStrcmp(attrs->name, (const xmlChar *)"isRegex"))
			 {
			    if(attrs->children && attrs->children->content &&
			       (!xmlStrcmp(attrs->children->content, (const xmlChar *)"YES")))
			      {
				 useRegex = true;
			      }
			 }
		       attrs=attrs->next;
		    }
		  addHost(NameNo, (char*)lcur->children->content, useRegex);
	       }
	     else if(!xmlStrcmp(lcur->name, (const xmlChar *)"NAME"))
	       {
		  NameNo = addName((char*)lcur->children->content);
	       }
	     else if(!xmlStrcmp(lcur->name, (const xmlChar *)"SSL_PRIVATEKEY"))
	       {
		  setSsl_Privatekey(NameNo, (char*)lcur->children->content);
	       }
	     else if(!xmlStrcmp(lcur->name, (const xmlChar *)"SSL_CERTIFICATE"))
	       {
		  setSsl_Certificate(NameNo, (char*)lcur->children->content);
	       }
	     else if(!xmlStrcmp(lcur->name, (const xmlChar *)"SSL_PASSWORD"))
	       {
		  if(lcur->children)
		    setSsl_Password(NameNo, (char*)lcur->children->content);
		  else
		    setSsl_Password(NameNo, (char*)EMPTY);
	       }
	     else if(!xmlStrcmp(lcur->name, (const xmlChar *)"IP"))
	       {
		  bool useRegex = false;
		  xmlAttr *attrs =  lcur->properties;
		  while(attrs)
		    {
		       if(!xmlStrcmp(attrs->name, (const xmlChar *)"isRegex"))
			 {
			    if(attrs->children && attrs->children->content &&
			       (!xmlStrcmp(attrs->children->content, (const xmlChar *)"YES")))
			      {
				 useRegex = true;
			      }
			 }
		       attrs=attrs->next;
		    }
		  addIp(NameNo, (char*)lcur->children->content, useRegex);
	       }
	     else if(!xmlStrcmp(lcur->name, (const xmlChar *)"PORT"))
	       {
		  setPort(NameNo, (int)atoi((char*)lcur->children->content));
	       }
	     else if(!xmlStrcmp(lcur->name, (const xmlChar *)"PROTOCOL"))
	       {
		  if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"HTTP"))
		    setProtocol(NameNo, PROTOCOL_HTTP);
		  else if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"HTTPS"))
		    setProtocol(NameNo, PROTOCOL_HTTPS);
		  else if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"FTP"))
		    setProtocol(NameNo, PROTOCOL_FTP);
		  else if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"CONTROL"))
		    setProtocol(NameNo, PROTOCOL_CONTROL);
		  else
		    { // dynamic protocol
		       int i = Dynamic.get((const char *)lcur->children->content);
		       if(i != -1)
			 setProtocol(NameNo, i + PROTOCOL_DYNAMIC);
		       else
			 setProtocol(NameNo, PROTOCOL_HTTP);
		    }
	       }
	     else if(!xmlStrcmp(lcur->name, (const xmlChar *)"DOCROOT"))
	       {
      if(lcur->children &&  lcur->children->content)
        setDocroot(NameNo, (char*)lcur->children->content);
      else
        setDocroot(NameNo, "");
	       }
	     else if(!xmlStrcmp(lcur->name, (const xmlChar *)"SYSFOLDER"))
	       {
      if(lcur->children &&  lcur->children->content)
        setSysfolder(NameNo, (char*)lcur->children->content);
      else
        setSysfolder(NameNo, "");
	       }
	     else if(!xmlStrcmp(lcur->name, (const xmlChar *)"ACCESSESLOG"))
	       {
		  setAccesseslog(NameNo, (char*)lcur->children->content);
	       }
	     else if(!xmlStrcmp(lcur->name, (const xmlChar *)"WARNINGLOG"))
	       {
		  setWarninglog(NameNo, (char*)lcur->children->content);
	       }
	     lcur=lcur->next;
	  }
     }
   return 0;
}

int VHostXML::save(const char * filename)
{
   XmlParser xmlFile;
   int ret = save_core(xmlFile);
   xmlFile.save((char *)filename);
   xmlFile.close();
   return ret;
}

int VHostXML::saveMemBuf(CMemBuf & buffer)
{
   XmlParser xmlFile;
   int ret = save_core(xmlFile);
   xmlFile.saveMemBuf(buffer);
   xmlFile.close();
   return ret;
}

// from vhosts.cpp with modification
// Old text way removed to make use of CMemBuf
int VHostXML::save_core(XmlParser & xmlFile)
{
   if(VHosts.isempty())
     return -1;

   // New xml way...
   int i, i2;
   
   xmlFile.newfile("VHOSTS");
   
   for(i = 0; i < VHosts.size(); i++)
     {
	xmlFile.addGroup("VHOST");

	xmlFile.addChild("NAME", VHosts.at(i)->Text);

	for(i2 = 0; i2 < ((VHostNode *)(VHosts.at(i)->Data))->Ip.size(); i2++)
	  {
	     xmlFile.addChild("IP", ((VHostNode *)(VHosts.at(i)->Data))->Ip.at(i2)->Text);
	  }

	for(i2 = 0; i2 < ((VHostNode *)(VHosts.at(i)->Data))->Host.size(); i2++)
	  {
	     xmlFile.addChild("HOST", ((VHostNode *)(VHosts.at(i)->Data))->Host.at(i2)->Text);
	  }

	char port[6];
	snprintf(port,6,"%i",getPort(i));
	xmlFile.addChild("PORT", port);

	if(getSsl_Privatekey(i) != EMPTY)
	  {
	     xmlFile.addChild("SSL_PRIVATEKEY", getSsl_Privatekey(i));
	  }

	if(getSsl_Certificate(i) != EMPTY)
	  {
	     xmlFile.addChild("SSL_CERTIFICATE", getSsl_Certificate(i));
	  }

	if(getSsl_Password(i) != EMPTY)
	  {
	     xmlFile.addChild("SSL_PASSWORD", getSsl_Password(i));
	  }

	switch(getProtocol(i))
	  {
	   case PROTOCOL_HTTP:
	     xmlFile.addChild("PROTOCOL", "HTTP");
	     break;
	   case PROTOCOL_HTTPS:
	     xmlFile.addChild("PROTOCOL", "HTTPS");
	     break;
	   case PROTOCOL_FTP:
	     xmlFile.addChild("PROTOCOL", "FTP");
	     break;
	   case PROTOCOL_CONTROL:
	     xmlFile.addChild("PROTOCOL", "CONTROL");
	     break;
	   default: // Dynamic protocol
	     xmlFile.addChild("PROTOCOL", Dynamic.at(getProtocol(i) - PROTOCOL_DYNAMIC)->Text);
	     break;
	  }

	xmlFile.addChild("DOCROOT", getDocroot(i));

	xmlFile.addChild("SYSFOLDER", getSysfolder(i));

	xmlFile.addChild("ACCESSESLOG", getAccesseslog(i));

	xmlFile.addChild("WARNINGLOG", getWarninglog(i));

	xmlFile.endGroup();
     }
   return 0;
}

void VHostXML::populateName(Fl_Choice * o)
{
   o->clear();
   for(int i = 0; i < VHosts.size(); i++)
     o->add(VHosts.at(i)->Text);
   if(VHosts.size() == 0)
     o->add(" ");
}

void VHostXML::populateHost(int VHostNo, Fl_Browser * o)
{
   o->clear();
   if(VHosts.isempty())
     return;
   for(int i = 0; i < ((VHostNode *)(VHosts.at(VHostNo)->Data))->Host.size(); i++)
     o->add(((VHostNode *)(VHosts.at(VHostNo)->Data))->Host.at(i)->Text);
}

void VHostXML::populateIp(int VHostNo, Fl_Browser * o)
{
   o->clear();
   if(VHosts.isempty())
     return;
   for(int i = 0; i < ((VHostNode *)(VHosts.at(VHostNo)->Data))->Ip.size(); i++)
     o->add(((VHostNode *)(VHosts.at(VHostNo)->Data))->Ip.at(i)->Text);
}

int VHostXML::addName(const char * Text)
{
   VHostNode * NewNode = new VHostNode;
   NewNode->Host.clear();
   NewNode->Ip.clear();
   NewNode->Port = 0;
   NewNode->Protocol = 0;
   NewNode->Ssl_Privatekey = (char *)EMPTY;
   NewNode->Ssl_Certificate = (char *)EMPTY;
   NewNode->Ssl_Password = (char *)EMPTY;
   NewNode->Docroot = (char *)EMPTY;
   NewNode->Sysfolder = (char *)EMPTY;
   NewNode->Accesseslog = (char *)EMPTY;
   NewNode->Warninglog = (char *)EMPTY;
   VectorNode * ret = VHosts.add(Text, (void *)NewNode);
   VHosts.sort();
   return ret->Number;
}

int VHostXML::addHost(int VHostNo, const char * Text, bool isRegx)
{
   if(VHosts.isempty())
     return 0;
   VectorNode * ret = ((VHostNode *)(VHosts.at(VHostNo)->Data))->Host.add(Text, (void *)isRegx);
   ((VHostNode *)(VHosts.at(VHostNo)->Data))->Host.sort();
   return ret->Number;
}

int VHostXML::addIp(int VHostNo, const char * Text, bool isRegx)
{
   if(VHosts.isempty())
     return 0;
   VectorNode * ret = ((VHostNode *)(VHosts.at(VHostNo)->Data))->Ip.add(Text, (void *)isRegx);
   ((VHostNode *)(VHosts.at(VHostNo)->Data))->Ip.sort();
   return ret->Number;
}

void VHostXML::removeName(int VHostNo)
{
   if(VHosts.isempty())
     return;
   DeleteVHostNode((VHostNode *)(VHosts.at(VHostNo)->Data));
   VHosts.remove(VHostNo);
}

void VHostXML::removeHost(int VHostNo, int index)
{
   if(VHosts.isempty())
     return;
   ((VHostNode *)(VHosts.at(VHostNo)->Data))->Host.remove(index);
}

void VHostXML::removeIp(int VHostNo, int index)
{
   if(VHosts.isempty())
     return;
   ((VHostNode *)(VHosts.at(VHostNo)->Data))->Ip.remove(index);
}

void VHostXML::setPort(int VHostNo, int val)
{
   if(VHosts.isempty())
     return;
   ((VHostNode *)(VHosts.at(VHostNo)->Data))->Port = val;
}

void VHostXML::setProtocol(int VHostNo, int val)
{
   if(VHosts.isempty())
     return;
   ((VHostNode *)(VHosts.at(VHostNo)->Data))->Protocol = val;
}

static inline void setstr(char *& dest, const char * val)
{
   if(dest != EMPTY)
     free(dest);
   if(val[0] == '\0')
     dest = (char *)EMPTY;
   else
     dest = strdup(val);
}

void VHostXML::setSsl_Privatekey(int VHostNo, const char * val)
{
   if(VHosts.isempty())
     return;
   setstr(((VHostNode *)(VHosts.at(VHostNo)->Data))->Ssl_Privatekey, val);
}

void VHostXML::setSsl_Certificate(int VHostNo, const char * val)
{
   if(VHosts.isempty())
     return;
   setstr(((VHostNode *)(VHosts.at(VHostNo)->Data))->Ssl_Certificate, val);
}

void VHostXML::setSsl_Password(int VHostNo, const char * val)
{
   if(VHosts.isempty())
     return;
   setstr(((VHostNode *)(VHosts.at(VHostNo)->Data))->Ssl_Password, val);
}

void VHostXML::setDocroot(int VHostNo, const char * val)
{
   if(VHosts.isempty())
     return;
   setstr(((VHostNode *)(VHosts.at(VHostNo)->Data))->Docroot, val);
}

void VHostXML::setSysfolder(int VHostNo, const char * val)
{
   if(VHosts.isempty())
     return;
   setstr(((VHostNode *)(VHosts.at(VHostNo)->Data))->Sysfolder, val);
}

void VHostXML::setAccesseslog(int VHostNo, const char * val)
{
   if(VHosts.isempty())
     return;
   setstr(((VHostNode *)(VHosts.at(VHostNo)->Data))->Accesseslog, val);
}

void VHostXML::setWarninglog(int VHostNo, const char * val)
{
   if(VHosts.isempty())
     return;
   setstr(((VHostNode *)(VHosts.at(VHostNo)->Data))->Warninglog, val);
}

int VHostXML::getPort(int VHostNo)
{
   if(VHosts.isempty())
     return 0;
   return ((VHostNode *)(VHosts.at(VHostNo)->Data))->Port;
}

int VHostXML::getProtocol(int VHostNo)
{
   if(VHosts.isempty())
     return 0;
   return ((VHostNode *)(VHosts.at(VHostNo)->Data))->Protocol;
}

const char * VHostXML::getSsl_Privatekey(int VHostNo)
{
   if(VHosts.isempty())
     return EMPTY;
   return ((VHostNode *)(VHosts.at(VHostNo)->Data))->Ssl_Privatekey;
}

const char * VHostXML::getSsl_Certificate(int VHostNo)
{
   if(VHosts.isempty())
     return EMPTY;
   return ((VHostNode *)(VHosts.at(VHostNo)->Data))->Ssl_Certificate;
}

const char * VHostXML::getSsl_Password(int VHostNo)
{
   if(VHosts.isempty())
     return EMPTY;
   return ((VHostNode *)(VHosts.at(VHostNo)->Data))->Ssl_Password;
}

const char * VHostXML::getDocroot(int VHostNo)
{
   if(VHosts.isempty())
     return EMPTY;
   return ((VHostNode *)(VHosts.at(VHostNo)->Data))->Docroot;
}

const char * VHostXML::getSysfolder(int VHostNo)
{
   if(VHosts.isempty())
     return EMPTY;
   return ((VHostNode *)(VHosts.at(VHostNo)->Data))->Sysfolder;
}

const char * VHostXML::getAccesseslog(int VHostNo)
{
   if(VHosts.isempty())
     return EMPTY;
   return ((VHostNode *)(VHosts.at(VHostNo)->Data))->Accesseslog;
}

const char * VHostXML::getWarninglog(int VHostNo)
{
   if(VHosts.isempty())
     return EMPTY;
   return ((VHostNode *)(VHosts.at(VHostNo)->Data))->Warninglog;
}

void VHostXML::populateProtocol(Fl_Choice * o)
{
   // Fl_Menu_ has a "terminator" item
   while(o->size() - 1 > PROTOCOL_DYNAMIC)
     {
	o->remove(o->size() - 2);
     }
   for(int i = 0; i < Dynamic.size(); i++)
     {
	o->add(Dynamic.at(i)->Text, 0, 0, 0, 0);
     }
}

void VHostXML::loadProtocols(Vector & list)
{
   Dynamic.clear();
   Dynamic.add(list);
}

static inline void delstr(char * val)
{
   if(val != EMPTY)
     free(val);
}

void VHostXML::DeleteVHostNode(VHostNode * Node)
{
   Node->Host.clear();
   Node->Ip.clear();
   delstr(Node->Ssl_Privatekey);
   delstr(Node->Ssl_Certificate);
   delstr(Node->Ssl_Password);
   delstr(Node->Docroot);
   delstr(Node->Sysfolder);
   delstr(Node->Accesseslog);
   delstr(Node->Warninglog);
   delete Node;
}

