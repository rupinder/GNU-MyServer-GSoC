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
#include "..\stdafx.h"
char HTTP_ERROR_MSGS[13][64]=
{
	"Bad request",					//400
	"Unauthorized",					//401
	"Execute access forbidden",		//403
	"File not found",				//404
	"Method not allowed",			//405
	"Not acceptable",				//406
	"Proxy authentication required",//407
	"Precondition failed",			//412
	"Request Entity Too Large",		//413
	"Request URI too long",			//414
	"Internal Server error",		//500
	"Not implemented",				//501
	"Bad gateway"					//502
};
char HTTP_ERROR_HTMLS[13][64]=
{
	"400.html",					//400
	"401.html",					//401
	"403.html",					//403
	"404.html",					//404
	"405.html",					//405
	"406.html",					//406
	"407.html",					//407
	"412.html",					//412
	"413.html",					//413
	"414.html",					//414
	"500.html",					//500
	"501.html",					//501
	"502.html"					//502
};