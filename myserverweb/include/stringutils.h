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
#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include "../stdafx.h"
char *getRFC822GMTTime(void);
char *getRFC822GMTTime(const time_t);
char *getRFC822LocalTime(void);
char *getRFC822LocalTime(const time_t);
void StrTrim(char* str,const char* trimChars);
void getFileExt(char* ext,const char* filename);
void gotoNextLine(char* cmd);
int hexVal(char c);
void translateEscapeString(char *TargetStr);
void splitPath(const char* path, char* dir, char*filename);
void getFilename(const char* path, char* filename);
#endif
