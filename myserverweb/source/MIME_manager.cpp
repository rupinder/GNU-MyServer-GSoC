/*
*myServer
*Copyright (C) Rocky_10_Balboa
*This library is free software; you can redistribute it and/or
*modify it under the terms of the GNU Library General Public
*License as published by the Free Software Foundation; either
*version 2 of the License, or (at your option) any later version.

*This library is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*Library General Public License for more details.

*You should have received a copy of the GNU Library General Public
*License along with this library; if not, write to the
*Free Software Foundation, Inc., 59 Temple Place - Suite 330,
*Boston, MA  02111-1307, USA.
*/

#include "..\include\mime_manager.h"
#include "..\include\filemanager.h"

/*
*Source code to manage the MIME types in myServer.
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
	numMimeTypesLoaded=0;
	char *buffer;
	MYSERVER_FILE_HANDLE f=ms_OpenFile(filename,MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_IFEXISTS);
	if(f==0)
		return 0;
	DWORD fs=getFileSize(f);
	buffer=(char*)malloc(fs+1);
	DWORD nbw;
	ms_ReadFromFile(f,buffer,fs,&nbw);
	ms_CloseFile(f);
	MIME_Manager::mime_record record;
	for(DWORD nc=0;;)
	{
		ZeroMemory(&record,sizeof(MIME_Manager::mime_record));
		/*
		*Do not consider the \r \n and space characters.
		*/
		while((buffer[nc]==' ') || (buffer[nc]=='\r') ||(buffer[nc]=='\n'))
			nc++;
		/*
		*If reached the # character or the end of the string end the loop.
		*/
		if(buffer[nc]=='#'||buffer[nc]=='\0')
			break;
		while(buffer[nc]!=',')
		{
			if((buffer[nc]!='\n')&&(buffer[nc]!='\r')&&(buffer[nc]!=' '))
				record.extension[lstrlen(record.extension)]=buffer[nc];
				
			nc++;
		}
		nc++;
		while(buffer[nc]!=',')
		{
			if((buffer[nc]!='\n')&&(buffer[nc]!='\r')&&(buffer[nc]!=' '))
				record.mime_type[lstrlen(record.mime_type)]=buffer[nc];
			nc++;
		}
		nc++;
		/*
		*Save the action to do with this type of files.
		*/
		char commandString[16];
		ZeroMemory(commandString,sizeof(commandString));
		while(buffer[nc]!=' ')
		{
			if((buffer[nc]!='\n')&&(buffer[nc]!='\r'))
				commandString[lstrlen(commandString)]=buffer[nc];
			nc++;
			/*
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
		}
		nc++;
		while(buffer[nc]!=';')
		{
			if((buffer[nc]!='\n')&&(buffer[nc]!='\r'))
				record.cgi_manager[lstrlen(record.cgi_manager)]=buffer[nc];
			nc++;
			/*
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

/*
*This function returns the type of action to do for handle this file type.
*Passing a file extension ext this function fills the strings dest and dest2
*respectly with the MIME type description and if there are the path to the CGI manager.
*/
int MIME_Manager::getMIME(char* ext,char *dest,char *dest2)
{
	for(MIME_Manager::mime_record *mr=data;mr;mr=mr->next)
	{
		if(!lstrcmpi(ext,mr->extension))
		{
			lstrcpy(dest,mr->mime_type);
			if(dest2 && mr->cgi_manager[0])
			{
					lstrcpy(dest2,mr->cgi_manager);
			}
			return mr->command;
		}
	}
	return CGI_CMD_SEND;
}

/*
*Clean the memory allocated by the structure.
*/
VOID MIME_Manager::clean()
{
	removeAllRecords();
}

/*
*Constructor of the class.
*/
MIME_Manager::MIME_Manager()
{
	ZeroMemory(this,sizeof(MIME_Manager));
}

/*
*Add a record.
*/
VOID MIME_Manager::addRecord(MIME_Manager::mime_record mr)
{
	MIME_Manager::mime_record *nmr =(MIME_Manager::mime_record*)malloc(sizeof(mime_record));
	memcpy(nmr,&mr,sizeof(mime_record));
	nmr->next=data;
	data=nmr;
	numMimeTypesLoaded++;
}

/*
*Remove a record passing the extension of the MIME type.
*/
VOID MIME_Manager::removeRecord(char *ext)
{
	MIME_Manager::mime_record *nmr1 = data;
	MIME_Manager::mime_record *nmr2 = 0;
	do
	{
		if(!lstrcmpi(nmr1->extension,ext))
		{
			if(nmr2)
			{
				nmr2->next = nmr1->next;
				free(nmr1);
			}
			else
			{
				data=nmr1->next;
				free(nmr1);
			}
			numMimeTypesLoaded--;
		}
		nmr2=nmr1;
		nmr1=nmr1->next;
	}while(nmr1);
}
/*
*Remove all records from the linked list.
*/
VOID MIME_Manager::removeAllRecords()
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
			free(nmr2);
			nmr1=nmr1->next;
		}
		else
		{
			break;
		}
	}
	data=0;
	numMimeTypesLoaded=0;
}

/*
*Returns the number of MIME types loaded.
*/
DWORD MIME_Manager::getNumMIMELoaded()
{
	return numMimeTypesLoaded;
}
