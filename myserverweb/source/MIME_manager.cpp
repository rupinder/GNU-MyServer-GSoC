/*
*MyServer
*Copyright (C) 2002 The MyServer Team
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

#include "../include/MIME_manager.h"
#include "../include/filemanager.h"
#include "../include/stringutils.h"
#include "../include/cXMLParser.h"

/*!
*Source code to manage the MIME types in MyServer.
*MIME types are recorded in a static buffer "data", that is a strings array.
*Every MIME type is described by three strings:
html,text/html,SEND NONE;
*1)its extension(for example HTML)
*2)its MIME description(for example text/html)
*3)simple script to describe the action to do for handle the files of this type.
*The file is ended by a '#' character.
*/
int MIME_Manager::load(char *filename)
{
	strncpy(MIME_Manager::filename,filename,MAX_PATH);
	numMimeTypesLoaded=0;
	char *buffer;
	MYSERVER_FILE f;
	int ret=f.openFile(filename,MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_IFEXISTS);
	if(ret<1)
		return 0;
	u_long fs=f.getFileSize();
	buffer=(char*)malloc(fs+1);	
	if(!buffer)
		return 0;
	u_long nbw;
	f.readFromFile(buffer,fs,&nbw);
	f.closeFile();
	MIME_Manager::mime_record record;
	for(u_long nc=0;;)
	{
		memset(&record, 0, sizeof(MIME_Manager::mime_record));
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
		char commandString[16];
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
			if(!lstrcmpi(commandString,"SEND"))
				record.command=CGI_CMD_SEND;
			if(!lstrcmpi(commandString,"RUNCGI"))
				record.command=CGI_CMD_RUNCGI;
			if(!lstrcmpi(commandString,"RUNMSCGI"))
				record.command=CGI_CMD_RUNMSCGI;
			if(!lstrcmpi(commandString,"EXECUTE"))
				record.command=CGI_CMD_EXECUTE;
			if(!lstrcmpi(commandString,"RUNISAPI"))
				record.command=CGI_CMD_RUNISAPI;
			if(!lstrcmpi(commandString,"EXECUTEISAPI"))
				record.command=CGI_CMD_EXECUTEISAPI;
			if(!lstrcmpi(commandString,"SENDLINK"))
				record.command=CGI_CMD_SENDLINK;
			if(!lstrcmpi(commandString,"EXECUTEWINCGI"))
				record.command=CGI_CMD_EXECUTEWINCGI;
			if(!lstrcmpi(commandString,"RUNFASTCGI"))
				record.command=CGI_CMD_RUNFASTCGI;
			if(!lstrcmpi(commandString,"EXECUTEFASTCGI"))
				record.command=CGI_CMD_EXECUTEFASTCGI;
			
		}
		nc++;
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
				record.cgi_manager[0]='\0';
			
		}
		addRecord(record);
		nc++;
	}
	free(buffer);
	return numMimeTypesLoaded;

}
/*!
*Get the name of the file opened by the class.
*/
char *MIME_Manager::getFilename()
{
	return filename;
}

