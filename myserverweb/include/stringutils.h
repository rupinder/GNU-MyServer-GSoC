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

#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include "../stdafx.h"
char *getRFC822GMTTime(char* out,int len);
char *getRFC822GMTTime(const time_t,char* out,int len);
char *getRFC822LocalTime(char* out,int len);
char *getRFC822LocalTime(const time_t,char* out,int len);
time_t getTime(char* str);
void StrTrim(char* str,const char* trimChars);
void gotoNextLine(char* cmd);
int hexVal(char c);
void translateEscapeString(char *TargetStr);
int hexToInt(const char *str);


#ifdef __linux__
void strupr(char * string);

extern "C" {
#include <string.h>
}

#define lstrcmp strcmp
#define strcmpi strcasecmp
#define lstrcmpi strcasecmp
#define _stricmp strcasecmp
#define lstrcpy strcpy
#define lstrcat strcat
#define lstrlen strlen
#endif

#endif

