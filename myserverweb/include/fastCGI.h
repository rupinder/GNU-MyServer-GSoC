/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
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

#ifndef FASTCGI_H
#define FASTCGI_H

#include "../stdafx.h"
#include "../include/http_headers.h"
#include "../include/utility.h"
#include "../include/sockets.h"
#include "../include/vhosts.h"
#include "../include/HTTPmsg.h"
#include "../include/connectionstruct.h"
#include "../include/stringutils.h"


/*!
 * Listening socket file number
 */
#define FCGI_LISTENSOCK_FILENO 0

typedef struct {
    unsigned char version;
    unsigned char type;
    unsigned char requestIdB1;
    unsigned char requestIdB0;
    unsigned char contentLengthB1;
    unsigned char contentLengthB0;
    unsigned char paddingLength;
    unsigned char reserved;
} FCGI_Header;

#define FCGI_MAX_LENGTH 0xffff

/*!
 * Number of bytes in a FCGI_Header.  Future versions of the protocol
 * will not reduce this number.
 */
#define FCGI_HEADER_LEN  8

/*!
 * Value for version component of FCGI_Header
 */
#define FCGI_VERSION_1           1

/*!
 * Current version of the FastCGI protocol
 */
#define FCGI_VERSION FCGI_VERSION_1

/*!
 * Values for type component of FCGI_Header
 */
#define FCGI_BEGIN_REQUEST       1
#define FCGI_ABORT_REQUEST       2
#define FCGI_END_REQUEST         3
#define FCGI_PARAMS              4
#define FCGI_STDIN               5
#define FCGI_STDOUT              6
#define FCGI_STDERR              7
#define FCGI_DATA                8
#define FCGI_GET_VALUES          9
#define FCGI_GET_VALUES_RESULT  10
#define FCGI_UNKNOWN_TYPE       11
#define FCGI_MAXTYPE (FCGI_UNKNOWN_TYPE)

/*!
 * Value for requestId component of FCGI_Header
 */
#define FCGI_NULL_REQUEST_ID     0


typedef struct {
    unsigned char roleB1;
    unsigned char roleB0;
    unsigned char flags;
    unsigned char reserved[5];
} FCGI_BeginRequestBody;

typedef struct {
    FCGI_Header header;
    FCGI_BeginRequestBody body;
} FCGI_BeginRequestRecord;

/*!
 * Mask for flags component of FCGI_BeginRequestBody
 */
#define FCGI_KEEP_CONN  1

/*!
 * Values for role component of FCGI_BeginRequestBody
 */
#define FCGI_RESPONDER  1
#define FCGI_AUTHORIZER 2
#define FCGI_FILTER     3


typedef struct {
    unsigned char appStatusB3;
    unsigned char appStatusB2;
    unsigned char appStatusB1;
    unsigned char appStatusB0;
    unsigned char protocolStatus;
    unsigned char reserved[3];
} FCGI_EndRequestBody;

typedef struct {
    FCGI_Header header;
    FCGI_EndRequestBody body;
} FCGI_EndRequestRecord;

/*!
 * Values for protocolStatus component of FCGI_EndRequestBody
 */
#define FCGI_REQUEST_COMPLETE 0
#define FCGI_CANT_MPX_CONN    1
#define FCGI_OVERLOADED       2
#define FCGI_UNKNOWN_ROLE     3

/*!
*Max number of FastCGI server allowed to run
*/
#define MAX_FCGI_SERVERS	25

/*!
 * Variable names for FCGI_GET_VALUES / FCGI_GET_VALUES_RESULT records
 */
#define FCGI_MAX_CONNS  "FCGI_MAX_CONNS"
#define FCGI_MAX_REQS   "FCGI_MAX_REQS"
#define FCGI_MPXS_CONNS "FCGI_MPXS_CONNS"


typedef struct {
    unsigned char type;    
    unsigned char reserved[7];
} FCGI_UnknownTypeBody;

typedef struct {
    FCGI_Header header;
    FCGI_UnknownTypeBody body;
} FCGI_UnknownTypeRecord;

struct fCGIContext
{
	httpThreadContext* td;
	int fcgiPID;
	MYSERVER_SOCKET sock;
	MYSERVER_FILE tempOut;
};
struct sfCGIservers
{
	char *path;/*!server executable path*/
	union 
	{
	    unsigned long fileHandle;
		SOCKET sock;
		unsigned int value;
	}DESCRIPTOR;
	MYSERVER_SOCKET socket;
	char host[128];
	/*! Process ID.  */ 
	int pid; 
	/*! IP port.  */
	u_short port;
};


class fastcgi
{
private:
	static int initialized;
	static struct sfCGIservers fCGIservers[MAX_FCGI_SERVERS];
	/*! Number of thread currently loaded.  */
	static int fCGIserversN;
	int FcgiConnectSocket(fCGIContext*,int);
	void generateFcgiHeader( FCGI_Header&, int ,int, int );
	MYSERVER_SOCKET getFcgiConnection();
	int buildFASTCGIEnvironmentString(httpThreadContext*,char*,char*);
	int sendFcgiBody(fCGIContext* con,char* buffer,int len,int type,int id);
	int isFcgiServerRunning(char*);
	int runFcgiServer(fCGIContext*,char*);
	int FcgiConnect(fCGIContext*,char*);
public:
	fastcgi();
	static int initializeFASTCGI();
	int sendFASTCGI(httpThreadContext* td,LPCONNECTION connection,
                  char* scriptpath,char* /*!ext*/,char *cgipath,int execute,
                  int onlyHeader);
	static int cleanFASTCGI();
};
#endif
