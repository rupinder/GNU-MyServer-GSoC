/*
MyServer
Copyright (C) 2002, 2003, 2004 The MyServer Team
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
#include "../include/stringutils.h"
#include "../include/xml_parser.h"

#include <string>

using namespace std;

/*!
 *Source code to manage the MIME types in MyServer.
 *MIME types are recorded in a static buffer "data", that is a strings array.
 *Every MIME type is described by three strings:
 *html,text/html,SEND NONE;
 *1)its extension(for example HTML)
 *2)its MIME description(for example text/html)
 *3)simple script to describe the action to do for handle the files of this type.
 *The file is ended by a '#' character.
 */
int MimeManager::load(const char *fn)
{
	char *buffer;
	int ret;
  char commandString[16];
	File f;
	u_long nbw, fs;
  u_long nc=0;
	MimeRecord record;  
  if(fn == 0)
    return -1;
	
	if(filename)
		delete filename;
	filename = new string(fn);

	numMimeTypesLoaded=0;
	if(data)
		delete data;
	data = new HashMap<string, MimeRecord*>();
	ret=f.openFile(filename->c_str(), FILE_OPEN_READ|FILE_OPEN_IFEXISTS);
	if(ret)
		return 0;
	fs=f.getFileSize();
	buffer=new char[fs+1];
	if(!buffer)
		return 0;

	f.readFromFile(buffer,fs,&nbw);
	f.closeFile();
	for(;;)
	{
		/*!
     *Do not consider the \r \n and space characters.
     */
		while((buffer[nc]==' ') || (buffer[nc]=='\r') ||(buffer[nc]=='\n'))
			nc++;
		/*!
     *If is reached the # character or the end of the string end the loop.
     */
		if(buffer[nc]=='\0'||buffer[nc]=='#'||nc==nbw)
			break;
		while(buffer[nc]!=',')
		{
      char db[2];
      db[1]='\0';
			if(isalpha(buffer[nc])||isdigit(buffer[nc]))
				if((buffer[nc]!='\n')&&(buffer[nc]!='\r')&&(buffer[nc]!=' '))
        {
          db[0]=buffer[nc];
					record.extension.append((const char*)db);
        }
			nc++;
		}
		nc++;
		while(buffer[nc]!=',')
		{
      char db[2];
      db[1]='\0';
      
			if((buffer[nc]!='\n')&&(buffer[nc]!='\r')&&(buffer[nc]!=' '))
      {
        db[0]=buffer[nc];
				record.mime_type.append((const char*) db);
      }
			nc++;
		}
		nc++;
		/*!
     *Save the action to do with this type of files.
     */
		memset(commandString, 0, 16);

		while(buffer[nc]!=' ')
		{
			if((buffer[nc]!='\n')&&(buffer[nc]!='\r'))
				commandString[strlen(commandString)]=buffer[nc];
			nc++;
			/*!
       *Parse all the possible actions.
       *By default send the file as it is.
       */
			record.command=CGI_CMD_SEND;
			if(!lstrcmpi(commandString, "SEND"))
				record.command=CGI_CMD_SEND;
			else if(!lstrcmpi(commandString, "RUNCGI"))
				record.command=CGI_CMD_RUNCGI;
			else if(!lstrcmpi(commandString, "RUNMSCGI"))
				record.command=CGI_CMD_RUNMSCGI;
			else if(!lstrcmpi(commandString, "EXECUTE"))
				record.command=CGI_CMD_EXECUTE;
			else if(!lstrcmpi(commandString, "RUNISAPI"))
				record.command=CGI_CMD_RUNISAPI;
			else if(!lstrcmpi(commandString, "EXECUTEISAPI"))
				record.command=CGI_CMD_EXECUTEISAPI;
			else if(!lstrcmpi(commandString, "SENDLINK"))
				record.command=CGI_CMD_SENDLINK;
			else if(!lstrcmpi(commandString, "EXECUTEWINCGI"))
				record.command=CGI_CMD_EXECUTEWINCGI;
			else if(!lstrcmpi(commandString, "RUNFASTCGI"))
				record.command=CGI_CMD_RUNFASTCGI;
			else if(!lstrcmpi(commandString, "EXECUTEFASTCGI"))
				record.command=CGI_CMD_EXECUTEFASTCGI;
			else
        record.command=CGI_CMD_EXTERNAL;
      record.cmd_name.assign(commandString);
		}
		nc++;
    record.cgi_manager.assign("");
		while(buffer[nc]!=';')
		{
      char db[2];
      db[1]='\0';
			if((buffer[nc]!='\n')&&(buffer[nc]!='\r'))
      {
        db[0]=buffer[nc];
				record.cgi_manager.append((const char*)db);
      }
			nc++;
			/*!
       *If the CGI manager path is "NONE" then set the cgi_manager element to
       *be empty.
       */
			if(!stringcmpi(record.cgi_manager,"NONE"))
				record.cgi_manager.assign("");
			
		}
		if(addRecord(record))
    {
      clean();
      return 0;
    }
		nc++;
	}

  /*! Store the loaded status. */
  loaded = 1;

	delete [] buffer;
	return data->size();

}

