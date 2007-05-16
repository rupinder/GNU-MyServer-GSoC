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

#include "../stdafx.h"
#include "../include/mime_manager.h"
#include "../include/file.h"
#include "../include/files_utility.h"
#include "../include/stringutils.h"
#include "../include/xml_parser.h"

#include <string>

using namespace std;

/*!
 *Destroy the object.
 */
MimeRecord::~MimeRecord()
{
  clear();
}

/*!
 *Add a filter to the list of filters to apply on this MIME type.
 *Return zero if the filters was not added.
 */
int MimeRecord::addFilter(const char* n, int acceptDuplicate) 
{
  if(!acceptDuplicate)
  {
    list<string>::iterator i = filters.begin();
    for( ; i != filters.end() ;i++ )
    {
      if(!stringcmpi(*i, n))
        return 0;
    }
  }
  filters.push_back(n);
  return 1;
}

/*!
 *Copy constructor.
 */
MimeRecord::MimeRecord(MimeRecord& m)
{
  list<string>::iterator i = m.filters.begin();

  filters.clear(); 

  for( ; i != m.filters.end(); i++)
  {
    filters.push_back(*i);
  }
  extension.assign(m.extension); 
  mimeType.assign(m.mimeType);
  command = m.command;
  cmdName.assign(m.cmdName);
  cgiManager.assign(m.cgiManager);
  headerChecker.clone(m.headerChecker);
} 

/*!
  *Clear the used memory.
  */
void MimeRecord::clear()
{
  filters.clear();
  headerChecker.clear();
  extension.assign(""); 
  mimeType.assign(""); 
  cmdName.assign("");
  cgiManager.assign("");
}  

/*!
 *Get the name of the file opened by the class.
 */
const char *MimeManager::getFilename()
{
	if(filename == 0)
		return "";
	return filename->c_str();
}

/*!
 *Load the MIME types from a XML file. Returns the number of
 *MIME types loaded successfully.
 */
