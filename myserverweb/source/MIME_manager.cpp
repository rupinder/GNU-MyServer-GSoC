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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "../stdafx.h"
#include "../include/MIME_manager.h"
#include "../include/filemanager.h"
#include "../include/stringutils.h"
#include "../include/cXMLParser.h"

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
int MimeManager::load(char *fn)
{
  int filenamelen;
	char *buffer;
	int ret;
  char commandString[16];
	File f;
  if(filename == 0)
    return -1;
  filenamelen = strlen(fn) + 1;
  filename = new char[filenamelen];
	
	strncpy(MimeManager::filename,filename,MAX_PATH);
	numMimeTypesLoaded=0;

	ret=f.openFile(filename,FILE_OPEN_READ|FILE_OPEN_IFEXISTS);
	if(ret)
		return 0;
	u_long fs=f.getFileSize();
	buffer=new char[fs+1];
	if(!buffer)
		return 0;
	u_long nbw;
	f.readFromFile(buffer,fs,&nbw);
	f.closeFile();
	MimeManager::MimeRecord record;
	for(u_long nc=0;;)
	{
		memset(&record, 0, sizeof(MimeManager::MimeRecord));
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
			if(isalpha(buffer[nc])||isdigit(buffer[nc]))
				if((buffer[nc]!='\n')&&(buffer[nc]!='\r')&&(buffer[nc]!=' '))
					record.extension[strlen(record.extension)]=buffer[nc];
			nc++;
		}
		nc++;
		while(buffer[nc]!=',')
		{
			if((buffer[nc]!='\n')&&(buffer[nc]!='\r')&&(buffer[nc]!=' '))
				record.mime_type[strlen(record.mime_type)]=buffer[nc];
			nc++;
		}
		nc++;
		/*!
     *Save the action to do with this type of files.
     */
		memset(commandString, 0, sizeof(commandString));

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
    record.cgi_manager = new char[MAX_PATH];
    if(record.cgi_manager == 0)
    {
      clean();
      return -1;
    }
		while(buffer[nc]!=';')
		{
			if((buffer[nc]!='\n')&&(buffer[nc]!='\r'))
				record.cgi_manager[strlen(record.cgi_manager)]=buffer[nc];
			nc++;
			/*!
       *If the CGI manager path is "NONE" then set the cgi_manager element in 
       *the record structure to a NULL string
       */
			if(!lstrcmpi(record.cgi_manager,"NONE"))
				record.cgi_manager = 0;
			
		}
		addRecord(record);
		nc++;
	}
	delete [] buffer;
	return numMimeTypesLoaded;

}
/*!
 *Get the name of the file opened by the class.
 */
char *MimeManager::getFilename()
{
	return filename;
}

/*!
 *Load the MIME types from a XML file.
 */
