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

#ifndef HTTP_H
#define HTTP_H
#include "../stdafx.h"
#include "../include/http_headers.h"
#include "../include/cgi.h"
#include "../include/wincgi.h"
#include "../include/fastcgi.h"
#include "../include/mscgi.h"
#include "../include/isapi.h"
#include "../include/cXMLParser.h"
class http
{
private:
	struct httpThreadContext td;
	mscgi lmscgi;
	wincgi lwincgi;
	isapi lisapi;
	cgi lcgi;
	fastcgi lfastcgi;	
public:
	int sendHTTPRESOURCE(httpThreadContext*,LPCONNECTION s,char *filename,int systemrequest=0,int OnlyHeader=0,int firstByte=0,int lastByte=-1,int yetmapped=0);
	int putHTTPRESOURCE(httpThreadContext*,LPCONNECTION s,char *filename,int systemrequest=0,int OnlyHeader=0,int firstByte=0,int lastByte=-1,int yetmapped=0);
	int deleteHTTPRESOURCE(httpThreadContext*,LPCONNECTION s,char *filename,int yetmapped=0);
	int sendHTTPFILE(httpThreadContext*,LPCONNECTION s,char *filenamePath,int OnlyHeader=0,int firstByte=0,int lastByte=-1);
	int sendHTTPDIRECTORY(httpThreadContext*,LPCONNECTION s,char* folder);
	int raiseHTTPError(httpThreadContext*,LPCONNECTION a,int ID);
	int sendHTTPhardError500(httpThreadContext* td,LPCONNECTION a);
	int sendAuth(httpThreadContext* td,LPCONNECTION a);
	void getPath(httpThreadContext* td,char *filenamePath,const char *filename,int systemrequest);
	int getMIME(char *MIME,char *filename,char *dest,char *dest2);
	int logHTTPaccess(httpThreadContext* td,LPCONNECTION a);
	int sendHTTPRedirect(httpThreadContext* td,LPCONNECTION a,char *newURL);
	int sendHTTPNonModified(httpThreadContext* td,LPCONNECTION a);

	/*!
	*The function is used to the request and build a response.
	*/
	int controlHTTPConnection(LPCONNECTION a,char *b1,char *b2,int bs1,int bs2,u_long nbtr,u_long id);
	static int loadProtocol(cXMLParser*);
	static int unloadProtocol(cXMLParser*);

};
#endif
