/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008, 2009 Free Software Foundation, Inc.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License,  or
(at your option) any later version.

This program is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "stdafx.h"
#include <include/base/string/stringutils.h>
#include <include/base/safetime/safetime.h>

extern "C" {
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifndef WIN32
#include <stdio.h>
#endif
}

#ifdef GETTIMEOFDAY
#include <sys/time.h>
#endif

#ifdef WIN32
#include <direct.h>
#include <time.h>
#endif

/*!
 *This function format current time to the RFC 822 format and output 
 *it to a string.
 */
const char *getRFC822GMTTime(string& out,int len)
{
  time_t ltime;
  time( &ltime );
  return getRFC822GMTTime(ltime, out, len);
}

/*!
 *This function format current time to the RFC 822 format and output 
 *it to a string.
 */
const char *getRFC822GMTTime(const time_t t, string& out, int len)
{
  /*! 
   *FIXME: Do not use a temporary buffer but access directly the string
   *class data. 
   */
  char buff[32];
  getRFC822GMTTime(t, buff, 32);
  out.assign(buff);
  return out.c_str();
}

/*!
 *This function format current time to the RFC 822 format and output 
 *it to a string.
 */
const char *getRFC822LocalTime(string& out,int len)
{
  time_t ltime;
  time( &ltime );
  return getRFC822LocalTime(ltime, out, len);
}

/*!
 *This function format current time to the RFC 822 format and output 
 *it to a string.
 */
const char *getRFC822LocalTime(const time_t t, string &out,int len)
{
  /*! 
   *FIXME: Do not use a temporary buffer but access directly the string
   *class data. 
   */
  char buff[32];
  getRFC822LocalTime(t, buff, 32);
  out.assign(buff);
  return out.c_str();
}

/*!
 *This function format current time to the RFC 822 format.
 */
const char *getRFC822GMTTime(char* out, int len)
{
  time_t ltime;
  time( &ltime );
  return getRFC822GMTTime(ltime, out, len);
}
/*!
 *This function formats a time to the RFC 822 format.
 */
const char *getRFC822GMTTime(const time_t ltime, char* out, int /*!len*/)
{
  char *asct;
  u_long ind = 0;
  tm* GMtime = gmtime( &ltime );
  GMtime->tm_year += 1900;
  asct=asctime(GMtime);
  out[ind++]=asct[0];
  out[ind++]=asct[1];
  out[ind++]=asct[2];
  out[ind++]=',';

  if(asct[8] != ' ')
    out[ind++]=' ';

  out[ind++]=asct[8];
  out[ind++]=asct[9];
  out[ind++]=' ';
  out[ind++]=asct[4];
  out[ind++]=asct[5];
  out[ind++]=asct[6];
  out[ind++]=' ';

  sprintf(&out[ind], "%i", GMtime->tm_year);
  ind += 4;

  out[ind++]=' ';
  out[ind++]=asct[11];
  out[ind++]=asct[12];
  out[ind++]=':';
  out[ind++]=asct[14];
  out[ind++]=asct[15];
  out[ind++]=':';
  out[ind++]=asct[17];
  out[ind++]=asct[18];
  out[ind++]=' ';
  out[ind++]='G';
  out[ind++]='M';
  out[ind++]='T';
  out[ind++]='\0';
  out[ind++]='\0';
  out[ind++]='\0';
  return out;
}
/*!
 *This function convert from a RFC 822 format to a time_t.
 */
