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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
}

#ifdef WIN32
#include <direct.h>
#endif

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
	sprintf(localTimeString,"%s, %i %s %i %i:%i:%i GMT",daysName[GMtime->tm_wday],GMtime->tm_mday,monthsName[GMtime->tm_mon],GMtime->tm_year,GMtime->tm_hour,GMtime->tm_min,GMtime->tm_sec);
	return localTimeString;
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
	#ifdef WIN32
	u_short lenTrimchars=(u_short)lstrlen(trimchars);
	u_short lenStr=(u_short)lstrlen(str);
	#else
	u_short lenTrimchars=(u_short)strlen(trimchars);
	u_short lenStr=(u_short)strlen(str);
	#endif
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
	#ifdef WIN32
		lstrcpy(str,&str[ncharToRemove]);
	#else
		strcpy(str,&str[ncharToRemove]);
	#endif
	doBreak=false;
	#ifdef WIN32
	for(j=lstrlen(str)-1;j;j-- )
	#else
	for(j=strlen(str)-1;j;j-- )
	#endif
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
