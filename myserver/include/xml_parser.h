/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007 The MyServer Team
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

#include "../stdafx.h"
#include "../include/file.h"
#include "../include/mem_buff.h"
extern "C" 
{
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h> 
}
#include <string>

using namespace std;

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
	int open(const char* filename);
	int open(string const &filename){return open(filename.c_str());};
	int openMemBuf(MemBuf &);
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
	
private:
	xmlDocPtr doc;
	string buffer;
	xmlNodePtr cur;
	xmlNodePtr prevCur;
	xmlNodePtr lastNode;
	time_t mtime;

};

#endif
