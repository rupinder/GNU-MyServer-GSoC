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
HRESULT MIME_Manager::load(char *filename)
{
	numMimeTypesLoaded=0;
	char buffer[MAX_MIME_TYPES*2*32];
	ZeroMemory(&(data[0][0][0]),sizeof(buffer));
	ZeroMemory(buffer,sizeof(buffer));
	FILE *f=fopen(filename,"rt");
	if(!f)
		return 1;
	fread(buffer,sizeof(buffer),1,f);
	char *token=strtok(buffer,",;");
	for(DWORD i=0;i<MAX_MIME_TYPES;i++)
	{
		if(i)
			token=strtok(NULL,",");
		if(token==NULL)
			break;
		lstrcpy(data[i][0],token);
		token=strtok(NULL,",");
		if(token==NULL)
			break;
		lstrcpy(data[i][1],token);

		token=strtok(NULL,";");
		if(token==NULL)
			break;
		if(lstrcmpi(token,"NONE"))
			lstrcpy(data[i][2],token);
		else
			data[i][2][0]='\0';
		numMimeTypesLoaded++;

		while(data[i][0][0]==10)
			lstrcpy(data[i][0],&data[i][0][1]);
		while(data[i][1][0]==10)
			lstrcpy(data[i][1],&data[i][1][1]);
		while(data[i][2][0]==10)
			lstrcpy(data[i][2],&data[i][2][1]);
		while(data[i][0][0]==13)
			lstrcpy(data[i][0],&data[i][0][1]);
		while(data[i][1][0]==13)
			lstrcpy(data[i][1],&data[i][1][1]);
		while(data[i][2][0]==13)
			lstrcpy(data[i][2],&data[i][2][1]);
	}
	fclose(f);
	return 0;

}
BOOL MIME_Manager::getMIME(char* ext,char *dest,char *dest2)
{
	DWORD i;
	for(i=0;i<numMimeTypesLoaded;i++)
	{
		if(!lstrcmpi(ext,data[i][0]	))
		{
			lstrcpy(dest,data[i][1]);
			if(data[i][2][0])
			{
				if(dest2)
					lstrcpy(dest2,data[i][2]);
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}
	}
	return FALSE;
}
VOID MIME_Manager::dumpToHTML(char *file)
{
	FILE *f=fopen(file,"w+t");
	fprintf(f,"<TABLE BORDER=1>\n<TR>\n<TD>Extension</TD>\n<TD>MIME type</TD>\n</TR>\n");
	for(DWORD i=0;i<numMimeTypesLoaded;i++)
	{
		fprintf(f,"<TR>\n<TD>\n%s</TD>\n<TD>\n%s</TD>\n",data[i][0],data[i][1]);
	}	
	fprintf(f,"</TABLE>\n</BODY>\n");
	fclose(f);

}
VOID MIME_Manager::dumpToFILE(char *file)
{
	FILE *f=fopen(file,"w+t");
	fwrite(data,sizeof(data),1,f);
	fclose(f);

}

VOID MIME_Manager::clean()
{

}
DWORD MIME_Manager::getNumMIMELoaded()
{
	return numMimeTypesLoaded;
}
