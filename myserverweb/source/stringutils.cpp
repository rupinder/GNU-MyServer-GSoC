/*
*MyServer
*Copyright (C) 2002, 2003, 2004 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License,  or
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
#include "../include/stringutils.h"

extern "C" {
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef NOT_WIN
#include <stdio.h>
#endif
}

#ifdef WIN32
#include <direct.h>
#endif
#define u_short unsigned short

/*!
 *This function format current time to the RFC 822 format.
 */
char *getRFC822GMTTime(char* out, int len)
{
	time_t ltime;
	time( &ltime );
	return getRFC822GMTTime(ltime, out, len);
}
/*!
 *This function formats a time to the RFC 822 format.
 */
char *getRFC822GMTTime(const time_t ltime, char* out, int /*!len*/)
{
	tm*  GMtime = gmtime( &ltime );
	GMtime->tm_year+=1900;
	char *asct=asctime(GMtime);
	out[0]=asct[0];
	out[1]=asct[1];
	out[2]=asct[2];
	out[3]=',';
	out[4]=' ';
	out[5]=asct[8];
	out[6]=asct[9];
	out[7]=' ';
	out[8]=asct[4];
	out[9]=asct[5];
	out[10]=asct[6];
	out[11]=' ';
	sprintf(&out[12], "%i", GMtime->tm_year);
	out[16]=' ';
	out[17]=asct[11];
	out[18]=asct[12];
	out[19]=':';
	out[20]=asct[14];
	out[21]=asct[15];
	out[22]=':';
	out[23]=asct[17];
	out[24]=asct[18];
	out[25]=' ';
	out[26]='G';
	out[27]='M';
	out[28]='T';
	out[29]='\0';
	out[30]='\0';
	out[31]='\0';
	return out;
}
/*!
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
	i=lb[0]+lb[1]+lb[3];
	switch(i)
	{
		case 310:
			t.tm_wday = 0;	//Sun
			break;
		case 298:
			t.tm_wday = 1;	//Mon
			break;
		case 302:
			t.tm_wday = 2;	//Tue
			break;
		case 288:
			t.tm_wday = 3;	//Wed
			break;
		case 305:
			t.tm_wday = 4;	//Thu
			break;
		case 289:
			t.tm_wday = 5;	//Fri
			break;
		case 296:
			t.tm_wday = 6;	//Sat
			break;
	}

	c++;
	for(i=0;i<30;i++)
	{
		if(i && str[c]==' ')
		{
			c++;
			lb[i]='\0';
			break;
		}
		else
			lb[i]=str[c++];
	}
	t.tm_mday=atoi(lb);
	
	for(i=0;i<30;i++)
	{
		if(i && str[c]==' ')
		{
			c++;
			lb[i]='\0';
			break;
		}
		else
			lb[i]=str[c++];
	}
	i=lb[0]+lb[1]+lb[3];
	switch(i)
	{
		case 281:
			t.tm_wday = 0;	//Jan
			break;
		case 269:
			t.tm_wday = 1;	//Feb
			break;
		case 288:
			t.tm_wday = 2;	//Mar
			break;
		case 291:
			t.tm_wday = 3;	//Apr
			break;
		case 295:
			t.tm_wday = 4;	//May
			break;
		case 301:
			t.tm_wday = 5;	//Jun
			break;
		case 299:
			t.tm_wday = 6;	//Jul
			break;
		case 285:
			t.tm_wday = 7;	//Aug
			break;
		case 296:
			t.tm_wday = 8;	//Sep
			break;
		case 294:
			t.tm_wday = 9;	//Oct
			break;
		case 307:
			t.tm_wday = 10;	//Nov
			break;
		case 268:
			t.tm_wday = 11;	//Dec
			break;
	}

	for(i=0;i<30;i++)
	{
		if(i && str[c]==' ')
		{
			c++;
			lb[i]='\0';
			break;
		}
		else
			lb[i]=str[c++];
	}
	t.tm_year=atoi(lb)-1900;
	
	for(i=0;i<30;i++)
	{
		if(i && str[c]==':')
		{
			c++;
			lb[i]='\0';
			break;
		}
		else
			lb[i]=str[c++];
	}
	t.tm_hour = atoi(lb);

	for(i=0;i<30;i++)
	{
		if(i && str[c]==':')
		{
			c++;
			lb[i]='\0';
			break;
		}
		else
			lb[i]=str[c++];
	}
	t.tm_min = atoi(lb);

	for(i=0;i<30;i++)
	{
		if(i && str[c]==':')
		{
			c++;
			lb[i]='\0';
			break;
		}
		else
			lb[i]=str[c++];
	}
	t.tm_sec = atoi(lb);
	t.tm_yday=0;
	t.tm_wday=0;	

	t.tm_isdst=-1;
	time_t ret=mktime(&t);
	return ret;
}

/*!
 *This function format current time to the RFC 822 format.
 */
char *getRFC822LocalTime(char* out, int len)
{
	time_t ltime;
	time( &ltime );
	return getRFC822LocalTime(ltime, out, len);
}
/*!
 *This function formats a time to the RFC 822 format.
 */
