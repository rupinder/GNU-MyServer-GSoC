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

#include "../stdafx.h"
#include "../include/stringutils.h"

extern "C" {
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#include <ctype.h>
#include <stdio.h>
#endif
}

#ifdef WIN32
#include <direct.h>
#endif
#define u_short unsigned short

static char daysName[7][4]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
static char monthsName[12][4]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dic"};
static char localTimeString[31];

/*
*This function format current time to the RFC 822 format.
*/
char *getRFC822GMTTime(void)
{
	time_t ltime;
	time( &ltime );
	return getRFC822GMTTime(ltime);
}
/*
*This function formats a time to the RFC 822 format.
*/
char *getRFC822GMTTime(const time_t ltime)
{
	tm*  GMtime = gmtime( &ltime );
	GMtime->tm_year+=1900;
	sprintf(localTimeString,"%s, %i %s %i %i:%i:%i GMT",daysName[GMtime->tm_wday],GMtime->tm_mday,monthsName[GMtime->tm_mon],GMtime->tm_year,GMtime->tm_hour,GMtime->tm_min,GMtime->tm_sec);
	return localTimeString;
}
/*
*This function convert from a RFC 822 format to a time_t.
*/
time_t getTime(char* str)
{
	char lb[30];
	int c=0;
	int i;
	tm t;
	for(i=0;i<30;i++)
	{
		if(str[c]==',')
		{
			c++;
			lb[i]='\0';
			break;
		}
		else
			lb[i]=str[c++];
	}
	if(!strcmp(lb,"Sun"))
		t.tm_wday = 0;
	if(!strcmp(lb,"Mon"))
		t.tm_wday = 1;
	if(!strcmp(lb,"Tue"))
		t.tm_wday = 2;
	if(!strcmp(lb,"Wed"))
		t.tm_wday = 3;
	if(!strcmp(lb,"Thu"))
		t.tm_wday = 4;
	if(!strcmp(lb,"Fri"))
		t.tm_wday = 5;
	if(!strcmp(lb,"Sat"))
		t.tm_wday = 6;

	c++;
	for(i=0;i<30;i++)
	{
		if(str[c]=' ')
		{
			c++;
			lb[i]='\0';
			break;
		}
		else
			lb[i]=str[c++];
	}
	t.tm_mday=atoi(lb);
	
	c++;
	for(i=0;i<30;i++)
	{
		if(str[c]==' ')
		{
			c++;
			lb[i]='\0';
			break;
		}
		else
			lb[i]=str[c++];
	}
	if(!strcmp(lb,"Jan"))
		t.tm_mon = 0;
	if(!strcmp(lb,"Feb"))
		t.tm_mon = 1;
	if(!strcmp(lb,"Mar"))
		t.tm_mon = 2;
	if(!strcmp(lb,"Apr"))
		t.tm_mon = 3;
	if(!strcmp(lb,"May"))
		t.tm_mon = 4;
	if(!strcmp(lb,"Jun"))
		t.tm_mon = 5;
	if(!strcmp(lb,"Jul"))
		t.tm_mon = 6;	
	if(!strcmp(lb,"Aug"))
		t.tm_mon = 7;
	if(!strcmp(lb,"Sep"))
		t.tm_mon = 8;
	if(!strcmp(lb,"Oct"))
		t.tm_mon = 9;
	if(!strcmp(lb,"Nov"))
		t.tm_mon = 10;
	if(!strcmp(lb,"Dec"))
		t.tm_mon = 11;

	c++;
	for(i=0;i<30;i++)
	{
		if(str[c]==' ')
		{
			c++;
			lb[i]='\0';
			break;
		}
		else
			lb[i]=str[c++];
	}
	t.tm_year=atoi(lb);
	
	c++;
	for(i=0;i<30;i++)
	{
		if(str[c]==':')
		{
			c++;
			lb[i]='\0';
			break;
		}
		else
			lb[i]=str[c++];
	}
	t.tm_hour = atoi(lb);

	c++;
	for(i=0;i<30;i++)
	{
		if(str[c]==':')
		{
			c++;
			lb[i]='\0';
			break;
		}
		else
			lb[i]=str[c++];
	}
	t.tm_min = atoi(lb);

	c++;
	for(i=0;i<30;i++)
	{
		if(str[c]==':')
		{
			c++;
			lb[i]='\0';
			break;
		}
		else
			lb[i]=str[c++];
	}
	t.tm_sec = atoi(lb);
	return mktime(&t);
}

/*
*This function format current time to the RFC 822 format.
*/
char *getRFC822LocalTime(void)
{
	time_t ltime;
	time( &ltime );
	return getRFC822LocalTime(ltime);
}
/*
*This function formats a time to the RFC 822 format.
*/
char *getRFC822LocalTime(const time_t ltime)
{
	tm*  GMtime = localtime( &ltime );
	GMtime->tm_year+=1900;
	sprintf(localTimeString,"%s, %i %s %i %i:%i:%i",daysName[GMtime->tm_wday],GMtime->tm_mday,monthsName[GMtime->tm_mon],GMtime->tm_year,GMtime->tm_hour,GMtime->tm_min,GMtime->tm_sec);
	return localTimeString;
}