/*!
 *Destroy the object.
 */
MimeRecord::~MimeRecord()
{
  clear();
}

/*!
 *Copy constructor.
 */
MimeRecord::MimeRecord(MimeRecord& m)
{
  list<string>::iterator i=m.filters.begin();

  filters.clear(); 

  for( ; i != m.filters.end(); i++)
  {
    filters.push_back(*i);
  }
  extension.assign(m.extension); 
  mime_type.assign(m.mime_type);
  command=m.command;
  cmd_name.assign(m.cmd_name);
  cgi_manager.assign(m.cgi_manager);
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
  mime_type.assign(""); 
  cmd_name.assign("");
  cgi_manager.assign("");
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
  if(!fn)
    return -1;
	if(filename)
		delete filename;

	filename = new string(fn);
	if(data)
		delete data;
	data = new  HashMap<string, MimeRecord*>();
	if(parser.open(fn))
	{
		return -1;
	}
	doc = parser.getDoc();
	node=doc->children->children;
	for(;node;node=node->next )
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
      attrs=attrs->next;
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
              r.value.compile((const char*)actionAttrs->children->content, REG_EXTENDED);
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
					rc.mime_type.assign((const char*)lcur->children->content);
			}

			if(lcur->name && !xmlStrcmp(lcur->name, (const xmlChar *)"FILTER"))
			{
				if(lcur->children->content)
          rc.addFilter((const char*)lcur->children->content);
			}

			if(lcur->name && !xmlStrcmp(lcur->name, (const xmlChar *)"CMD"))
			{
        if(lcur->children->content)
          rc.cmd_name.assign((const char*)lcur->children->content);
        
        rc.command=CGI_CMD_SEND;

				if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"SEND"))
					rc.command=CGI_CMD_SEND;

				else if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"RUNCGI"))
					rc.command=CGI_CMD_RUNCGI;
			
        else if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"RUNMSCGI"))
					rc.command=CGI_CMD_RUNMSCGI;

				else if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"EXECUTE"))
					rc.command=CGI_CMD_EXECUTE;

				else if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"RUNISAPI"))
					rc.command=CGI_CMD_RUNISAPI;

				else if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"EXECUTEISAPI"))
					rc.command=CGI_CMD_EXECUTEISAPI;

				else if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"SENDLINK"))
					rc.command=CGI_CMD_SENDLINK;

				else if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"EXECUTEWINCGI"))
					rc.command=CGI_CMD_EXECUTEWINCGI;

				else if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"RUNFASTCGI"))
					rc.command=CGI_CMD_RUNFASTCGI;

				else if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"EXECUTEFASTCGI"))
					rc.command=CGI_CMD_EXECUTEFASTCGI;

        else if(lcur->children->content)
          rc.command=CGI_CMD_EXTERNAL;
			}

			if(lcur->name && !xmlStrcmp(lcur->name, (const xmlChar *)"MANAGER"))
			{
        /*! If the specified manager is not NONE store its path in the record. */
				if(lcur->children->content && lstrcmpi((char*)lcur->children->content,"NONE"))
        {
          rc.cgi_manager.assign((const char*)lcur->children->content);
        }
        else
        {
          rc.cgi_manager.assign("");
        }
      }
			lcur=lcur->next;
		}
		if(addRecord(rc))
    {
      clean();
      return 0;
    }
	}
	parser.close();
  
  /*! Store the loaded status. */
  loaded = 1;

	return data->size();
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
 *Save the MIME types to a XML file.
 */