int MimeManager::loadXML(const char *fn)
{
	XmlParser parser;
	xmlNodePtr node;
  xmlDocPtr doc;
	int retSize;
  if(!fn)
    return -1;

	rwLock.writeLock();

	if(filename)
		delete filename;

	filename = new string(fn);

	if(data)
		delete data;

	data = new HashMap<string, MimeRecord*>();

	if(parser.open(fn))
	{
		rwLock.writeUnlock();
		return -1;
	}

	removeAllRecords();

	doc = parser.getDoc();
	node = doc->children->children;

	for(; node; node = node->next )
	{
		xmlNodePtr lcur = node->children;
    xmlAttr *attrs = lcur ? lcur->properties : 0;
		MimeRecord rc;
		if(xmlStrcmp(node->name, (const xmlChar *)"MIMETYPE"))
			continue;
		rc.clear();

    while(attrs)
    {
      if(!xmlStrcmp(attrs->name, (const xmlChar *)"defaultAction"))
      {
        if(attrs->children && attrs->children->content && 
           (!xmlStrcmp(attrs->children->content, (const xmlChar *)"ALLOW")))
          rc.headerChecker.setDefaultCmd(HttpHeaderChecker::ALLOW);
        else
          rc.headerChecker.setDefaultCmd(HttpHeaderChecker::DENY);
      }
      attrs = attrs->next;
    }

		while(lcur)
		{
			if(lcur->name && !xmlStrcmp(lcur->name, (const xmlChar *)"ACTION"))
			{
        HttpHeaderChecker::Rule r;
        xmlAttr *actionAttrs = lcur->properties; 

        if(lcur->children->content && lcur->children->content &&
           (!xmlStrcmp(lcur->children->content, (const xmlChar *)"DENY")))
          r.cmd = HttpHeaderChecker::DENY;
        else
          r.cmd = HttpHeaderChecker::ALLOW;

        while(actionAttrs)
        {
          if(!xmlStrcmp(actionAttrs->name, (const xmlChar *)"name"))
          {
            if(actionAttrs->children && actionAttrs->children->content)
              r.name.assign((const char*)actionAttrs->children->content);

          }

          if(!xmlStrcmp(actionAttrs->name, (const xmlChar *)"value"))
          {
            if(actionAttrs->children && actionAttrs->children->content)
              r.value.compile((const char*)actionAttrs->children->content, 
															REG_EXTENDED);
          }
          actionAttrs=actionAttrs->next;

        }
        rc.headerChecker.addRule(r);
			}

			if(lcur->name && !xmlStrcmp(lcur->name, (const xmlChar *)"EXT"))
			{
				if(lcur->children->content)
					rc.extension.assign((const char*)lcur->children->content);
			}

			if(lcur->name && !xmlStrcmp(lcur->name, (const xmlChar *)"MIME"))
			{
				if(lcur->children->content)
					rc.mimeType.assign((const char*)lcur->children->content);
			}

			if(lcur->name && !xmlStrcmp(lcur->name, (const xmlChar *)"FILTER"))
			{
				if(lcur->children->content)
          rc.addFilter((const char*)lcur->children->content);
			}

			if(lcur->name && !xmlStrcmp(lcur->name, (const xmlChar *)"CMD"))
			{
        if(lcur->children->content)
          rc.cmdName.assign((const char*)lcur->children->content);
        
        rc.command = CGI_CMD_SEND;

				if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"SEND"))
					rc.command = CGI_CMD_SEND;

				else if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"RUNCGI"))
					rc.command = CGI_CMD_RUNCGI;
			
        else if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"RUNMSCGI"))
					rc.command = CGI_CMD_RUNMSCGI;

				else if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"EXECUTE"))
					rc.command = CGI_CMD_EXECUTE;

				else if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"RUNISAPI"))
					rc.command = CGI_CMD_RUNISAPI;

				else if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"EXECUTEISAPI"))
					rc.command = CGI_CMD_EXECUTEISAPI;

				else if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"SENDLINK"))
					rc.command = CGI_CMD_SENDLINK;

				else if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,
											(const xmlChar *)"EXECUTEWINCGI"))
					rc.command = CGI_CMD_EXECUTEWINCGI;

				else if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"RUNFASTCGI"))
					rc.command = CGI_CMD_RUNFASTCGI;

				else if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,
											(const xmlChar *)"EXECUTEFASTCGI"))
					rc.command = CGI_CMD_EXECUTEFASTCGI;

				else if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"RUNSCGI"))
					rc.command = CGI_CMD_RUNSCGI;

				else if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"EXECUTESCGI"))
					rc.command = CGI_CMD_EXECUTESCGI;


        else if(lcur->children->content)
          rc.command = CGI_CMD_EXTERNAL;
			}

			if(lcur->name && !xmlStrcmp(lcur->name, (const xmlChar *)"MANAGER"))
			{
        /*! 
				 *If the specified manager is not NONE store its path in the record. 
				 */
				if(lcur->children->content && lstrcmpi((char*)lcur->children->content,
																							 "NONE"))
        {
          rc.cgiManager.assign((const char*)lcur->children->content);
        }
        else
        {
          rc.cgiManager.assign("");
        }
      }
			lcur = lcur->next;
		}

		if(addRecord(rc))
    {
      clean();
			rwLock.writeUnlock();
      return 0;
    }
	}
	parser.close();
  
  /*! Store the loaded status. */
  loaded = 1;

	retSize = data->size();

	rwLock.writeUnlock();

	return retSize;
}


/*!
 *Save the MIME types to a XML file.
 */