/*!
*Load the MIME types from a XML file.
*/
int MIME_Manager::loadXML(char *filename)
{
	cXMLParser parser;
	strncpy(MIME_Manager::filename,filename,MAX_PATH);
	if(int r=parser.open(filename))
	{
		return -1;
	}
	xmlDocPtr doc = parser.getDoc();
	xmlNodePtr node=doc->children->children;
	int nm=0;
	for(;node;node=node->next )
	{
		if(xmlStrcmp(node->name, (const xmlChar *)"MIMETYPE"))
			continue;
		xmlNodePtr lcur=node->children;
		mime_record rc;
		rc.command=rc.extension[0]=rc.mime_type[0]='\0';
		strcpy(rc.cgi_manager,"NONE");
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
						strcpy(rc.cgi_manager,(char*)lcur->children->content);
					else
						rc.cgi_manager[0]='\0';
				}
			}
			lcur=lcur->next;
		}
		nm++;
		addRecord(rc);
	}
	parser.close();
	return nm;
}
/*!
*Save the MIME types to a XML file.
*/
int MIME_Manager::saveXML(char *filename)
{
	MYSERVER_FILE::deleteFile(filename);
	MYSERVER_FILE f;
	u_long nbw;
	f.openFile(filename,MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_OPEN_ALWAYS);
	f.writeToFile("<?xml version=\"1.0\"?>\r\n",23,&nbw);
	f.writeToFile("<MIMETYPES>\r\n",13,&nbw);
	mime_record *rc=data;
	while(rc)
	{
		f.writeToFile("\r\n<MIMETYPE>\r\n<EXT>",19,&nbw);
		f.writeToFile(rc->extension,(u_long)strlen(rc->extension),&nbw);
		f.writeToFile("</EXT>\r\n<MIME>",14,&nbw);
		f.writeToFile(rc->mime_type,(u_long)strlen(rc->mime_type),&nbw);
		f.writeToFile("</MIME>\r\n<CMD>",14,&nbw);
		char command[16];
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
		if(rc->cgi_manager[0])
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
int MIME_Manager::save(char *filename)
{
	MYSERVER_FILE::deleteFile(filename);
	MYSERVER_FILE f;
	f.openFile(filename,MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_OPEN_ALWAYS);
	MIME_Manager::mime_record *nmr1;
	u_long nbw;
	for(nmr1 = data;nmr1;nmr1 = nmr1->next )
	{
		f.writeToFile(nmr1->extension,(u_long)strlen(nmr1->extension),&nbw);
		f.writeToFile(",",1,&nbw);
		f.writeToFile(nmr1->mime_type,(u_long)strlen(nmr1->mime_type),&nbw);
		f.writeToFile(",",1,&nbw);
		char command[16];
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
		if(nmr1->cgi_manager[0])
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
*respectly with the MIME type description and if there are the path to the CGI manager.
*/
int MIME_Manager::getMIME(char* ext,char *dest,char *dest2)
{
	for(MIME_Manager::mime_record *mr=data;mr;mr=mr->next )
	{
		if(!lstrcmpi(ext,mr->extension))
		{
			if(dest)
				strcpy(dest,mr->mime_type);

			if(dest2)
			{
				if(mr->cgi_manager[0])
					strcpy(dest2,mr->cgi_manager);
				else
					dest2[0]='\0';
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
int MIME_Manager::getMIME(int id,char* ext,char *dest,char *dest2)
{
	int i=0;
	for(MIME_Manager::mime_record *mr=data;mr;mr=mr->next )
	{
		if(i==id)
		{
			if(ext)
				strcpy(ext,mr->extension);
			if(dest)
				strcpy(dest,mr->mime_type);
			if(dest2)
			{
				if(mr->cgi_manager[0])
					strcpy(dest2,mr->cgi_manager);
				else
					dest2[0]='\0';
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
*Clean the memory allocated by the structure.
*/
void MIME_Manager::clean()
{
	removeAllRecords();
}

/*!
*Constructor of the class.
*/
MIME_Manager::MIME_Manager()
{
	data=0;
	numMimeTypesLoaded=0;
	filename[0]='\0';
}

/*!
*Add a new record.
*/
void MIME_Manager::addRecord(MIME_Manager::mime_record mr)
{
	/*!
	*If the MIME type already exists remove it.
	*/
	if(getRecord(mr.extension))
		removeRecord(mr.extension);
	MIME_Manager::mime_record *nmr =(MIME_Manager::mime_record*)malloc(sizeof(mime_record));
	if(!nmr)	
		return;
	memcpy(nmr,&mr,sizeof(mime_record));
	nmr->next =data;
	data=nmr;
	numMimeTypesLoaded++;
}

/*!
*Remove a record by the extension of the MIME type.
*/
void MIME_Manager::removeRecord(char *ext)
{
	MIME_Manager::mime_record *nmr1 = data;
	MIME_Manager::mime_record *nmr2 = 0;
	if(!nmr1)
		return;
	do
	{
		if(!lstrcmpi(nmr1->extension,ext))
		{
			if(nmr2)
			{
				nmr2->next  = nmr1->next;
				free(nmr1);
			}
			else
			{
				data=nmr1->next;
				free(nmr1);
			}
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
void MIME_Manager::removeAllRecords()
{
	if(data==0)
		return;
	MIME_Manager::mime_record *nmr1 = data;
	MIME_Manager::mime_record *nmr2 = NULL;

	for(;;)
	{
		nmr2=nmr1;
		if(nmr2)
		{
			nmr1=nmr1->next;
			free(nmr2);
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
MIME_Manager::mime_record *MIME_Manager::getRecord(char ext[10])
{
	MIME_Manager::mime_record *nmr1;
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
u_long MIME_Manager::getNumMIMELoaded()
{
	return numMimeTypesLoaded;
}
