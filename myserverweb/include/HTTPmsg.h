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
#ifndef HTTPMSG_H
#define HTTPMSG_H
/*
*Error over 1000 are warnings.
*/
#define e_200			1001
#define e_201			1002
#define e_202			1003
#define e_301			1004
#define e_302			1005
#define e_304			1006
#define e_400			0
#define e_401			1
#define e_401AUTH		1004
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

extern char HTTP_ERROR_MSGS[13][64];
extern char HTTP_ERROR_HTMLS[13][64];
int getErrorIDfromHTTPStatusCode(int statusCode);
int getHTTPStatusCodeFromErrorID(int statusCode);
#endif
