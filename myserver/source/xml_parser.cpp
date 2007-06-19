/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006, 2007 The MyServer Team
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
 * Libxml2.lib is the dynamic version of the libxml2 library
 * libxml2_a.lib is the static version of the libxml2 library
 *
 * For the static version, please use the following linker options:
 * /NODEFAULTLIB:LIBCMT
 * /NODEFAULTLIB:LIBCMTD
 * 
 */

#ifdef LIXML_STATICALLY_LINKED
#pragma comment (lib,"libxml2_a.lib")
#else
#pragma comment (lib,"libxml2.lib")
#endif

#endif

/**
 * Internal call back functions for saveMemBuf
 * @param context Context
 * @param buffer Buffer
 * @parm length Length
 * @return Returns the length
 */
static int MemBufWriteCallback(void * context, const char * buffer, int len)
{
	((MemBuf *)context)->addBuffer((const void *)buffer, len);
	return len;
}


/**
 * Memory Buffer Close Callback
 * @param context Context
 * @return Returns 0
 */
static int MemBufCloseCallback(void * context)
{
	return 0;
}


/**
 * Initializes the libxml2 library
 * Calls xmlInitParser()
 * @return Returns true 
 */
bool XmlParser::startXML()
{
	xmlInitParser();
	return 1;
}


/**
 * Cleans up the libxml2 library.
 * @return Returns true
 */
bool XmlParser::cleanXML()
{
	xmlCleanupParser();
	return 1;
}


/**
 * Opens a files and stores it in memory.
 * @param filename The filename
 * @return Returns 0 on success, non zero values on failure
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


/**
 * Gets the last modification time of the file
 * @return Returns last modification time
 */
time_t XmlParser::getLastModTime()
{
	return mtime;
}

/**
 * Read the XML data from a char array
 * @param memory Memory Buffer
 * @return Returns 0 on succes, non 0 on failure
 */
int XmlParser::openMemBuf(MemBuf & memory)
{
	mtime=0;
	cur=0;
	
	if(memory.getLength() == 0)
		return -1;
	
	if(doc==0)
	{
		doc = xmlParseMemory((const char * )memory.getBuffer(), 
				             memory.getLength());
	}
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


/**
 * Constructor of the XmlParser class
 */
XmlParser::XmlParser()
{
	doc = 0;
	cur = 0;
	prevCur = 0;
	lastNode = 0;
}


/**
 * Destructor of the XmlParser class
 * Destroys the XmlParser object
 */
XmlParser::~XmlParser()
{
	close();
}

/**
 * Returns the XML document
 * @return Returns XML document
 */
xmlDocPtr XmlParser::getDoc()
{
	return doc;
}

/**
 * Gets the value of the vName root child element.
 * @param vName vName of the root child elment
 * @return Returns the value of the vName
 */
char *XmlParser::getValue(const char* vName)
{
	char *ret = 0;
	xmlNodePtr lcur;
	cur = xmlDocGetRootElement(doc);
	
	if(!cur)
		return 0;
	
	lcur = cur->xmlChildrenNode;
	buffer.assign("");
	
	while(lcur)
	{
		if(!xmlStrcmp(lcur->name, (const xmlChar *)vName))
		{
			lastNode = lcur;
			
			if(lcur->children->content)
			{
				int inlen = strlen((const char*)lcur->children->content);
				int outlen = inlen * 2;
				char* tmpBuff = new char[outlen];
				if(UTF8Toisolat1((unsigned char*)tmpBuff, &outlen, 
                         		 (unsigned char*)lcur->children->content, &inlen) >= 0)
				{
					tmpBuff[outlen] = '\0';
					buffer.assign(tmpBuff);
				}
				else
					buffer.assign((char*)lcur->children->content);

				delete [] tmpBuff;

				ret = (char*)buffer.c_str();
			}
			
			break;
		}
		
		lcur = lcur->next;
	}
	
	return ret;
}


/**
 * Sets the value of the vName root child element
 * @param vName
 * @param value
 * @return Returns 0 on success, non zero on failures
 */
int XmlParser::setValue(const char* vName, const char *value)
{
	xmlNodePtr lcur = cur->xmlChildrenNode;
	buffer.assign("");
	
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


/**
 * Gets the attribute for the node field.
 * @param field Field
 * @param attr Attribute
 * @return
 */
char *XmlParser::getAttr(const char* field, const char *attr)
{
	xmlNodePtr lcur = cur->xmlChildrenNode;
	buffer.assign("");
	
	while(lcur)
	{
		if(!xmlStrcmp(lcur->name, (const xmlChar *)field))
		{
			lastNode = lcur;
			xmlAttr *attrs =  lcur->properties;
			
			while(attrs)
			{
				if(!xmlStrcmp(attrs->name, (const xmlChar *)attr))
				{
					return (char*)attrs->children->content;
				}
				
				attrs=attrs->next;
			}
		}
		
		lcur=lcur->next;
	}
	
	return 0;
}


/**
 * Frees the memory, use by the XmlParser class
 */
int XmlParser::close()
{
	if(doc)
	{
		xmlFreeDoc(doc);
	}
	
	doc=0;
	cur=0;
	prevCur=0;
	lastNode=0;
	
	return 0;
}

/**
 * Saves the XML tree into a file
 * If no errors occur nbytes[optional] will contain
 * the amount of written bytes
 * @param filename Filename
 * @param nbytes Amount of bytes
 * @return Returns 0 on success, non 0 on failures
 */
int XmlParser::save(const char *filename,int *nbytes)
{
	int err=xmlSaveFile(filename,doc);
	
	if(nbytes)
		*nbytes = err;
	
	return err;
}


/**
 * Saves the XML tree into memory
 * If no errors occur nbytes[optional] will contain
 * the amount of written bytes
 * @param memory Memory Buffer
 * @param nbytes Amount of bytes
 * @return Returns 0 on success, non 0 on failures
 */
int XmlParser::saveMemBuf(MemBuf & memory,int *nbytes)
{
	/*! Initialize the callback struct. */
	xmlOutputBufferPtr callback;
	callback = xmlOutputBufferCreateIO(MemBufWriteCallback,
                                       MemBufCloseCallback,
                                       (void *)&memory,
                                       NULL);
	
	/*! Clear the buffer */
	memory.free(); 
	
	/*! Let libxml2 fill the MemBuf class with our interal callbacks. */
	int err = xmlSaveFileTo(callback, doc, NULL);
  
	if(nbytes)
		*nbytes = err;
	
	return err;
}


/**
 * Starts a new XML tree for a new file
 * @param root roote elment entry
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


/**
 * Adds a new child element entry
 * @param name Child name
 * @param value Value of the child
 */
void XmlParser::addChild(const char * name, const char * value)
{
	lastNode = xmlNewTextChild(cur, NULL, (const xmlChar*)name,
                               (const xmlChar*)value);
	
	addLineFeed();
}


/**
 * Starts a new sub group
 * Only one level for now
 * @param name Name of the sub group
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


/**
 * Ends the sub group, if any
 * Only one level for now
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


/**
 * Sets an attribute, using the last node entry
 * @param name Name
 * @param value Value
 */
void XmlParser::setAttr(const char * name, const char * value)
{
	if(lastNode == 0)
		return;
	
	xmlSetProp(lastNode, (const xmlChar*)name, (const xmlChar*)value);
}


/**
 * Adds a line feed to the XML data
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