int MimeManager::saveXML(const char *filename)
{
	File::deleteFile(filename);
	File f;
	u_long nbw;
	HashMap<string, MimeRecord*>::Iterator it = data->begin();
	HashMap<string, MimeRecord*>::Iterator end = data->end();

	f.openFile(filename, FILE_OPEN_WRITE|FILE_OPEN_ALWAYS);
	f.writeToFile("<?xml version=\"1.0\"?>\r\n",23,&nbw);
	f.writeToFile("<MIMETYPES>\r\n",13,&nbw);

	for(; it!=end; it++)
	{
    MimeRecord *rc = *it;
		char command[16];
    if(!rc)
       break;
		f.writeToFile("\r\n<MIMETYPE>\r\n<EXT>",19,&nbw);
		f.writeToFile(rc->extension.c_str(),(u_long)rc->extension.length(),&nbw);
		f.writeToFile("</EXT>\r\n<MIME>",14,&nbw);
		f.writeToFile(rc->mime_type.c_str(),(u_long)rc->mime_type.length(),&nbw);
		f.writeToFile("</MIME>\r\n<CMD>",14,&nbw);
		if(rc->command==CGI_CMD_SEND)
			strcpy(command,"SEND");
		else if(rc->command==CGI_CMD_RUNCGI)
			strcpy(command,"RUNCGI");
		else if(rc->command==CGI_CMD_RUNMSCGI)
			strcpy(command,"RUNMSCGI");
		else if(rc->command==CGI_CMD_EXECUTE)
			strcpy(command,"EXECUTE");
		else if(rc->command==CGI_CMD_SENDLINK)
			strcpy(command,"SENDLINK");
		else if(rc->command==CGI_CMD_RUNISAPI)
			strcpy(command,"RUNISAPI");
		else if(rc->command==CGI_CMD_EXECUTEISAPI)
			strcpy(command,"EXECUTEISAPI");
		else if(rc->command==CGI_CMD_EXECUTEWINCGI)
			strcpy(command,"EXECUTEWINCGI");
		else if(rc->command==CGI_CMD_RUNFASTCGI)
			strcpy(command,"RUNFASTCGI");	
		else if(rc->command==CGI_CMD_EXECUTEFASTCGI)
			strcpy(command,"EXECUTEFASTCGI");	
		else if(rc->command==CGI_CMD_EXTERNAL)
			strcpy(command, rc->cmd_name.c_str());
	
		f.writeToFile(command,(u_long)strlen(command),&nbw);

		f.writeToFile("</CMD>\r\n<MANAGER>",17,&nbw);
		if(rc->cgi_manager.length())
			f.writeToFile(rc->cgi_manager.c_str(), 
                    (u_long)rc->cgi_manager.length(),&nbw);
		else
			f.writeToFile("NONE",4,&nbw);
		f.writeToFile("</MANAGER>\r\n</MIMETYPE>\r\n",25,&nbw);	
	}
	f.writeToFile("\r\n</MIMETYPES>",14,&nbw);
	f.closeFile();
	return 1;
}

/*!
 *Save the MIME types to a file.
 */
int MimeManager::save(const char *filename)
{
	File f;
	u_long nbw;
	HashMap<string, MimeRecord*>::Iterator it = data->begin();
	HashMap<string, MimeRecord*>::Iterator end = data->end();

	File::deleteFile(filename);
	f.openFile(filename, FILE_OPEN_WRITE|FILE_OPEN_ALWAYS);
	for(;it != end; it++ )
	{
		char command[16];
    MimeRecord *nmr1=*it;
    if(!nmr1)
      break;
		f.writeToFile(nmr1->extension.c_str(), 
                  (u_long)nmr1->extension.length(), &nbw);
		f.writeToFile(",",1,&nbw);
		f.writeToFile(nmr1->mime_type.c_str(), 
                  (u_long)nmr1->mime_type.length(), &nbw);
		f.writeToFile(",",1,&nbw);
		if(nmr1->command==CGI_CMD_SEND)
			strcpy(command,"SEND ");
		else if(nmr1->command==CGI_CMD_RUNCGI)
			strcpy(command,"RUNCGI ");
		else if(nmr1->command==CGI_CMD_RUNMSCGI)
			strcpy(command,"RUNMSCGI ");
		else if(nmr1->command==CGI_CMD_EXECUTE)
			strcpy(command,"EXECUTE ");
		else if(nmr1->command==CGI_CMD_SENDLINK)
			strcpy(command,"SENDLINK ");
		else if(nmr1->command==CGI_CMD_RUNISAPI)
			strcpy(command,"RUNISAPI ");
		else if(nmr1->command==CGI_CMD_EXECUTEISAPI)
			strcpy(command,"EXECUTEISAPI ");
		else if(nmr1->command==CGI_CMD_EXECUTEWINCGI)
			strcpy(command,"EXECUTEWINCGI ");
		else if(nmr1->command==CGI_CMD_RUNFASTCGI)
			strcpy(command,"RUNFASTCGI ");	
		else if(nmr1->command==CGI_CMD_EXECUTEFASTCGI)
			strcpy(command,"EXECUTEFASTCGI ");	
		else
			strcpy(command, nmr1->cmd_name.c_str());
		f.writeToFile(command,(u_long)strlen(command),&nbw);
		if(nmr1->cgi_manager.length())
			f.writeToFile(nmr1->cgi_manager.c_str(), 
                    (u_long)nmr1->cgi_manager.length(),&nbw);
		else
			f.writeToFile("NONE",(u_long)strlen("NONE"),&nbw);
		f.writeToFile(";\r\n",(u_long)strlen(";\r\n"),&nbw);
	}
	f.setFilePointer(f.getFileSize()-2);
	f.writeToFile("#\0",2,&nbw);
	f.closeFile();

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
  MimeRecord* mr=data ? data->get(ext) : 0;
  if(mr)
  {
    if(dest)
      strcpy(dest,mr->mime_type.c_str());

    if(dest2)
		{
      if(mr->cgi_manager.length())
      {
        *dest2=new char[mr->cgi_manager.length()+1];
        if(*dest2==0)
          return 0;
        strcpy(*dest2, mr->cgi_manager.c_str());
        }
				else
					*dest2=0;
    }
    return mr->command;
  }
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
  MimeRecord *mr = data ? data->get(ext.c_str()): 0;
	if(mr)
	{
		if(!stringcmpi(mr->extension, ext.c_str()))
		{
			dest.assign(mr->mime_type.c_str());

      if(mr->cgi_manager.length())
      {
        dest2.assign(mr->cgi_manager.c_str());
      }
      else
        dest2.assign("");
    
			return mr->command;
		}
	}
	/*!
   *If the ext is not registered send the file as it is.
   */
	return CGI_CMD_SEND;
}

