/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
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
#include "../include/MIME_manager.h"
#include "../include/filemanager.h"
#include "../include/stringutils.h"
#include "../include/cXMLParser.h"

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
	MimeManager::MimeRecord record;  
  if(fn == 0)
    return -1;
	
	filename.assign(fn);
	numMimeTypesLoaded=0;

	ret=f.openFile(filename.c_str(), FILE_OPEN_READ|FILE_OPEN_IFEXISTS);
	if(ret)
		return 0;
	fs=f.getFileSize();
	buffer=new char[fs+1];
	if(!buffer)
		return 0;

	f.readFromFile(buffer,fs,&nbw);
	f.closeFile();
	for(u_long nc=0;;)
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
			if(!lstrcmpi(commandString, "RUNCGI"))
				record.command=CGI_CMD_RUNCGI;
			if(!lstrcmpi(commandString, "RUNMSCGI"))
				record.command=CGI_CMD_RUNMSCGI;
			if(!lstrcmpi(commandString, "EXECUTE"))
				record.command=CGI_CMD_EXECUTE;
			if(!lstrcmpi(commandString, "RUNISAPI"))
				record.command=CGI_CMD_RUNISAPI;
			if(!lstrcmpi(commandString, "EXECUTEISAPI"))
				record.command=CGI_CMD_EXECUTEISAPI;
			if(!lstrcmpi(commandString, "SENDLINK"))
				record.command=CGI_CMD_SENDLINK;
			if(!lstrcmpi(commandString, "EXECUTEWINCGI"))
				record.command=CGI_CMD_EXECUTEWINCGI;
			if(!lstrcmpi(commandString, "RUNFASTCGI"))
				record.command=CGI_CMD_RUNFASTCGI;
			if(!lstrcmpi(commandString, "EXECUTEFASTCGI"))
				record.command=CGI_CMD_EXECUTEFASTCGI;
			
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
	return data.size();

}

/*!
 *Destroy the object.
 */
MimeManager::MimeRecord::~MimeRecord()
{
  clear();
}

/*!
 *Copy constructor.
 */
MimeManager::MimeRecord::MimeRecord(MimeRecord& m)
{
  list<string*>::iterator i=m.filters.begin();

  filters.clear(); 

  for( ; i != m.filters.end(); i++)
  {
    string *filter = new string();
    filter->assign( *(*i) );
    filters.push_back(filter);
  }
  extension.assign(m.extension); 
  mime_type.assign(m.mime_type);
  command=m.command; 
  cgi_manager.assign(m.cgi_manager);
} 

/*!
  *Clear the used memory.
  */
void MimeManager::MimeRecord::clear()
{
  list<string*>::iterator i = filters.begin();
  for( ; i != filters.end() ; i++)
  {
    delete (*i);
  }
  extension.assign(""); 
  mime_type.assign(""); 
  cgi_manager.assign("");
}  

/*!
 *Get the name of the file opened by the class.
 */
