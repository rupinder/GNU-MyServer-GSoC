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
#include "../include/threads.h"
#include "../include/http_data_handler.h"

/*!
 * Listening socket file number
 */
#define FcgiLISTENSOCK_FILENO 0

typedef struct {
    unsigned char version;
    unsigned char type;
    unsigned char requestIdB1;
    unsigned char requestIdB0;
    unsigned char contentLengthB1;
    unsigned char contentLengthB0;
    unsigned char paddingLength;
    unsigned char reserved;
} FcgiHeader;

#define FcgiMAX_LENGTH 0xffff

/*!
 * Number of bytes in a FcgiHeader.  Future versions of the protocol
 * will not reduce this number.
 */
#define FcgiHEADER_LEN  8

/*!
 * Value for version component of FcgiHeader
 */
#define FcgiVERSION_1           1

/*!
 * Current version of the FastCGI protocol
 */
#define FcgiVERSION FcgiVERSION_1

/*!
 * Values for type component of FcgiHeader
 */
#define FcgiBEGIN_REQUEST       1
#define FcgiABORT_REQUEST       2
#define FcgiEND_REQUEST         3
#define FcgiPARAMS              4
#define FcgiSTDIN               5
#define FcgiSTDOUT              6
#define FcgiSTDERR              7
#define FcgiDATA                8
#define FcgiGET_VALUES          9
#define FcgiGET_VALUES_RESULT  10
#define FcgiUNKNOWN_TYPE       11
#define FcgiMAXTYPE (FcgiUNKNOWN_TYPE)

/*!
 * Value for requestId component of FcgiHeader
 */
#define FcgiNULL_REQUEST_ID     0


typedef struct {
    unsigned char roleB1;
    unsigned char roleB0;
    unsigned char flags;
    unsigned char reserved[5];
} FcgiBeginRequestBody;

typedef struct {
    FcgiHeader header;
    FcgiBeginRequestBody body;
} FcgiBeginRequestRecord;

/*!
 * Mask for flags component of FcgiBeginRequestBody
 */
#define FcgiKEEP_CONN  1

/*!
 * Values for role component of FcgiBeginRequestBody
 */
#define FcgiRESPONDER  1
#define FcgiAUTHORIZER 2
#define FcgiFILTER     3


typedef struct {
    unsigned char appStatusB3;
    unsigned char appStatusB2;
    unsigned char appStatusB1;
    unsigned char appStatusB0;
    unsigned char protocolStatus;
    unsigned char reserved[3];
} FcgiEndRequestBody;

typedef struct {
    FcgiHeader header;
    FcgiEndRequestBody body;
} FcgiEndRequestRecord;

/*!
 * Values for protocolStatus component of FcgiEndRequestBody
 */
#define FcgiREQUEST_COMPLETE 0
#define FcgiCANT_MPX_CONN    1
#define FcgiOVERLOADED       2
#define FcgiUNKNOWN_ROLE     3

/*!
*Max number of FastCGI server allowed to run
*/
#define MAX_FcgiSERVERS	25

/*!
 * Variable names for FcgiGET_VALUES / FcgiGET_VALUES_RESULT records
 */
#define FcgiMAX_CONNS  "FcgiMAX_CONNS"
#define FcgiMAX_REQS   "FcgiMAX_REQS"
#define FcgiMPXS_CONNS "FcgiMPXS_CONNS"


typedef struct {
    unsigned char type;    
    unsigned char reserved[7];
} FcgiUnknownTypeBody;

typedef struct {
    FcgiHeader header;
    FcgiUnknownTypeBody body;
} FcgiUnknownTypeRecord;

struct sfCGIservers
{
  /*! Server executable path. */
	char *path;
	union 
	{
    unsigned long fileHandle;
		SOCKET sock;
		unsigned int value;
	}DESCRIPTOR;
	Socket socket;
	char host[128];
	/*! Process ID.  */ 
	int pid; 
	/*! IP port.  */
	u_short port;
  sfCGIservers* next;
};

struct fCGIContext
{
	HttpThreadContext* td;
  sfCGIservers* server;
	Socket sock;
	File tempOut;
};

class FastCgi  : public HttpDataHandler
{
private:
	static int timeout;
  static  int max_fcgi_servers;
	static int initialized;
  static Mutex servers_mutex;
	static struct sfCGIservers *fCGIservers;

	/*! Number of thread currently loaded.  */
	static int fCGIserversN;

	int FcgiConnectSocket(fCGIContext*,sfCGIservers*);
	void generateFcgiHeader( FcgiHeader&, int ,int, int );
	Socket getFcgiConnection();
	int buildFASTCGIEnvironmentString(HttpThreadContext*,char*,char*);
	int sendFcgiBody(fCGIContext* con,char* buffer,int len,int type,int id);
	sfCGIservers* isFcgiServerRunning(char*);
  sfCGIservers* runFcgiServer(fCGIContext*,char*);
	sfCGIservers* FcgiConnect(fCGIContext*,char*);

public:
  static int getTimeout();
  static void setTimeout(int);
  static void setMaxFcgiServers(int);
  static int getMaxFcgiServers();
	FastCgi();
	static int load();
	int send(HttpThreadContext* td, ConnectionPtr connection,
                  char* scriptpath,char *cgipath,int execute,
                  int onlyHeader);
	static int unload();
};
#endif