int MimeManager::loadXML(char *filename)
{
	XmlParser parser;
	xmlNodePtr node;
  xmlDocPtr doc;
	int nm;
  int filenamelen;
  MimeManager::filename = new char[filenamelen];
  if(MimeManager::filename==0)
    return -1;
	strcpy(MimeManager::filename,filename);
	if(parser.open(filename))
	{
		return -1;
	}
	doc = parser.getDoc();
	node=doc->children->children;
	nm=0;
	for(;node;node=node->next )
	{
		if(xmlStrcmp(node->name, (const xmlChar *)"MIMETYPE"))
			continue;
		xmlNodePtr lcur=node->children;
		MimeRecord rc;
		rc.command=rc.extension[0]=rc.mime_type[0]='\0';
    rc.cgi_manager = 0;
		while(lcur)
		{
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"EXT"))
			{
				if(lcur->children->content)
					strcpy(rc.extension,(char*)lcur->children->content);
			}
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"MIME"))
			{
				if(lcur->children->content)
					strcpy(rc.mime_type,(char*)lcur->children->content);
			}
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"CMD"))
			{
				if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"SEND"))
					rc.command=CGI_CMD_SEND;
				if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"RUNCGI"))
					rc.command=CGI_CMD_RUNCGI;
				if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"RUNMSCGI"))
					rc.command=CGI_CMD_RUNMSCGI;
				if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"EXECUTE"))
					rc.command=CGI_CMD_EXECUTE;
				if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"RUNISAPI"))
					rc.command=CGI_CMD_RUNISAPI;
				if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"EXECUTEISAPI"))
					rc.command=CGI_CMD_EXECUTEISAPI;
				if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"SENDLINK"))
					rc.command=CGI_CMD_SENDLINK;
				if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"EXECUTEWINCGI"))
					rc.command=CGI_CMD_EXECUTEWINCGI;
				if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"RUNFASTCGI"))
					rc.command=CGI_CMD_RUNFASTCGI;
				if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"EXECUTEFASTCGI"))
					rc.command=CGI_CMD_EXECUTEFASTCGI;
			}
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"MANAGER"))
			{
				if(lcur->children->content)
				{
					if(lstrcmpi((char*)lcur->children->content,"NONE"))
          {
            int cgi_managerlen=strlen((char*)lcur->children->content)+1;
            rc.cgi_manager=new char[cgi_managerlen];
            if(rc.cgi_manager == 0)
            {
              clean();
              return -1;
            }
						strcpy(rc.cgi_manager,(char*)lcur->children->content);
          }
					else
						rc.cgi_manager=0;
				}
			}
			lcur=lcur->next;
		}
		nm++;
		addRecord(rc);
	}
	parser.close();
  
  /*! Store the loaded status. */
  loaded = 1;

	return nm;
}
/*!
 *Save the MIME types to a XML file.
 */
int MimeManager::saveXML(char *filename)
{
	File::deleteFile(filename);
	File f;
	u_long nbw;
	f.openFile(filename,FILE_OPEN_WRITE|FILE_OPEN_ALWAYS);
	f.writeToFile("<?xml version=\"1.0\"?>\r\n",23,&nbw);
	f.writeToFile("<MIMETYPES>\r\n",13,&nbw);
	MimeRecord *rc=data;
	while(rc)
	{
		char command[16];
		f.writeToFile("\r\n<MIMETYPE>\r\n<EXT>",19,&nbw);
		f.writeToFile(rc->extension,(u_long)strlen(rc->extension),&nbw);
		f.writeToFile("</EXT>\r\n<MIME>",14,&nbw);
		f.writeToFile(rc->mime_type,(u_long)strlen(rc->mime_type),&nbw);
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
		if(rc->cgi_manager)
			f.writeToFile(rc->cgi_manager,(u_long)strlen(rc->cgi_manager),&nbw);
		else
			f.writeToFile("NONE",4,&nbw);
		f.writeToFile("</MANAGER>\r\n</MIMETYPE>\r\n",25,&nbw);
		rc=rc->next;	
	}
	f.writeToFile("\r\n</MIMETYPES>",14,&nbw);
	f.closeFile();
	return 1;
}

/*!
 *Save the MIME types to a file.
 */
