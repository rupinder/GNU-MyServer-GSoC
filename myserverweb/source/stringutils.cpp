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
	if(!strcmp(lb, "Sun"))
		t.tm_wday = 0;
	if(!strcmp(lb, "Mon"))
		t.tm_wday = 1;
	if(!strcmp(lb, "Tue"))
		t.tm_wday = 2;
	if(!strcmp(lb, "Wed"))
		t.tm_wday = 3;
	if(!strcmp(lb, "Thu"))
		t.tm_wday = 4;
	if(!strcmp(lb, "Fri"))
		t.tm_wday = 5;
	if(!strcmp(lb, "Sat"))
		t.tm_wday = 6;

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
	if(!strcmp(lb, "Jan"))
		t.tm_mon = 0;
	if(!strcmp(lb, "Feb"))
		t.tm_mon = 1;
	if(!strcmp(lb, "Mar"))
		t.tm_mon = 2;
	if(!strcmp(lb, "Apr"))
		t.tm_mon = 3;
	if(!strcmp(lb, "May"))
		t.tm_mon = 4;
	if(!strcmp(lb, "Jun"))
		t.tm_mon = 5;
	if(!strcmp(lb, "Jul"))
		t.tm_mon = 6;	
	if(!strcmp(lb, "Aug"))
		t.tm_mon = 7;
	if(!strcmp(lb, "Sep"))
		t.tm_mon = 8;
	if(!strcmp(lb, "Oct"))
		t.tm_mon = 9;
	if(!strcmp(lb, "Nov"))
		t.tm_mon = 10;
	if(!strcmp(lb, "Dec"))
		t.tm_mon = 11;

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
*Trim the string str by the characters trimchars.
*/
void StrTrim(char* str, const char* trimchars)
{
	u_short lenTrimchars=(u_short)strlen(trimchars);
	u_short lenStr=(u_short)strlen(str);
	u_long continueLoop;
	/*!
	*Number of characters to remove from the initial position of the string.
	*/
	u_short ncharToRemove=0;
	if(lenStr==0)
		return;
	if(lenTrimchars==0)
		return;
	u_long j;
	for(j=0;j<=lenStr;j++)
	{
		continueLoop=0;
		for(int i=0;i<lenTrimchars;i++)
		{
			if(str[j]==trimchars[i])
			{
				ncharToRemove++;
				continueLoop=1;
				break;
			}
		}
		if(!continueLoop)
			break;
	}
	if(ncharToRemove)
	{
		u_long strLen=(u_long)strlen(str)+1;
		for( j=0  ;  j  < strLen ; j++ )
			str[j] = str[j+ncharToRemove];
	}
	for( j=(u_long)(strlen(str)-1) ; j ; j-- )
	{
		continueLoop=0;
		for(int i=0;i<lenTrimchars;i++)
		{
			if(str[j]==trimchars[i])
			{
				str[j]='\0';
				continueLoop=1;
				break;
			}
		}
		if(!continueLoop)
			break;
	}
}

/*!
*Set the buffer passed to the next line.
*A new line is the first character after \n.
*/
void gotoNextLine(char* cmd)
{
	while(*cmd++!='\n')if(*cmd=='\0')break;

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
	int nChars=(int)strlen(characters);
	int i;
	register int j;
	for(i=0;(max ? i<max : 1) && str[i] ; i++ )
	{
		for(j=0;j<nChars;j++)
		{
			if(str[i]==characters[j])
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