int MimeManager::saveXML(const char *filename)
{
	File f;
	u_long nbw;

	rwLock.writeLock();

	FilesUtility::deleteFile(filename);
	HashMap<string, MimeRecord*>::Iterator it = data->begin();
	HashMap<string, MimeRecord*>::Iterator end = data->end();

	f.openFile(filename, File::MYSERVER_OPEN_WRITE|File::MYSERVER_OPEN_ALWAYS);
	f.writeToFile("<?xml version=\"1.0\"?>\r\n",23,&nbw);
	f.writeToFile("<MIMETYPES>\r\n", 13, &nbw);

	for(; it != end; it++)
	{
    MimeRecord *rc = *it;
		char command[16];
    if(!rc)
       break;
		f.writeToFile("\r\n<MIMETYPE>\r\n<EXT>",19,&nbw);
		f.writeToFile(rc->extension.c_str(),(u_long)rc->extension.length(),&nbw);
		f.writeToFile("</EXT>\r\n<MIME>",14,&nbw);
		f.writeToFile(rc->mimeType.c_str(),(u_long)rc->mimeType.length(),&nbw);
		f.writeToFile("</MIME>\r\n<CMD>",14,&nbw);
		if(rc->command == CGI_CMD_SEND)
			strcpy(command, "SEND");
		else if(rc->command == CGI_CMD_RUNCGI)
			strcpy(command, "RUNCGI");
		else if(rc->command == CGI_CMD_RUNMSCGI)
			strcpy(command, "RUNMSCGI");
		else if(rc->command == CGI_CMD_EXECUTE)
			strcpy(command, "EXECUTE");
		else if(rc->command == CGI_CMD_SENDLINK)
			strcpy(command, "SENDLINK");
		else if(rc->command == CGI_CMD_RUNISAPI)
			strcpy(command, "RUNISAPI");
		else if(rc->command == CGI_CMD_EXECUTEISAPI)
			strcpy(command, "EXECUTEISAPI");
		else if(rc->command == CGI_CMD_EXECUTEWINCGI)
			strcpy(command, "EXECUTEWINCGI");
		else if(rc->command == CGI_CMD_RUNFASTCGI)
			strcpy(command, "RUNFASTCGI");	
		else if(rc->command == CGI_CMD_EXECUTEFASTCGI)
			strcpy(command, "EXECUTEFASTCGI");	
		else if(rc->command == CGI_CMD_RUNSCGI)
			strcpy(command, "RUNSCGI");	
		else if(rc->command == CGI_CMD_EXECUTESCGI)
			strcpy(command, "EXECUTESCGI");	
		else if(rc->command == CGI_CMD_EXTERNAL)
			strcpy(command, rc->cmdName.c_str());
	
		f.writeToFile(command,(u_long)strlen(command),&nbw);

		f.writeToFile("</CMD>\r\n<MANAGER>",17,&nbw);
		if(rc->cgiManager.length())
			f.writeToFile(rc->cgiManager.c_str(), 
                    (u_long)rc->cgiManager.length(), &nbw);
		else
			f.writeToFile("NONE", 4, &nbw);
		f.writeToFile("</MANAGER>\r\n</MIMETYPE>\r\n", 25, &nbw);	
	}
	f.writeToFile("\r\n</MIMETYPES>", 14, &nbw);
	f.closeFile();

	rwLock.writeUnlock();
	return 1;
}

/*!
 *This function returns the type of action to do for handle this file type.
 *Passing a file extension ext this function fills the strings dest and dest2
 *respectly with the MIME type description and if there are the path to the 
 *CGI manager.
 */
int MimeManager::getMIME(char* ext,char *dest,char **dest2)
{
  MimeRecord* mr;

	rwLock.readLock();

	mr = data ? data->get(ext) : 0;
  if(mr)
  {
    if(dest)
      strcpy(dest, mr->mimeType.c_str());

    if(dest2)
		{
      if(mr->cgiManager.length())
      {
        *dest2 = new char[mr->cgiManager.length() + 1];
        if(*dest2 == 0)
				{
					rwLock.readUnlock();
          return 0;
				}
        strcpy(*dest2, mr->cgiManager.c_str());
        }
				else
					*dest2 = 0;
    }
		rwLock.readUnlock();
    return mr->command;
  }
	rwLock.readUnlock();

	/*!
   *If the ext is not registered send the file as it is.
   */
	return CGI_CMD_SEND;
}

/*!
 *This function returns the type of action to do for handle this file type.
 *Passing a file extension ext this function fills the strings dest and dest2
 *respectly with the MIME type description and if there are the path to the 
 *CGI manager.
 */
int MimeManager::getMIME(string& ext,string& dest,string& dest2)
{
  MimeRecord *mr;

	rwLock.readLock();
	mr = data ? data->get(ext.c_str()): 0;
	if(mr)
	{
		if(!stringcmpi(mr->extension, ext.c_str()))
		{
			dest.assign(mr->mimeType.c_str());

      if(mr->cgiManager.length())
      {
        dest2.assign(mr->cgiManager.c_str());
      }
      else
        dest2.assign("");
    
			rwLock.readUnlock();
			return mr->command;
		}
	}
	rwLock.readUnlock();
	/*!
   *If the ext is not registered send the file as it is.
   */
	return CGI_CMD_SEND;
}

/*!
 *Get a MIME type by the position of the record in the list.
 */
