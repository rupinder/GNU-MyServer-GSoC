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


class http
{
private:
	httpThreadContext td;
public:
	static int sendHTTPRESOURCE(httpThreadContext*,LPCONNECTION s,char *filename,int systemrequest=0,int OnlyHeader=0,int firstByte=0,int lastByte=-1,int yetmapped=0);
	static int putHTTPRESOURCE(httpThreadContext*,LPCONNECTION s,char *filename,int systemrequest=0,int OnlyHeader=0,int firstByte=0,int lastByte=-1,int yetmapped=0);
	static int deleteHTTPRESOURCE(httpThreadContext*,LPCONNECTION s,char *filename,int yetmapped=0);
	static int sendHTTPFILE(httpThreadContext*,LPCONNECTION s,char *filenamePath,int OnlyHeader=0,int firstByte=0,int lastByte=-1);
	static int sendHTTPDIRECTORY(httpThreadContext*,LPCONNECTION s,char* folder);
	static int raiseHTTPError(httpThreadContext*,LPCONNECTION a,int ID);
	static int sendHTTPhardError500(httpThreadContext* td,LPCONNECTION a);
	static int sendAuth(httpThreadContext* td,LPCONNECTION a);
	static void getPath(httpThreadContext* td,char *filenamePath,const char *filename,int systemrequest);
	static int getMIME(char *MIME,char *filename,char *dest,char *dest2);
	static int logHTTPaccess(httpThreadContext* td,LPCONNECTION a);
	static int sendHTTPRedirect(httpThreadContext* td,LPCONNECTION a,char *newURL);
	static int sendHTTPNonModified(httpThreadContext* td,LPCONNECTION a);

	/*!
	*The main function is controlHTTPConnection(...), that parses the request builds a response.
	*/
	int controlHTTPConnection(LPCONNECTION a,char *b1,char *b2,int bs1,int bs2,u_long nbtr,u_long id);
};
#endif