int MimeManager::save(char *filename)
{
	File f;
	MimeManager::MimeRecord *nmr1;
  u_long nbw;
	File::deleteFile(filename);
	f.openFile(filename,FILE_OPEN_WRITE|FILE_OPEN_ALWAYS);
	for(nmr1 = data;nmr1;nmr1 = nmr1->next )
	{
		char command[16];
		f.writeToFile(nmr1->extension,(u_long)strlen(nmr1->extension),&nbw);
		f.writeToFile(",",1,&nbw);
		f.writeToFile(nmr1->mime_type,(u_long)strlen(nmr1->mime_type),&nbw);
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
		if(nmr1->cgi_manager)
			f.writeToFile(nmr1->cgi_manager,(u_long)strlen(nmr1->cgi_manager),&nbw);
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
	for(MimeManager::MimeRecord *mr=data;mr;mr=mr->next )
	{
		if(!lstrcmpi(ext,mr->extension))
		{
			if(dest)
				strcpy(dest,mr->mime_type);

			if(dest2)
			{
				if(mr->cgi_manager)
        {
          int cgi_managerlen=strlen(mr->cgi_manager)+1;
          *dest2=new char[cgi_managerlen];
          if(*dest2==0)
            return 0;
					strcpy(*dest2, mr->cgi_manager);
        }
				else
					*dest2=0;
			}
			return mr->command;
		}
	}
	/*!
   *If the ext is not registered send the file as it is.
   */
	return CGI_CMD_SEND;
}
/*!
*Pass only the position of the record in the list.
*/
int MimeManager::getMIME(int id,char* ext,char *dest,char **dest2)
{
	int i=0;
	for(MimeManager::MimeRecord *mr=data;mr;mr=mr->next )
	{
		if(i==id)
		{
			if(ext)
				strcpy(ext,mr->extension);
			if(dest)
				strcpy(dest,mr->mime_type);
			if(dest2)
			{
				if(mr->cgi_manager)
        {
          int cgi_managerlen=strlen(mr->cgi_manager)+1;
          *dest2=new char[cgi_managerlen];
          if(*dest2==0)
            return 0;
					strcpy(*dest2, mr->cgi_manager);
        }
				else
					dest2=0;
			}
			return mr->command;
		}
		i++;
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
  loaded = 0;
  if(filename)
    delete [] filename;
  filename = 0;
	removeAllRecords();
}

/*!
 *Constructor of the class.
 */
MimeManager::MimeManager()
{
	data = 0;
	numMimeTypesLoaded = 0;
	filename = 0;
  loaded = 0;
}

/*!
 *Add a new record.
 */
void MimeManager::addRecord(MimeManager::MimeRecord mr)
{
	/*!
   *If the MIME type already exists remove it.
   */
	if(getRecord(mr.extension))
		removeRecord(mr.extension);
	MimeManager::MimeRecord *nmr =new MimeManager::MimeRecord;
	if(!nmr)	
		return;
	memcpy(nmr,&mr,sizeof(MimeRecord));
	nmr->next =data;
	data=nmr;
	numMimeTypesLoaded++;
}

/*!
 *Remove a record by the extension of the MIME type.
 */
void MimeManager::removeRecord(char *ext)
{
	MimeManager::MimeRecord *nmr1 = data;
	MimeManager::MimeRecord *nmr2 = 0;
	if(!nmr1)
		return;
	do
	{
		if(!lstrcmpi(nmr1->extension,ext))
		{
			if(nmr2)
			{
				nmr2->next  = nmr1->next;
			}
			else
			{
				data=nmr1->next;
				
			}
      if(nmr1->cgi_manager)
        delete nmr1->cgi_manager;
      delete nmr1;
			numMimeTypesLoaded--;
			break;
		}
		nmr2=nmr1;
		nmr1=nmr1->next;
	}while(nmr1);
}

/*!
 *Remove all records from the linked list.
 */
void MimeManager::removeAllRecords()
{
	if(data==0)
		return;
	MimeManager::MimeRecord *nmr1 = data;
	MimeManager::MimeRecord *nmr2 = 0;

	for(;;)
	{
		nmr2=nmr1;
		if(nmr2)
		{
			nmr1=nmr1->next;
      if(nmr2->cgi_manager)
        delete []  nmr2->cgi_manager;
      delete nmr2;
		}
		else
		{
			break;
		}
	}
	data=0;
	numMimeTypesLoaded=0;
}

/*!
 *Get a pointer to an existing record passing its extension.
 *Don't modify the member next of the returned structure.
 */
MimeManager::MimeRecord *MimeManager::getRecord(char ext[10])
{
	MimeManager::MimeRecord *nmr1;
	for(nmr1 = data;nmr1;nmr1 = nmr1->next )
	{
		if(!lstrcmpi(nmr1->extension,ext))
		{
			return nmr1;
		}
	}
	return NULL;
}

/*!
 *Returns the number of MIME types loaded.
 */
u_long MimeManager::getNumMIMELoaded()
{
	return numMimeTypesLoaded;
}

/*!
 *Check if the MIME manager is loaded.
 */
int MimeManager::isLoaded()
{
  return loaded;
}