char *getRFC822LocalTime(const time_t ltime, char* out, int /*!len*/)
{
	tm*  GMtime = localtime( &ltime );
	GMtime->tm_year+=1900;
	char *asct=asctime(GMtime);
	out[0]=asct[0];
	out[1]=asct[1];
	out[2]=asct[2];
	out[3]=',';
	out[4]=' ';
	out[5]=asct[8];
	out[6]=asct[9];
	out[7]=' ';
	out[8]=asct[4];
	out[9]=asct[5];
	out[10]=asct[6];
	out[11]=' ';
	sprintf(&out[12], "%i", GMtime->tm_year);
	out[16]=' ';
	out[17]=asct[11];
	out[18]=asct[12];
	out[19]=':';
	out[20]=asct[14];
	out[21]=asct[15];
	out[22]=':';
	out[23]=asct[17];
	out[24]=asct[18];
	out[25]=' ';
	out[26]='\0';
	out[27]='\0';
	out[28]='\0';
	out[29]='\0';
	out[30]='\0';
	out[31]='\0';
	return out;
}

/*!
 *This funtions takes two strings, first the str we're going to work on,
 *and second a list of characters that the funtion is going to remove 
 *from the head and tail of the first string.
 *Ex:       char str[16]="Hellow World!!!";
 *          char trim[7]="e!HlwW";
 *	        StrTrim(str,trim);
 *result:	  str="ow World"
 *'w', 'W' and the last 'l' aren't removed because they aren't 
 *attached to the head or tail of the string
 */

void StrTrim(char* str, char* trimchars)
{
	char *strptr=str;
	char *trimptr=trimchars;
	
	/*!
	*Here we trim the characters of the head of the string.
	*Just cycle through the trimchars and compare,
	*if we find a char in str, increment the str to check the next char,
	*and set the trimchars to the beggining.
	*The first time it fails to find a char in str, it just leaves.
	*/
	while(*trimptr && *strptr)
	{
		if(*strptr==*trimptr)
		{
			strptr++;
			trimptr=trimchars;
			continue;
		}
		trimptr++;
	}

	trimptr=trimchars;
	
	/*!
	*Here we push the string back to occupy the potencial
	*empty spaces created at the beginning of the string.
	*If the string was completly trimmed (if(!(*strptr))),
	*just return an empty string. 
	*If no 'holes' were created (!if(str!=strptr)) just move
	*the pointer to the tail of the string to check there now.
	*/
	if(str!=strptr)			//Holes were created
	{
		if(!(*strptr))		//Full trim
		{
			*str=0;
			return;
		}
		while(*strptr)
		{
			*str=*strptr;
			str++;
			strptr++;
		}
	}else					//No holes were created
	{
		while(*str)
			str++;
	}
	
	/*!
	*Now str-1 is exactly at the end of the string, we'll start trim from
	*there then.
	*/
	str--;
	/*!
	*Here we trim the characters of the tail of the string.
	*Just cycle through the trimchars and compare,
	*if we find a char in the str, decrement the str to
	*check the previous char, and set the trimchars to the beggining.
	*The first time it fails to find a char in str, it just leaves.
	*Note: Here we only check *trimptr in the while loop and not *str,
	*that's because i know there is at least one character in there that
	*isn't on trimchars, the character that stoped the first trim up there,
	*so that character will allways be reach before str's NULL.
	*/
	while(*trimptr)
	{
		if(*str==*trimptr)
		{
			str--;
			trimptr=trimchars;
			continue;
		}
		trimptr++;
	}
	*(str+1)=0;				//Now to finish up
}

/*!
 *Set the buffer passed to the next line.
 *A new line is the first character after \n.
 */
void gotoNextLine(char** cmd)
{
	while(*(*cmd++)!='\n')if(**cmd=='\0')break;

}

/*!
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

/*!
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

/*!
 *Convert from an hex string to an int
 */
int hexToInt(const char *str)
{
	register u_long u;
	register const char *cp;
	cp = str;
	if (*cp == '\0')
	    return 0;
	u = 0;
	while (*cp != '\0') 
	{
		if (!isxdigit((int)*cp))
			return 0;
		if (u >= 0x10000000)
		    return 0;
		u <<= 4;
		if (*cp <= '9')	
		    u += *cp++ - '0';
		else if (*cp >= 'a')
		    u += *cp++ - 'a' + 10;
		else
		    u += *cp++ - 'A' + 10;
	}
	return u;
}
/*!
 *Get the offset from string start of a character.
 */
int getCharInString(char* str, const char* characters, int max)
{
	int i, j;
	
	if(max)
	{
		for(i=0; (i<max) && (str[i]); i++ )
		{
			for(j=0;characters[j];j++)
			{
				if(str[i]==characters[j])
					return i;
			}
		}
	}else
	{	
		for(i=1; str[i]; i++ )
		{
			for(j=0;characters[j];j++)
			{
				if(str[i]==characters[j])
					return i;
			}
		}
	}
	return -1;
}

/*!
 *Get the offset from string start of the first \r or \n.
 */
int getEndLine(char* str, int max)
{
	int i;
	
	if(max)
	{
		for(i=0; (i<max) && (str[i]); i++ )
		{
			if((str[i]=='\r') || (str[i]=='\n'))
				return i;
		}
	}else
	{
		for(i=1; str[i]; i++ )
		{
			if((str[i]=='\r') || (str[i]=='\n'))
				return i;
		}
	}
	return -1;
}

#ifdef NOT_WIN 
char* strupr(char * string)
{
    unsigned int len = strlen(string);
    for(register unsigned int i = 0; i < len; i++)
       string[i] = toupper(string[i]);
    return string;
}
#endif
