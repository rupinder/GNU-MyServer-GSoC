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


#include "../stdafx.h"
#include "../include/HTTPmsg.h"

/*!
 *This array is used to describe the errors for the HTTP protocol.
 */
char HTTP_ERROR_MSGS[29][64]=
{
	"Bad Request",						/*!400*/
	"Unauthorized",						/*!401*/
	"Forbidden",			/*!403*/
	"Not Found",					/*!404*/
	"Method Not Allowed",				/*!405*/
	"Not Acceptable",					/*!406*/
	"Proxy Authentication Required",	/*!407*/
	"Precondition Failed",				/*!412*/
	"Request Entity Too Large",			/*!413*/
	"Request-uri Too Long",				/*!414*/
	"Internal Server Error",			/*!500*/
	"Not Implemented",					/*!501*/
	"Bad Gateway",						/*!502*/
	"Service Unavailable",				/*!503*/
	"OK",								/*!200*/
	"Created",							/*!201*/			
	"Accepted",							/*!202*/			
	"Non-Authoritative Information",	/*!203*/			
	"No Content",						/*!204*/			
	"Reset Content",						/*!205*/
	"Partial Content",						/*!206*/
	"Multiple Choices",					/*!300*/	
	"Moved Permanently",				/*!301*/		
	"Found",				/*!302*/		
	"See Other",						/*!303*/			
	"Not Modified",						/*!304*/
	"Continue",						/*!100*/	
	"Gateway Timeout",					/*!504*/
	"HTTP Version Not Supported"		/*!505*/		
};

/*!
 *This array is used to describe the HTTP files for personalized errors page.
 */
char HTTP_ERROR_HTMLS[29][64]=
{
	"/errors/400.html",						/*!400*/
	"/errors/401.html",						/*!401*/
	"/errors/403.html",						/*!403*/
	"/errors/404.html",						/*!404*/
	"/errors/405.html",						/*!405*/
	"/errors/406.html",						/*!406*/
	"/errors/407.html",						/*!407*/
	"/errors/412.html",						/*!412*/
	"/errors/413.html",						/*!413*/
	"/errors/414.html",						/*!414*/
	"/errors/500.html",						/*!500*/
	"/errors/501.html",						/*!501*/
	"/errors/502.html", 					/*!502*/
	"/errors/503.html",						/*!503*/
	"/errors/200.html",						/*!200*/
	"/errors/201.html",						/*!201*/			
	"/errors/202.html",						/*!202*/			
	"/errors/203.html",						/*!203*/			
	"/errors/204.html",						/*!204*/			
	"/errors/205.html",						/*!205*/			
	"/errors/206.html",						/*!206*/				
	"/errors/300.html",						/*!300*/	
	"/errors/301.html",						/*!301*/		
	"/errors/302.html",						/*!302*/		
	"/errors/303.html",						/*!303*/			
	"/errors/304.html",						/*!304*/
	"/errors/100.html",						/*!100*/	
	"/errors/504.html",						/*!504*/
	"/errors/505.html"						/*!505*/		
};
/*!
*Return an error ID starting from an HTTP status code.
*/
int getErrorIDfromHTTPStatusCode(int statusCode)
{
	switch(statusCode)	
	{
		case 200:
			return e_200;
			break;
		case 201:
			return e_201;
			break;
		case 202:
			return e_202;
			break;
		case 203:
			return e_203;
			break;
		case 204:
			return e_204;
			break;
		case 205:
			return e_205;
			break;
		case 206:
			return e_206;
			break;
		case 300:
			return e_300;
			break;
		case 301:
			return e_301;
			break;
		case 302:
			return e_302;
			break;
		case 303:
			return e_303;
			break;
		case 304:
			return e_304;
			break;
		case 400:
			return e_400;
			break;
		case 401:
			return e_401;
			break;
		case 403:
			return e_403;
			break;
		case 404:
			return e_404;
			break;
		case 405:
			return e_405;
			break;
		case 406:
			return e_406;
			break;
		case 407:
			return e_407;
			break;
		case 412:
			return e_412;
			break;
		case 413:
			return e_413;
			break;
		case 414:
			return e_414;
			break;
		case 500:
			return e_500;
			break;
		case 501:
			return e_501;
			break;
		case 502:
			return e_502;
			break;
		case 503:
			return e_503;
			break;
		case 100:
			return e_100;
			break;
		case 504:
			return e_504;
			break;
		case 505:
			return e_505;
			break;			

	}
	return -1;
}
/*!
*Return an HTTP status code starting from an error ID.
*/
int getHTTPStatusCodeFromErrorID(int statusCode)
{
	switch(statusCode)	
	{
		case e_200:
			return 200;
			break;
		case e_201:
			return 201;
			break;
		case e_202:
			return 202;
			break;
		case e_203:
			return 203;
			break;
		case e_204:
			return 204;
			break;
		case e_205:
			return 205;
			break;
		case e_206:
			return 206;
			break;		
		case e_300:
			return 300;
			break;
		case e_301:
			return 301;
			break;
		case e_302:
			return 302;
			break;
		case e_303:
			return 303;
			break;
		case e_304:
			return 304;
			break;
		case e_400:
			return 400;
			break;
		case e_401:
			return 401;
			break;
		case e_403:
			return 403;
			break;
		case e_404:
			return 404;
			break;
		case e_405:
			return 405;
			break;
		case e_406:
			return 406;
			break;
		case e_407:
			return 407;
			break;
		case e_412:
			return 412;
			break;
		case e_413:
			return 413;
			break;
		case e_414:
			return 414;
			break;
		case e_500:
			return 500;
			break;
		case e_501:
			return 501;
			break;
		case e_502:
			return 502;
			break;
		case e_503:
			return 503;
			break;	
		case e_100:
			return 100;
			break;
		case e_504:
			return 504;
			break;	
		case e_505:
			return 505;
			break;
	}
	return -1;
}
