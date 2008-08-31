/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007, 2008 Free Software Foundation, Inc.
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

#ifndef CXMLPARSER_IN
#define CXMLPARSER_IN

#include "stdafx.h"
#include <include/base/file/file.h>
#include <include/base/mem_buff/mem_buff.h>
extern "C" 
{
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h> 
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
}
#include <string>

using namespace std;

/*!
 *Helper class to memorize a xpath result.
 */
class XmlXPathResult
{
public:
  XmlXPathResult(xmlXPathObjectPtr obj){xpathObj = obj;}
  ~XmlXPathResult(){if(xpathObj)xmlXPathFreeObject(xpathObj);}
  xmlXPathObjectPtr getObject(){return xpathObj;}
  xmlNodeSetPtr getNodeSet(){return (xpathObj ? xpathObj->nodesetval : NULL);}
private:
  xmlXPathObjectPtr xpathObj;
};

/*!
 *This class is used to open a .xml file and read information from it.
 */
class XmlParser
{
public:
	static bool startXML();
  static bool cleanXML();
	XmlParser();
	~XmlParser();
	xmlDocPtr getDoc();
	int open(const char* filename, bool useXpath = 0);
	int open(string const &filename, bool useXpath = 0){return open(filename.c_str(), useXpath);};
	int openMemBuf(MemBuf &, bool useXpath = 0);

	char *getValue(const char* field);
	char *getValue(string const &field){return getValue(field.c_str());};
	char *getAttr(const char* field, const char *attr);
	int setValue(const char* field, const char *value);
	int close();

	int save(const char *filename,int *nbytes = 0);
	int save(string const &filename,int *nbytes = 0){return save(filename.c_str(), nbytes);};
	int saveMemBuf(MemBuf &,int *nbytes = 0);

	void newfile(const char * root);
	void newfile(string const &root){newfile(root.c_str());};
	void addChild(const char * name, const char * value);
	void addChild(string const &name, string& value)
  {addChild(name.c_str(), value.c_str());};
	void addGroup(const char * name);
	void addGroup(string const &name)
  {addGroup(name.c_str());};
	void endGroup();

	void setAttr(const char * name, const char * value);

	void setAttr(string& name, string& value)
	{
		setAttr(name.c_str(), value.c_str());
	};
    
	void addLineFeed();
	time_t getLastModTime();
	
  XmlXPathResult* evaluateXpath(string & path){return evaluateXpath(path.c_str());}
  XmlXPathResult* evaluateXpath(const char*);
  bool isXpathEnabled(){return useXpath;}
private:
  xmlXPathContextPtr xpathCtx;
  bool useXpath;
	xmlDocPtr doc;
	string buffer;
	xmlNodePtr cur;
	xmlNodePtr prevCur;
	xmlNodePtr lastNode;
	time_t mtime;

};

#endif