/*
*Splits a file path into a directory and filename.
*Path is an input value while dir and filename are the output values.
*/
void splitPath(const char *path, char *dir, char *filename)
{
	int splitpoint, i, j;
	i = 0;
	j = 0;
	splitpoint =(int)( strlen(path) - 1);
	while ((splitpoint > 0) && (path[splitpoint] != '/'))
		splitpoint--;
	if ((splitpoint == 0) && (path[splitpoint] != '/'))
	{
		dir[0] = 0;
		strcpy(filename, path);
	}
	else
	{
		splitpoint++;
		while (i < splitpoint)
		{
			dir[i] = path[i];
			i++;
		}
		dir[i] = 0;
		while (path[i] != 0)
		{
			filename[j] = path[i];
			j++;
			i++;
		}
		filename[j] = 0;
	}
}

/*
*Trim a string.
*/
void StrTrim(char* str,const char* trimchars)
{
	u_short lenTrimchars=(u_short)strlen(trimchars);
	u_short lenStr=(u_short)strlen(str);

	/*
	*Number of characters to remove from the initial position of the string.
	*/
	u_short ncharToRemove=0;
	int doBreak=false;
	if(lenStr==0)
		return;
	if(lenTrimchars==0)
		return;
	int j;
	for(j=0;j<=lenStr;j++)
	{
		if(doBreak)
			break;
		for(int i=0;i<lenTrimchars;i++)
		{
			if(str[j]==trimchars[i])
			{
				ncharToRemove++;
				break;
			}
			else
			{
				if((i==lenTrimchars-1)||(str[j]=='\0'))
				{
					doBreak=true;
					break;
				}
			}
		}
	}
	if(ncharToRemove)
		strcpy(str,&str[ncharToRemove]);
	doBreak=false;
	for(j=strlen(str)-1;j;j-- )
	{
		if(doBreak)
			break;
		for(int i=0;i<lenTrimchars;i++)
		{
			if(str[j]==trimchars[i])
			{
				str[j]='\0';
				break;
			}
			else
			{
				if(i==lenTrimchars-1)
				{
					doBreak=true;
					break;
				}
			}
		}
	}
}

/*
*Set the buffer passed to the next line.
*A new line is the first character after \n.
*/
void gotoNextLine(char* cmd)
{
	while(*cmd++!='\n')if(*cmd=='\0')break;

}

/*
*Get the file extension passing its path.
*/
void getFileExt(char* ext,const char* filename)
{
	int nDot, nPathLen;
	nPathLen =(int)(strlen(filename) - 1);
	nDot = nPathLen;
	while ((nDot > 0) && (filename[nDot] != '.'))
		nDot--;
	if (nDot > 0)
		strcpy(ext, filename + nDot + 1);
	else
		ext[0] = 0;
}


/*
*Translates HTTP escape sequences.
*/
void translateEscapeString(char *str)
{
	int i, j;
	i = 0;
	j = 0;
	while (str[i] != 0)
	{
		if ((str[i] == '%') && (str[i+1] != 0) && (str[i+2] != 0))
		{
			str[j] =(char) (16 * hexVal(str[i+1]) + hexVal(str[i+2]));
			i = i + 3;
		}
		else
		{
			str[j] = str[i];
			i++;
		}
		j++;
	}
	str[j] = 0;
}

/*
*This function converts a hexadecimal number to a decimal number.
*/
int hexVal(char ch)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	else
	{
		if (ch >= 'a' && ch <= 'f')
			return ch - 'a' + 10;
		else
		{
			if (ch >= 'A' && ch <= 'F')
				return ch - 'A' + 10;
			else
				return 0;
		}
	}
}

/*
*Get the filename from a path.
*/
void getFilename(const char *path, char *filename)
{
	int splitpoint, i, j;
	i = 0;
	j = 0;
	splitpoint =(int)(strlen(path) - 1);
	while ((splitpoint > 0) && (path[splitpoint] != '/'))
	splitpoint--;
	if ((splitpoint == 0) && (path[splitpoint] != '/'))
	{
		strcpy(filename, path);
	}
	else
	{
		splitpoint++;
		i=splitpoint;
		while (path[i] != 0)
		{
			filename[j] = path[i];
			j++;
			i++;
		}
		filename[j] = 0;
	}
}

#ifdef __linux__
void strupr(char * string)
{
    unsigned int len = strlen(string);
    for(unsigned int i = 0; i < len; i++)
       string[i] = toupper(string[i]);
}
#endif
