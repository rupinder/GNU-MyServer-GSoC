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
	lstrcpy(this->filename,filename);
	numMimeTypesLoaded=0;
	char *buffer;
	MYSERVER_FILE_HANDLE f=ms_OpenFile(filename,MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_IFEXISTS);
	if(f==0)
		return 0;
	u_long fs=ms_getFileSize(f);
	buffer=(char*)malloc(fs+1);
	u_long nbw;
	ms_ReadFromFile(f,buffer,fs,&nbw);
	ms_CloseFile(f);
	MIME_Manager::mime_record record;
	for(u_long nc=0;;)
	{
		ZeroMemory(&record,sizeof(MIME_Manager::mime_record));
		/*
		*Do not consider the \r \n and space characters.
		*/
		while((buffer[nc]==' ') || (buffer[nc]=='\r') ||(buffer[nc]=='\n'))
			nc++;
		/*
		*If is reached the # character or the end of the string end the loop.
		*/
		if(buffer[nc]=='#'||buffer[nc]=='\0')
			break;
		while(buffer[nc]!=',')
		{
			if(isalpha(buffer[nc])||isdigit(buffer[nc]))
				if((buffer[nc]!='\n')&&(buffer[nc]!='\r')&&(buffer[nc]!=' '))
					record.extension[lstrlen(record.extension)]=buffer[nc];
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
			if(!lstrcmpi(commandString,"RUNISAPI"))
				record.command=CGI_CMD_RUNISAPI;
			if(!lstrcmpi(commandString,"SENDLINK"))
				record.command=CGI_CMD_SENDLINK;
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
*Get the name of the file opened by the class.
*/
char *MIME_Manager::getFilename()
{
	return filename;
}
/*
*Save the MIME types to a file.
*/
int MIME_Manager::save(char *filename)
{
	ms_DeleteFile(filename);
	MYSERVER_FILE_HANDLE f=ms_OpenFile(filename,MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_OPEN_ALWAYS);
	MIME_Manager::mime_record *nmr1;
	u_long nbw;
	for(nmr1 = data;nmr1;nmr1 = nmr1->next)
	{
		ms_WriteToFile(f,nmr1->extension,lstrlen(nmr1->extension),&nbw);
		ms_WriteToFile(f,",",lstrlen(","),&nbw);
		ms_WriteToFile(f,nmr1->mime_type,lstrlen(nmr1->mime_type),&nbw);
		ms_WriteToFile(f,",",lstrlen(","),&nbw);
		char command[16];
		if(nmr1->command==CGI_CMD_SEND)
			lstrcpy(command,"SEND ");
		else if(nmr1->command==CGI_CMD_RUNCGI)
			lstrcpy(command,"RUNCGI ");
		else if(nmr1->command==CGI_CMD_RUNMSCGI)
			lstrcpy(command,"RUNMSCGI ");
		else if(nmr1->command==CGI_CMD_EXECUTE)
			lstrcpy(command,"EXECUTE ");
		else if(nmr1->command==CGI_CMD_SENDLINK)
			lstrcpy(command,"SENDLINK ");
		else if(nmr1->command==CGI_CMD_RUNISAPI)
			lstrcpy(command,"RUNISAPI ");

		ms_WriteToFile(f,command,lstrlen(command),&nbw);
		if(nmr1->cgi_manager[0])
			ms_WriteToFile(f,nmr1->cgi_manager,lstrlen(nmr1->cgi_manager),&nbw);
		else
			ms_WriteToFile(f,"NONE",lstrlen("NONE"),&nbw);
		ms_WriteToFile(f,";\r\n",lstrlen(";\r\n"),&nbw);
	}
	ms_setFilePointer(f,ms_getFileSize(f)-2);
	ms_WriteToFile(f,"#\0",2,&nbw);
	ms_CloseFile(f);

	return 1;
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
			if(dest)
				lstrcpy(dest,mr->mime_type);

			if(dest2)
			{
				if(mr->cgi_manager[0])
					lstrcpy(dest2,mr->cgi_manager);
				else
					dest2[0]='\0';
			}
			return mr->command;
		}
	}
	/*
	*If the ext is not registered send the file as it is.
	*/
	return CGI_CMD_SEND;
}
/*
*Pass only the position of the record in the list.
*/
int MIME_Manager::getMIME(int id,char* ext,char *dest,char *dest2)
{
	int i=0;
	for(MIME_Manager::mime_record *mr=data;mr;mr=mr->next)
	{
		if(i==id)
		{
			if(ext)
				lstrcpy(ext,mr->extension);

			if(dest)
				lstrcpy(dest,mr->mime_type);

			if(dest2)
			{
				if(mr->cgi_manager[0])
					lstrcpy(dest2,mr->cgi_manager);
				else
					dest2[0]='\0';
			}
			return mr->command;
		}
		i++;
	}
	/*
	*If the ext is not registered send the file as it is.
	*/
	return CGI_CMD_SEND;
}

/*
*Clean the memory allocated by the structure.
*/
void MIME_Manager::clean()
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
*Add a new record.
*/
void MIME_Manager::addRecord(MIME_Manager::mime_record mr)
{
	/*
	*If the MIME type already exists remove it.
	*/
	if(getRecord(mr.extension))
		removeRecord(mr.extension);
	MIME_Manager::mime_record *nmr =(MIME_Manager::mime_record*)malloc(sizeof(mime_record));
	memcpy(nmr,&mr,sizeof(mime_record));
	nmr->next=data;
	data=nmr;
	numMimeTypesLoaded++;
}

/*
*Remove a record passing the extension of the MIME type.
*/
void MIME_Manager::removeRecord(char *ext)
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
/*
*Get a pointer to an existing record passing its extension.
*Don't modify the member next of the returned structure.
*/
MIME_Manager::mime_record *MIME_Manager::getRecord(char ext[10])
{
	MIME_Manager::mime_record *nmr1;
	for(nmr1 = data;nmr1;nmr1 = nmr1->next)
	{
		if(!lstrcmpi(nmr1->extension,ext))
		{
			return nmr1;
		}
	}
	return NULL;
}

/*
*Returns the number of MIME types loaded.
*/
u_long MIME_Manager::getNumMIMELoaded()
{
	return numMimeTypesLoaded;
}