/*!
 *Get a MIME type by the position of the record in the list.
 */
int MimeManager::getMIME(int id,char* ext,char *dest,char **dest2)
{
  MimeRecord *mr;
	HashMap<string, MimeRecord*>::Iterator it;
	HashMap<string, MimeRecord*>::Iterator end;

	if(data == 0)
		return 0;

	it = data->begin();
	end = data->end();
	if(id > data->size() || id < 0)
  {
    return CGI_CMD_SEND;   
  }
	/*! FIXME: find a O(1) solution. */
	while(id-- && it != end)it++;

  mr = *it;

  if(!mr)
    return CGI_CMD_SEND;

  if(ext)
    strcpy(ext,mr->extension.c_str());
  if(dest)
    strcpy(dest,mr->mime_type.c_str());
  if(dest2)
  {
    if(mr->cgi_manager.length())
    {
      *dest2 = new char[mr->cgi_manager.length()+1];
      if(*dest2==0)
        return 0;
      strcpy(*dest2, mr->cgi_manager.c_str());
    }
    else
      dest2=0;
  }

  return mr->command;

}

/*!
 *Get a MIME type by the position of the record in the list.
 */
int MimeManager::getMIME(int id,string& ext,string& dest,string& dest2)
{
  MimeRecord *mr;
  if(!data || id > data->size() || id < 0)
  {
    return CGI_CMD_SEND;   
  }

  mr=data->get(ext.c_str());
  if(mr)
  {
    ext.assign(mr->extension);
    dest.assign(mr->mime_type);
			
    if(mr->cgi_manager.length())
      dest2.assign(mr->cgi_manager.c_str());
    else
      dest2.assign("");
			
    return mr->command;
  }

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
    loaded = 0;
		if(filename) 
			delete filename;
    filename = 0;
    removeAllRecords();
  }
}

/*!
 *Constructor of the class.
 */
MimeManager::MimeManager()
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
  MimeRecord *nmr=0;
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
  MimeRecord *rec=data->remove(ext.c_str());
  if(rec)
    delete rec;
}

/*!
 *Remove all records from the linked list.
 */
void MimeManager::removeAllRecords()
{
	HashMap<string, MimeRecord*>::Iterator it = data->begin();
	HashMap<string, MimeRecord*>::Iterator end = data->end();
  for(; it != end; it++)
  {
    MimeRecord *rec=*it;
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
  return data ? data->get(ext.c_str()) : 0;
}

/*!
 *Returns the number of MIME types loaded.
 */
u_long MimeManager::getNumMIMELoaded()
{
	return data ? data->size() : 0;
}

/*!
 *Check if the MIME manager is loaded.
 */
int MimeManager::isLoaded()
{
  return loaded;
}