time_t getTime(const char* str)
{
  char lb[30];
  int c = 0;
  int i;
  tm t;
  for(i = 0; i < 30; i++)
  {
    if(str[c] == ',')
    {
      c++;
      lb[i] = '\0';
      break;
    }
    else
      lb[i] = str[c++];
  }
  i = lb[0] + lb[1] + lb[3];
  switch(i)
  {
    case 310:
      t.tm_wday = 0;  //Sun
      break;
    case 298:
      t.tm_wday = 1;  //Mon
      break;
    case 302:
      t.tm_wday = 2;  //Tue
      break;
    case 288:
      t.tm_wday = 3;  //Wed
      break;
    case 305:
      t.tm_wday = 4;  //Thu
      break;
    case 289:
      t.tm_wday = 5;  //Fri
      break;
    case 296:
      t.tm_wday = 6;  //Sat
      break;
  }

  c++;
  for(i = 0; i < 30; i++)
  {
    if(i && str[c]==' ')
    {
      c++;
      lb[i] = '\0';
      break;
    }
    else
      lb[i] = str[c++];
  }
  t.tm_mday = atoi(lb);
  
  for(i = 0; i < 30; i++)
  {
    if(i && str[c] == ' ')
    {
      c++;
      lb[i] = '\0';
      break;
    }
    else
      lb[i] = str[c++];
  }
  i = lb[0] + lb[1] + lb[3];
  switch(i)
  {
    case 281:
      t.tm_wday = 0;  //Jan
      break;
    case 269:
      t.tm_wday = 1;  //Feb
      break;
    case 288:
      t.tm_wday = 2;  //Mar
      break;
    case 291:
      t.tm_wday = 3;  //Apr
      break;
    case 295:
      t.tm_wday = 4;  //May
      break;
    case 301:
      t.tm_wday = 5;  //Jun
      break;
    case 299:
      t.tm_wday = 6;  //Jul
      break;
    case 285:
      t.tm_wday = 7;  //Aug
      break;
    case 296:
      t.tm_wday = 8;  //Sep
      break;
    case 294:
      t.tm_wday = 9;  //Oct
      break;
    case 307:
      t.tm_wday = 10;  //Nov
      break;
    case 268:
      t.tm_wday = 11;  //Dec
      break;
  }

  for(i = 0; i < 30; i++)
  {
    if(i && str[c]==' ')
    {
      c++;
      lb[i] = '\0';
      break;
    }
    else
      lb[i] = str[c++];
  }
  t.tm_year = atoi(lb) - 1900;
  
  for(i = 0; i < 30; i++)
  {
    if(i && str[c] == ':')
    {
      c++;
      lb[i] = '\0';
      break;
    }
    else
      lb[i] = str[c++];
  }
  t.tm_hour = atoi(lb);

  for(i = 0; i < 30; i++)
  {
    if(i && str[c] == ':')
    {
      c++;
      lb[i] = '\0';
      break;
    }
    else
      lb[i] = str[c++];
  }
  t.tm_min = atoi(lb);

  for(i = 0; i < 30; i++)
  {
    if(i && str[c]==':')
    {
      c++;
      lb[i] = '\0';
      break;
    }
    else
      lb[i] = str[c++];
  }
  t.tm_sec = atoi(lb);
  t.tm_yday = 0;
  t.tm_wday = 0;  

  t.tm_isdst = -1;

  return mktime(&t);
}

/*!
 *This function format current time to the RFC 822 format.
 */
const char *getRFC822LocalTime(char* out, int len)
{
  time_t ltime;
  time( &ltime );
  return getRFC822LocalTime(ltime, out, len);
}
/*!
 *This function formats a time to the RFC 822 format.
 */
const char *getRFC822LocalTime(const time_t ltime, char* out, int /*!len*/)
{
  char *asct;
  tm result;
  u_long ind = 0;
  
  myserver_localtime( &ltime, &result );

  result.tm_year += 1900;
  asct = asctime(&result);
  out[ind++] = asct[0];
  out[ind++] = asct[1];
  out[ind++] = asct[2];
  out[ind++] = ',';

  if(asct[8] != ' ')
    out[ind++] = ' ';

  out[ind++] = asct[8];
  out[ind++] = asct[9];
  out[ind++] = ' ';
  out[ind++] = asct[4];
  out[ind++] = asct[5];
  out[ind++] = asct[6];
  out[ind++] = ' ';

  sprintf(&out[ind], "%i", result.tm_year);
  ind += 4;

  out[ind++] = ' ';
  out[ind++] = asct[11];
  out[ind++] = asct[12];
  out[ind++] = ':';
  out[ind++] = asct[14];
  out[ind++] = asct[15];
  out[ind++] = ':';
  out[ind++] = asct[17];
  out[ind++] = asct[18];
  out[ind++] = ' ';
  out[ind++] = '\0';
  out[ind++] = '\0';
  out[ind++] = '\0';
  out[ind++] = '\0';
  out[ind++] = '\0';
  out[ind++] = '\0';
  return out;
}

/*!
 *Get the local time string.
 */
const char* getLocalLogFormatDate(char* out, int len)
{
  time_t ltime;
  time( &ltime );
  return getLocalLogFormatDate(ltime, out, len);
}

/*!
 *Get the GMT time string.
 */
const char* getGMTLogFormatDate(char* out, int len)
{  
  time_t ltime;
  time( &ltime );
  return getGMTLogFormatDate(ltime, out, len);
}


/*!
 *Get the local time string.
 */
const char* getLocalLogFormatDate(const time_t t, string& out, int len)
{
  char buff[32];
  getLocalLogFormatDate(t, buff, len);
  out.assign(buff);
  return out.c_str();
}

/*!
 *Get the GMT time string.
 */
const char* getGMTLogFormatDate(const time_t t, string& out, int len)
{
  char buff[32];
  getGMTLogFormatDate(t, buff, len);
  out.assign(buff);
  return out.c_str();
}
/*!
 *Get the local time string.
 */
