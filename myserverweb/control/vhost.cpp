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
#include <cstring>
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

vHostXML::~vHostXML()
{
   clear();
}

void vHostXML::clear()
{
   for(int i = 0; i < vHosts.size(); i++)
     DeletevHostNode((vHostNode *)(vHosts.at(i)->Data));
   vHosts.clear();
}

int vHostXML::load(const char * filename)
{
   cXMLParser parser;
   if(parser.open((char *)filename))  // But I promis not to change filename
     return -1;
   
   int ret = load_core(parser);
   parser.close();
   return ret;
}

int vHostXML::loadMemBuf(CMemBuf & buffer)
{
   cXMLParser parser;
   if(parser.openMemBuf(buffer))
     return -1;
   
   int ret = load_core(parser);
   parser.close();
   return ret;
}

// from vhosts.cpp with modification
// TODO: Change to use libxml2 or cXMLParser more "proper"
int vHostXML::load_core(cXMLParser & parser)
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
		    {
		       // Do nothing?
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

int vHostXML::save(const char * filename)
{
   cXMLParser xmlFile;
   int ret = save_core(xmlFile);
   xmlFile.save((char *)filename);
   xmlFile.close();
   return ret;
}

int vHostXML::saveMemBuf(CMemBuf & buffer)
{
   cXMLParser xmlFile;
   int ret = save_core(xmlFile);
   xmlFile.saveMemBuf(buffer);
   xmlFile.close();
   return ret;
}

// from vhosts.cpp with modification
// Old text way removed to make use of CMemBuf
int vHostXML::save_core(cXMLParser & xmlFile)
{
   if(vHosts.isempty())
     return -1;

   // New xml way...
   int i, i2;
   
   xmlFile.newfile("VHOSTS");
   
   for(i = 0; i < vHosts.size(); i++)
     {
	xmlFile.addGroup("VHOST");

	xmlFile.addChild("NAME", vHosts.at(i)->Text);

	for(i2 = 0; i2 < ((vHostNode *)(vHosts.at(i)->Data))->Ip.size(); i2++)
	  {
	     xmlFile.addChild("IP", ((vHostNode *)(vHosts.at(i)->Data))->Ip.at(i2)->Text);
	  }

	for(i2 = 0; i2 < ((vHostNode *)(vHosts.at(i)->Data))->Host.size(); i2++)
	  {
	     xmlFile.addChild("HOST", ((vHostNode *)(vHosts.at(i)->Data))->Host.at(i2)->Text);
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
	   default:
	     // do something here?
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

void vHostXML::populateName(Fl_Choice * o)
{
   o->clear();
   for(int i = 0; i < vHosts.size(); i++)
     o->add(vHosts.at(i)->Text);
   if(vHosts.size() == 0)
     o->add(" ");
}

void vHostXML::populateHost(int vHostNo, Fl_Browser * o)
{
   o->clear();
   if(vHosts.isempty())
     return;
   for(int i = 0; i < ((vHostNode *)(vHosts.at(vHostNo)->Data))->Host.size(); i++)
     o->add(((vHostNode *)(vHosts.at(vHostNo)->Data))->Host.at(i)->Text);
}

void vHostXML::populateIp(int vHostNo, Fl_Browser * o)
{
   o->clear();
   if(vHosts.isempty())
     return;
   for(int i = 0; i < ((vHostNode *)(vHosts.at(vHostNo)->Data))->Ip.size(); i++)
     o->add(((vHostNode *)(vHosts.at(vHostNo)->Data))->Ip.at(i)->Text);
}

int vHostXML::addName(const char * Text)
{
   vHostNode * NewNode = new vHostNode;
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
   VectorNode * ret = vHosts.add(Text, (void *)NewNode);
   vHosts.sort();
   return ret->Number;
}

int vHostXML::addHost(int vHostNo, const char * Text, bool isRegx)
{
   if(vHosts.isempty())
     return 0;
   VectorNode * ret = ((vHostNode *)(vHosts.at(vHostNo)->Data))->Host.add(Text, (void *)isRegx);
   ((vHostNode *)(vHosts.at(vHostNo)->Data))->Host.sort();
   return ret->Number;
}

int vHostXML::addIp(int vHostNo, const char * Text, bool isRegx)
{
   if(vHosts.isempty())
     return 0;
   VectorNode * ret = ((vHostNode *)(vHosts.at(vHostNo)->Data))->Ip.add(Text, (void *)isRegx);
   ((vHostNode *)(vHosts.at(vHostNo)->Data))->Ip.sort();
   return ret->Number;
}

void vHostXML::removeName(int vHostNo)
{
   if(vHosts.isempty())
     return;
   DeletevHostNode((vHostNode *)(vHosts.at(vHostNo)->Data));
   vHosts.remove(vHostNo);
}

void vHostXML::removeHost(int vHostNo, int index)
{
   if(vHosts.isempty())
     return;
   ((vHostNode *)(vHosts.at(vHostNo)->Data))->Host.remove(index);
}

void vHostXML::removeIp(int vHostNo, int index)
{
   if(vHosts.isempty())
     return;
   ((vHostNode *)(vHosts.at(vHostNo)->Data))->Ip.remove(index);
}

void vHostXML::setPort(int vHostNo, int val)
{
   if(vHosts.isempty())
     return;
   ((vHostNode *)(vHosts.at(vHostNo)->Data))->Port = val;
}

void vHostXML::setProtocol(int vHostNo, int val)
{
   if(vHosts.isempty())
     return;
   ((vHostNode *)(vHosts.at(vHostNo)->Data))->Protocol = val;
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

void vHostXML::setSsl_Privatekey(int vHostNo, const char * val)
{
   if(vHosts.isempty())
     return;
   setstr(((vHostNode *)(vHosts.at(vHostNo)->Data))->Ssl_Privatekey, val);
}

void vHostXML::setSsl_Certificate(int vHostNo, const char * val)
{
   if(vHosts.isempty())
     return;
   setstr(((vHostNode *)(vHosts.at(vHostNo)->Data))->Ssl_Certificate, val);
}

void vHostXML::setSsl_Password(int vHostNo, const char * val)
{
   if(vHosts.isempty())
     return;
   setstr(((vHostNode *)(vHosts.at(vHostNo)->Data))->Ssl_Password, val);
}

void vHostXML::setDocroot(int vHostNo, const char * val)
{
   if(vHosts.isempty())
     return;
   setstr(((vHostNode *)(vHosts.at(vHostNo)->Data))->Docroot, val);
}

void vHostXML::setSysfolder(int vHostNo, const char * val)
{
   if(vHosts.isempty())
     return;
   setstr(((vHostNode *)(vHosts.at(vHostNo)->Data))->Sysfolder, val);
}

void vHostXML::setAccesseslog(int vHostNo, const char * val)
{
   if(vHosts.isempty())
     return;
   setstr(((vHostNode *)(vHosts.at(vHostNo)->Data))->Accesseslog, val);
}

void vHostXML::setWarninglog(int vHostNo, const char * val)
{
   if(vHosts.isempty())
     return;
   setstr(((vHostNode *)(vHosts.at(vHostNo)->Data))->Warninglog, val);
}

int vHostXML::getPort(int vHostNo)
{
   if(vHosts.isempty())
     return 0;
   return ((vHostNode *)(vHosts.at(vHostNo)->Data))->Port;
}

int vHostXML::getProtocol(int vHostNo)
{
   if(vHosts.isempty())
     return 0;
   return ((vHostNode *)(vHosts.at(vHostNo)->Data))->Protocol;
}

const char * vHostXML::getSsl_Privatekey(int vHostNo)
{
   if(vHosts.isempty())
     return EMPTY;
   return ((vHostNode *)(vHosts.at(vHostNo)->Data))->Ssl_Privatekey;
}

const char * vHostXML::getSsl_Certificate(int vHostNo)
{
   if(vHosts.isempty())
     return EMPTY;
   return ((vHostNode *)(vHosts.at(vHostNo)->Data))->Ssl_Certificate;
}

const char * vHostXML::getSsl_Password(int vHostNo)
{
   if(vHosts.isempty())
     return EMPTY;
   return ((vHostNode *)(vHosts.at(vHostNo)->Data))->Ssl_Password;
}

const char * vHostXML::getDocroot(int vHostNo)
{
   if(vHosts.isempty())
     return EMPTY;
   return ((vHostNode *)(vHosts.at(vHostNo)->Data))->Docroot;
}

const char * vHostXML::getSysfolder(int vHostNo)
{
   if(vHosts.isempty())
     return EMPTY;
   return ((vHostNode *)(vHosts.at(vHostNo)->Data))->Sysfolder;
}

const char * vHostXML::getAccesseslog(int vHostNo)
{
   if(vHosts.isempty())
     return EMPTY;
   return ((vHostNode *)(vHosts.at(vHostNo)->Data))->Accesseslog;
}

const char * vHostXML::getWarninglog(int vHostNo)
{
   if(vHosts.isempty())
     return EMPTY;
   return ((vHostNode *)(vHosts.at(vHostNo)->Data))->Warninglog;
}

static inline void delstr(char * val)
{
   if(val != EMPTY)
     free(val);
}

void vHostXML::DeletevHostNode(vHostNode * Node)
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

