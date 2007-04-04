/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006 The MyServer Team
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

#include "../include/xml_parser.h"
#include "../include/utility.h"
#include "../include/files_utility.h"

extern "C" {
#include <string.h>
}

#ifndef lstrlen
#define lstrlen strlen
#endif

#ifdef WIN32
/*!
 *Libxml2.lib is the dynamic version of the libxml2 while libxml2_a.lib is 
 *static.
 *With static version use the linker options: /NODEFAULTLIB:LIBCMT
 * /NODEFAULTLIB:LIBCMTD.
 */

#ifdef LIXML_STATICALLY_LINKED
#pragma comment (lib,"libxml2_a.lib")
#else
#pragma comment (lib,"libxml2.lib")
#endif

#endif

/* Internal call back functions for saveMemBuf.  */
static int MemBufWriteCallback(void * context, const char * buffer, int len)
{
	((MemBuf *)context)->addBuffer((const void *)buffer, len);
	return len;
}
static int MemBufCloseCallback(void * context)
{
	return 0;
}

/*!
 *Initialize the libxml2 library.
 */
int XmlParser::startXML()
{
	xmlInitParser();
	return 1;
}

/*!
 *Cleanup the libxml2 library.
 */
int XmlParser::cleanXML()
{
	xmlCleanupParser();
	return 1;
}

/*!
 *With the open function we open a file and store it in memory.
 *Return nonzero on errors.
 */
int XmlParser::open(const char* filename)
{
	cur=0;
	if(!FilesUtility::fileExists(filename))
		return -1;
	if(doc!=0)
		close();
  doc = xmlParseFile(filename);
  if(doc == 0)
    return -1;
	cur = xmlDocGetRootElement(doc);
	if(!cur)
	{
		close();
		return -1;
	}
  mtime = FilesUtility::getLastModTime(filename);
  if(mtime == static_cast<time_t>(-1))
  {
		close();
		return -1;
  }
     
	return 0;
}

/*!
 *Get the last modification time for the parsed file.
 */
time_t XmlParser::getLastModTime()
{
  return mtime;
}

/*!
 *Read the xml data from a char array
 *Return nonzero on errors.
 */