const char* getLocalLogFormatDate(string& out, int len)
{
  char buff[32];
  getLocalLogFormatDate(buff, len);
  out.assign(buff);
  return out.c_str();
}

/*!
 *Get the GMT time string.
 */
const char* getGMTLogFormatDate(string& out, int len)
{
  char buff[32];
  getGMTLogFormatDate(buff, len);
  out.assign(buff);
  return out.c_str();
}

/*!
 *Get a string in the format "day/month/year:hour:minute:second offset" 
 *for the local zone.
 */
const char* getLocalLogFormatDate(const time_t t, char* out, int len)
{
  int offset = 0;
  time_t ltime;
  time( &ltime );
  char *asct;
  tm GMtime;
  myserver_localtime( &ltime, &GMtime );
  if(len < 25)
    return 0;
  GMtime.tm_year += 1900;
  asct = asctime(&GMtime);
   out[0] = asct[8] != ' ' ? asct[8] : '0';
  out[1] = asct[9];
  out[2] = '/';
  out[3] = asct[4];
  out[4]= asct[5];
  out[5] = asct[6];
  out[6] = '/';
  sprintf(&out[7], "%i", GMtime.tm_year);
  out[11] = ':';
  out[12] = asct[11];
  out[13] = asct[12];
  out[14] = ':';
  out[15] = asct[14];
  out[16] = asct[15];
  out[17] = ':';
  out[18] = asct[17];
  out[19] = asct[18];
  out[20] = ' ';

#ifndef WIN32
  extern long timezone; 
  offset = -timezone;
#else

#ifdef GETTIMEOFDAY
  struct timeval tv;
  struct timezone tz;

  gettimeofday(&tv, &tz);
  offset = -tz.tz_minuteswest * 60;

#else
  TIME_ZONE_INFORMATION tzi;
  GetTimeZoneInformation(&tzi);
  offset = -tzi.Bias * 60;
#endif

#endif

  if(offset < 0)
  {
    offset = -offset;
    out[21] = '-';
  }
  else
    out[21] = '+';
  sprintf(&out[22], "%.2i%.2i", offset / (60 * 60), offset % (60 * 60) / 60);
  out[26] = '\0';
  return out;
}


/*!
 *Get a string in the format "day/month/year:hour:minute:second offset" 
 *for the GMT zone.
 */
const char* getGMTLogFormatDate(const time_t t, char* out, int len)
{
  time_t ltime;
  time( &ltime );
  char *asct;
  tm result;

  myserver_localtime( &ltime, &result );

  if(len < 25)
    return 0;

  result.tm_year += 1900;
  asct = asctime(&result);
   out[0] = asct[8] != ' ' ? asct[8] : '0';
  out[1] = asct[9];
  out[2] = '/';
  out[3] = asct[4];
  out[4] = asct[5];
  out[5] = asct[6];
  out[6] = '/';
  sprintf(&out[7], "%i", result.tm_year);
  out[11] = ':';
  out[12] = asct[11];
  out[13] = asct[12];
  out[14] = ':';
  out[15] = asct[14];
  out[16] = asct[15];
  out[17] = ':';
  out[18] = asct[17];
  out[19] = asct[18];
  out[20] = ' ';
  out[21] = '+';
  out[22] = '0';
  out[23] = '0';
  out[24] = '0';
  out[25] = '0';
  out[26] = '\0';
  return out;
}

/*!
 *Trim a string from the right side.
 */
string trimRight(string const &s, string const &t )
{
  string str = s;
  return str.erase(str.find_last_not_of(t) + 1 ) ;
}


/*!
 *Trim a string from the left side.
 */
string trimLeft(string const &s, string const &t )
{
  std::string str = s;
  return str.erase( 0, s.find_first_not_of (t)) ;
}

/*!
 *Trim a string.
 */
string trim(string const &s,  string const &t)
{
  return trimLeft(trimRight(s, t));
}

/*!
 *This funtions takes two strings, first the str we're going to work on,
 *and second a list of characters that the funtion is going to remove 
 *from the head and tail of the first string.
 *Ex:       char str[16]="Hellow World!!!";
 *          char trim[7]="e!HlwW";
 *          StrTrim(str,trim);
 *result:    str="ow World"
 *'w', 'W' and the last 'l' aren't removed because they aren't 
 *attached to the head or tail of the string
 */

