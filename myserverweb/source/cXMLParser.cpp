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

#include "..\include\cXMLParser.h"
#include "..\include\Utility.h"

/*
*This code is used to parse a pseudo-xml file.
*/
void cXMLParser::open(char* filename)
{
	file=ms_OpenFile(filename,MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_IFEXISTS);
	buffersize=getFileSize(file);
	buffer=(char*)malloc(buffersize);
	DWORD nbr;
	if(buffer)
		ms_ReadFromFile(file,buffer,buffersize,&nbr);
	if(nbr==0)
		buffer[0]='#';
}
/*
*Only get the the text T in <VALUENAME>T</VALUENAME>
*/
char *cXMLParser::getValue(char* vName)
{
	if(buffer==0)
		return 0;
	BOOL found;
	char *ret=NULL;
	unsigned int i,j;
	int len=lstrlen(vName);
	for(i=0;i<buffersize;i++)
	{
 		if(buffer[i]=='<')
		{
			/*
			*If there is a comment go to end of this
			*/
			if(buffer[i+1]=='!')
			if(buffer[i+2]=='-')
			if(buffer[i+3]=='-')
			{
				i+=3;
				while((buffer[i]!='-')&&(buffer[i+1]!='>'))
					i++;
				continue;
			}
			/*
			*If we arrive here this isn't a comment
			*/
			found=TRUE;
			for(j=0;j<len;j++)
			{
				if(buffer[i+j+1]!=vName[j])
				{
					found=FALSE;
					break;
				}
			}
			if(buffer[i+j+1]!='>')
				found=FALSE;
				
			while(buffer[i++]!='>');
			if(found)
			{
				for(j=0;;j++)
				{
					data[j]=buffer[i+j];
					data[j+1]='\0';
					if(buffer[i+j+1]=='<')
						break;
				}
				ret=data;
				break;
			}
		}
	}
	return ret;
}

void cXMLParser::close()
{
	if(file)
		ms_CloseFile(file);
	if(buffer)
		free(buffer);
}