const char *MimeManager::getFilename()
{
	return filename.c_str();
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
	filename.assign(fn);
	if(parser.open(fn))
	{
		return -1;
	}
	doc = parser.getDoc();
	node=doc->children->children;
	for(;node;node=node->next )
	{
		xmlNodePtr lcur=node->children;
		MimeRecord rc;
		if(xmlStrcmp(node->name, (const xmlChar *)"MIMETYPE"))
			continue;
		rc.clear();
		while(lcur)
		{
			if(lcur->name && !xmlStrcmp(lcur->name, (const xmlChar *)"EXT"))
			{
				if(lcur->children->content)
					rc.extension.assign((char*)lcur->children->content);
			}

			if(lcur->name && !xmlStrcmp(lcur->name, (const xmlChar *)"MIME"))
			{
				if(lcur->children->content)
					rc.mime_type.assign((char*)lcur->children->content);
			}

			if(lcur->name && !xmlStrcmp(lcur->name, (const xmlChar *)"CMD"))
			{
				if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"SEND"))
					rc.command=CGI_CMD_SEND;

				if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"RUNCGI"))
					rc.command=CGI_CMD_RUNCGI;
			
        if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"RUNMSCGI"))
					rc.command=CGI_CMD_RUNMSCGI;

				if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"EXECUTE"))
					rc.command=CGI_CMD_EXECUTE;

				if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"RUNISAPI"))
					rc.command=CGI_CMD_RUNISAPI;

				if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"EXECUTEISAPI"))
					rc.command=CGI_CMD_EXECUTEISAPI;

				if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"SENDLINK"))
					rc.command=CGI_CMD_SENDLINK;

				if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"EXECUTEWINCGI"))
					rc.command=CGI_CMD_EXECUTEWINCGI;

				if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"RUNFASTCGI"))
					rc.command=CGI_CMD_RUNFASTCGI;

				if(lcur->children->content && 
           !xmlStrcmp(lcur->children->content,(const xmlChar *)"EXECUTEFASTCGI"))
					rc.command=CGI_CMD_EXECUTEFASTCGI;
			}

			if(lcur->name && !xmlStrcmp(lcur->name, (const xmlChar *)"MANAGER"))
			{
        /*! If the specified manager is not NONE store its path in the record. */
				if(lcur->children->content && lstrcmpi((char*)lcur->children->content,"NONE"))
        {
          rc.cgi_manager.assign((char*)lcur->children->content);
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

	return data.size();
}

/*!
 *Add a filter to the list of filters to apply on this MIME type.
 *Return zero if the filters was not added.
 */
int MimeManager::MimeRecord::addFilter(const char* n, int acceptDuplicate) 
{
  string *s;
  if(!acceptDuplicate)
  {
    list<string*>::iterator i = filters.begin();
    for( ; i != filters.end() ;i++ )
    {
      if(!stringcmpi(*(*i), n))
        return 0;
    }
  }
  s = new string();
  s->assign(n);
  filters.push_back(s);
  return 1;
}

/*!
 *Save the MIME types to a XML file.
 */
int MimeManager::saveXML(const char *filename)
{
	File::deleteFile(filename);
	File f;
  int i;
	u_long nbw;
	f.openFile(filename, FILE_OPEN_WRITE|FILE_OPEN_ALWAYS);
	f.writeToFile("<?xml version=\"1.0\"?>\r\n",23,&nbw);
	f.writeToFile("<MIMETYPES>\r\n",13,&nbw);

	for(i=0; i<data.size(); i++)
	{
    MimeRecord *rc=data.getData(i);
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
  int i;
  u_long nbw;
	File::deleteFile(filename);
	f.openFile(filename, FILE_OPEN_WRITE|FILE_OPEN_ALWAYS);
	for(i=0;i<data.size(); i++ )
	{
		char command[16];
    MimeManager::MimeRecord *nmr1=data.getData(i);
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
  MimeRecord* mr=data.getData(ext);
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
  MimeManager::MimeRecord *mr=data.getData(ext.c_str());
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
  if(id > data.size() || id < 0)
  {
    return CGI_CMD_SEND;   
  }

  mr = data.getData(id);

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
      *dest2=new char[mr->cgi_manager.length()+1];
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
  MimeManager::MimeRecord *mr;
  if(id > data.size() || id < 0)
  {
    return CGI_CMD_SEND;   
  }

  mr=data.getData(ext.c_str());
  if(mr)
  {
    ext.assign(mr->extension);
    dest.assign(mr->mime_type);
			
    if(mr->cgi_manager.length())
    {
      dest2.assign(mr->cgi_manager.c_str());
    }
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
    filename.assign("");
    removeAllRecords();
  }
}

/*!
 *Constructor of the class.
 */
MimeManager::MimeManager()
{
	data.clear();
  filename.assign("");
  loaded = 0;
}

/*!
 *Add a new record. Returns zero on success.
 */
int MimeManager::addRecord(MimeManager::MimeRecord& mr)
{
	/*!
   *If the MIME type already exists remove it.
   */
  MimeManager::MimeRecord *nmr=0;
  try
  {
    if(getRecord(mr.extension))
      removeRecord(mr.extension);
    nmr = new MimeManager::MimeRecord(mr);
    if(!nmr)	
      return 1;
    data.insert(nmr->extension.c_str(), nmr);
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
  MimeManager::MimeRecord *rec=data.removeNode(ext.c_str());
  if(rec)
    delete rec;
}

/*!
 *Remove all records from the linked list.
 */
void MimeManager::removeAllRecords()
{
  int i;
  for(i=0 ; i<data.size(); i++)
  {
    MimeManager::MimeRecord *rec=data.getData(i);
    if(rec)
      delete rec;
  }

	data.clear();
}

/*!
 *Get a pointer to an existing record passing its extension.
 */
MimeManager::MimeRecord *MimeManager::getRecord(string const &ext)
{
  return data.getData(ext.c_str());
}

/*!
 *Returns the number of MIME types loaded.
 */
u_long MimeManager::getNumMIMELoaded()
{
	return data.size();
}

/*!
 *Check if the MIME manager is loaded.
 */
int MimeManager::isLoaded()
{
  return loaded;
}