int XmlParser::openMemBuf(MemBuf & memory)
{
  mtime=0;
	cur=0;
	if(memory.getLength() == 0)
		return -1;
	if(doc==0)
		doc = xmlParseMemory((const char * )memory.getBuffer(),
		                     memory.getLength());
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
 *Constructor of the XmlParser class.
 */
XmlParser::XmlParser()
{
  doc = 0;
	cur = 0;
	prevCur = 0;
	lastNode = 0;
}

/*!
 *Destroy the XmlParser object.
 */
XmlParser::~XmlParser()
{
	close();
}

/*!
 *Return the xml Document.
 */
xmlDocPtr XmlParser::getDoc()
{
	return doc;
}

/*!
 *Get the value of the vName root children element.
 */
char *XmlParser::getValue(const char* vName)
{
  char *ret = 0;
	xmlNodePtr lcur;
  cur = xmlDocGetRootElement(doc);
	if(!cur)
		return 0;

	lcur = cur->xmlChildrenNode;
	buffer[0] = '\0';
	while(lcur)
	{
		if(!xmlStrcmp(lcur->name, (const xmlChar *)vName))
		{
			lastNode = lcur;
			if(lcur->children->content)
      {
        int outlen = 250;
        int inlen = strlen((const char*)lcur->children->content);
        if(UTF8Toisolat1((unsigned char*)buffer, &outlen, 
                         (unsigned char*)lcur->children->content, &inlen)>= 0)
          buffer[outlen] = '\0';
        else
          strncpy(buffer, (char*)lcur->children->content, 250);
        ret = buffer;
      }
			break;
		}
		lcur = lcur->next;
	}
	
	return ret;
}

/*!
 *Set the value of the vName root children element.
 *Returns nonzero on errors.
 */
int XmlParser::setValue(char* vName,char *value)
{
	xmlNodePtr lcur=cur->xmlChildrenNode;
	buffer[0]='\0';
	while(lcur)
	{
		if(!xmlStrcmp(lcur->name, (const xmlChar *)vName))
		{
			lastNode = lcur;
			if(lcur->children->content)
				strcpy((char*)lcur->children->content, value);
			return 0;
		}
		lcur=lcur->next;
	}
	return 1;
}

/*!
 *Get the attribute attr for the node field.
 */
char *XmlParser::getAttr(char* field,char *attr)
{
	xmlNodePtr lcur=cur->xmlChildrenNode;
	buffer[0]='\0';
	while(lcur)
	{
		if(!xmlStrcmp(lcur->name, (const xmlChar *)field))
		{
			lastNode = lcur;
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
 *free the memory used by the class.
 */
int XmlParser::close()
{
  if(doc)
    xmlFreeDoc(doc);
	doc=0;
	cur=0;
	prevCur=0;
	lastNode=0;
	return 0;
}

/*!
 *Save the XML tree to a file
 *Returns nonzero on errors
 *If no errors nbytes[optional] will cointain the number 
 *of bytes written.
 */
int XmlParser::save(const char *filename,int *nbytes)
{
  int err = xmlSaveFile(filename,doc);
  if(nbytes)
    *nbytes = err;

  return err;
}

/*!
 *Save the XML tree to memory
 *Returns nonzero on errors
 *If no errors nbytes[optional] will cointain the number 
 *of bytes written.
 */
int XmlParser::saveMemBuf(MemBuf & memory,int *nbytes)
{
  /*! Initialize the callback struct. */
  xmlOutputBufferPtr callback;
  callback = xmlOutputBufferCreateIO(MemBufWriteCallback,
                                     MemBufCloseCallback,
                                     (void *)&memory,
                                     NULL);
  
  /*! Clear out the buffer. */
  memory.free(); 
  
  /*! Let libxml2 fill the MemBuf class with our interal callbacks. */
  int err = xmlSaveFileTo(callback, doc, NULL);
  if(nbytes)
    *nbytes = err;
  
  return err;
}

/*!
 *Start a new XML tree for a new file.
 *Returns nothing.
 *root is the root element entry.
 */
void XmlParser::newfile(const char * root)
{
   if(doc != 0)
     close();
   doc = xmlNewDoc((const xmlChar*)"1.0");
   cur = xmlNewDocNode(doc, NULL, (const xmlChar*)root, NULL);
   xmlDocSetRootElement(doc, cur);
   
   addLineFeed();
   addLineFeed();
}

/*!
 *Adds a new child entry.
 *Returns nothing.
 *name is the child name and value is its value.
 */
void XmlParser::addChild(const char * name, const char * value)
{
   lastNode = xmlNewTextChild(cur, NULL, (const xmlChar*)name,
                               (const xmlChar*)value);
   addLineFeed();
}

/*!
 *Starts a new sub group (only one level for now).
 *Returns nothing.
 *name is the name of the sub group.
 */
void XmlParser::addGroup(const char * name)
{
  if(prevCur == 0)
  {
    prevCur = cur;
    cur = xmlNewTextChild(cur, NULL, (const xmlChar*)name, NULL);
    lastNode = cur;
     
    addLineFeed();
  }
}

/*!
 *Ends the sub group if any (only one level for now).
 *Returns nothing.
 */
void XmlParser::endGroup()
{
  if(prevCur != 0)
  {
    cur = prevCur;
    prevCur = 0;
     
    addLineFeed();
    addLineFeed();
  }
}

/*!
 *Sets or resets an Attribute
 *Returns nothing.
 *Uses last node entry, name is the name and value is the value
 */
void XmlParser::setAttr(const char * name, const char * value)
{
	if(lastNode == 0)
		return;
	xmlSetProp(lastNode, (const xmlChar*)name, (const xmlChar*)value);
}
/*!
 * Adds a line feed to the xml data
 */
void XmlParser::addLineFeed()
{
#ifdef WIN32
   xmlNodePtr endline = xmlNewDocText(doc, (const xmlChar *)"\r\n");
#else
   xmlNodePtr endline = xmlNewDocText(doc, (const xmlChar *)"\n");
#endif
   xmlAddChild(cur, endline);
}