int MimeManager::getMIME(int id, char* ext, char *dest, char **dest2)
{
  MimeRecord *mr;
	HashMap<string, MimeRecord*>::Iterator it;
	HashMap<string, MimeRecord*>::Iterator end;

	if(data == 0)
		return 0;

	rwLock.readLock();


	it = data->begin();
	end = data->end();
	if(id > data->size() || id < 0)
  {
		rwLock.readUnlock();
    return CGI_CMD_SEND;   
  }
	/*! FIXME: find a O(1) solution.  */
	while(id-- && it != end)it++;

  mr = *it;

  if(!mr)
	{
		rwLock.readUnlock();
    return CGI_CMD_SEND;
	}

  if(ext)
    strcpy(ext,mr->extension.c_str());
  if(dest)
    strcpy(dest,mr->mimeType.c_str());
  if(dest2)
  {
    if(mr->cgiManager.length())
    {
      *dest2 = new char[mr->cgiManager.length() + 1];
      if(*dest2 == 0)
			{
				rwLock.readUnlock();
        return 0;
			}
      strcpy(*dest2, mr->cgiManager.c_str());
    }
    else
      dest2 = 0;
  }

	rwLock.readUnlock();
  return mr->command;

}

/*!
 *Get a MIME type by the position of the record in the list.
 */
int MimeManager::getMIME(int id,string& ext,string& dest,string& dest2)
{
  MimeRecord *mr;

	rwLock.readLock();

  if(!data || id > data->size() || id < 0)
  {
		rwLock.readUnlock();
    return CGI_CMD_SEND;   
  }

  mr=data->get(ext.c_str());
  if(mr)
  {
    ext.assign(mr->extension);
    dest.assign(mr->mimeType);
			
    if(mr->cgiManager.length())
      dest2.assign(mr->cgiManager.c_str());
    else
      dest2.assign("");
			
		rwLock.readUnlock();
    return mr->command;
  }

	rwLock.readUnlock();
	/*!
   *If the ext is not registered send the file as it is.
   */
	return CGI_CMD_SEND;
}


/*!
 *Destroy the object.
 */
MimeManager::~MimeManager()
{
  clean();
}

/*!
 *Clean the memory allocated by the structure.
 */
void MimeManager::clean()
{
  if(loaded)
  {
		rwLock.writeLock();
    loaded = 0;
		if(filename) 
			delete filename;
    filename = 0;
    removeAllRecords();
		delete data;
		data = 0;
		rwLock.writeUnlock();
  }
}

/*!
 *Constructor of the class.
 */
MimeManager::MimeManager() : rwLock(100000)
{
	data = 0;
  filename = 0;
  loaded = 0;
}

/*!
 *Add a new record. Returns zero on success.
 */
int MimeManager::addRecord(MimeRecord& mr)
{
	/*!
   *If the MIME type already exists remove it.
   */
  MimeRecord *nmr = 0;
  try
  {
		MimeRecord *old;
		if(getRecord(mr.extension))
      removeRecord(mr.extension);
    nmr = new MimeRecord(mr);
    if(!nmr)	
      return 1;
		string keyStr(nmr->extension);
    old = data->put(keyStr, nmr);
		if(old)
			delete old;
  }
  catch(...)
  {
    if(nmr)
      delete nmr;
    return 1;
  };
  
  return 0;
}

/*!
 *Remove a record by the extension of the MIME type.
 */
void MimeManager::removeRecord(const string& ext)
{
  MimeRecord *rec = data->remove(ext.c_str());
  if(rec)
    delete rec;
}

/*!
 *Remove all records from the linked list.
 */
void MimeManager::removeAllRecords()
{
	HashMap<string, MimeRecord*>::Iterator it = data->begin();
  for(; it != data->end(); it++)
  {
    MimeRecord *rec = *it;
    if(rec)
      delete rec;
  }

	data->clear();
}

/*!
 *Get a pointer to an existing record passing its extension.
 */
MimeRecord *MimeManager::getRecord(string const &ext)
{
  MimeRecord* mr;

	rwLock.readLock();

	mr = data ? data->get(ext.c_str()) : 0;

	rwLock.readUnlock();

	return mr;
}

/*!
 *Returns the number of MIME types loaded.
 */
u_long MimeManager::getNumMIMELoaded()
{
	u_long ret;

	rwLock.readLock();

	ret = data ? data->size() : 0;

	rwLock.readUnlock();

	return ret;
}

/*!
 *Check if the MIME manager is loaded.
 */
int MimeManager::isLoaded()
{
  return loaded;
}
