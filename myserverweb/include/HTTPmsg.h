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
#pragma once
/*
*Error 401AUTH don't use any web personalized page
*/
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

extern char HTTP_ERROR_MSGS[13][64];
extern char HTTP_ERROR_HTMLS[13][64];

extern char msgSending[33];
extern char msgRunOn[33];
extern char msgFolderContents[33];
extern char msgFile[33];
extern char msgLModify[33];
extern char msgSize[33];
