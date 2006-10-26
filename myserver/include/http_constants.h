/*
MyServer
Copyright (C) 2002, 2003, 2004 The MyServer Team
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef HTTPMSG_H
#define HTTPMSG_H

#define e_200			14
#define e_201			15
#define e_202			16
#define e_203			17
#define e_204			18
#define e_205			19
#define e_206			20
#define e_300			21
#define e_301			22
#define e_302			23
#define e_303			24
#define e_304			25
#define e_100			26
#define e_400			0
#define e_401			1
#define e_401AUTH		1001
#define e_403			2
#define e_404			3
#define e_405			4
#define e_406			5
#define e_407			6
#define e_412			7
#define e_413			8 
#define e_414			9
#define e_500			10
#define e_501			11
#define e_502			12
#define e_503			13
#define e_504			27
#define e_505			28

extern char HTTP_ERROR_MSGS[29][64];
extern char HTTP_ERROR_HTMLS[29][64];
int getErrorIDfromHTTPStatusCode(int statusCode);
int getHTTPStatusCodeFromErrorID(int statusCode);
#endif