void StrTrim(char* str, char* trimchars)
{
  char *strptr = str;
  char *trimptr = trimchars;
  
  /*!
  *Here we trim the characters of the head of the string.
  *Just cycle through the trimchars and compare,
  *if we find a char in str, increment the str to check the next char,
  *and set the trimchars to the beggining.
  *The first time it fails to find a char in str, it just leaves.
  */
  while(*trimptr && *strptr)
  {
    if(*strptr == *trimptr)
    {
      strptr++;
      trimptr = trimchars;
      continue;
    }
    trimptr++;
  }

  trimptr = trimchars;
  
  /*!
  *Here we push the string back to occupy the potencial
  *empty spaces created at the beginning of the string.
  *If the string was completly trimmed (if(!(*strptr))),
  *just return an empty string. 
  *If no 'holes' were created (!if(str!=strptr)) just move
  *the pointer to the tail of the string to check there now.
  */
  if(str != strptr)      //Holes were created
  {
    if(!(*strptr))    //Full trim
    {
      *str = 0;
      return;
    }
    while(*strptr)
    {
      *str = *strptr;
      str++;
      strptr++;
    }
  }else          //No holes were created
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
    if(*str == *trimptr)
    {
      str--;
      trimptr = trimchars;
      continue;
    }
    trimptr++;
  }
  *(str + 1) = 0;        //Now to finish up
}

/*!
 *Set the buffer passed to the next line.
 *A new line is the first character after \n.
 */
void gotoNextLine(char** cmd)
{
  while(*(*cmd++) != '\n')
    if(**cmd == '\0')
      break;

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
    if ((str[i] == '%') && (str[i + 1] != 0) && (str[i + 2] != 0))
    {
      str[j] =(char) (16 * hexVal(str[i + 1]) + hexVal(str[i + 2]));
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
 *Translates HTTP escape sequences.
 */
void translateEscapeString(string& str)
{
  int i, j, len;
  i = 0;
  j = 0;
  len = str.length();
  while (len--)
  {
    if ((str[i] == '%') && (str[i + 1] != 0) && (str[i + 2] != 0))
    {
      str[j] =(char) (16 * hexVal(str[i + 1]) + hexVal(str[i + 2]));
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
int getCharInString(const char* str, const char* characters, int max)
{
  int i, j;
  
  if(max)
  {
    for(i = 0; (i < max) && (str[i]); i++ )
    {
      for(j = 0; characters[j]; j++)
      {
        if(str[i] == characters[j])
          return i;
      }
    }
  }else
  {  
    for(i = 0; str[i]; i++ )
    {
      for(j = 0; characters[j]; j++)
      {
        if(str[i] == characters[j])
          return i;
      }
    }
  }
  return -1;
}

/*!
 *Get the offset to the first '\\r' or '\\n'.
 *Returns -1 on errors.
 */
int getEndLine(const char* str, int max)
{
  int i;
  
  if(max)
  {
    for(i = 0; (i < max) && (str[i]); i++ )
    {
      if((str[i]=='\r') || (str[i]=='\n'))
        return i;
    }
  }else
  {
    for(i = 0; str[i]; i++ )
    {
      if((str[i] == '\r') || (str[i] == '\n'))
        return i;
    }
  }
  return -1;
}

/*!
 *Compares two strings and returns a value indicating their relationship. 
 *This function ignores case.
 *Returns = 0 if [A] is identical to [B].
 *Returns < 0 if [A] is less than [B].
 *Returns > 0 if [A] is greater than [B].
 */
int stringcmpi(string const &a, string const &b)
{
  return strcmpi(a.c_str(), b.c_str());
}

/*!
 *Compares two strings and returns a value indicating their relationship. 
 *This function ignores case.
 *Returns = 0 if [A] is identical to [B].
 *Returns < 0 if [A] is less than [B].
 *Returns > 0 if [A] is greater than [B].
 */
int stringcmpi(string const &a, const char* b)
{
  return strcmpi(a.c_str(), b);
}

/*!
 *Compares two strings and returns a value indicating their relationship. 
 *This function values the strings case too.
 *Returns = 0 if [A] is identical to [B].
 *Returns < 0 if [A] is less than [B].
 *Returns > 0 if [A] is greater than [B].
 */
int stringcmp(const string& a, const string& b)
{
  return strcmp(a.c_str(), b.c_str());
}

/*!
 *Compares two strings and returns a value indicating their relationship. 
 *This function values the strings case too.
 *Returns = 0 if [A] is identical to [B].
 *Returns < 0 if [A] is less than [B].
 *Returns > 0 if [A] is greater than [B].
 */
int stringcmp(string const &a, const char* b)
{
  return strcmp(a.c_str(), b);
}

#ifndef WIN32 
char* strupr(char * s)
{
    unsigned int len = strlen(s);
    for(register unsigned int i = 0; i < len; i++)
       s[i] = toupper(s[i]);
    return s;
}
#endif
