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
#include "../include/hash_dictionary.h"
#include <string>

using namespace std;

/*!
 *Listening socket file number.
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
} FcgiHeader;

#define FCGIMAX_LENGTH 0xffff

/*!
 *Number of bytes in a FcgiHeader.  Future versions of the protocol
 *will not reduce this number.
 */
#define FCGIHEADER_LEN  8

/*!
 *Value for version component of FcgiHeader.
 */
#define FCGIVERSION_1           1

/*!
 *Current version of the FastCGI protocol.
 */
#define FCGIVERSION FCGIVERSION_1

/*!
 *Values for type component of FcgiHeader
 */
#define FCGIBEGIN_REQUEST       1
#define FCGIABORT_REQUEST       2
#define FCGIEND_REQUEST         3
#define FCGIPARAMS              4
#define FCGISTDIN               5
#define FCGISTDOUT              6
#define FCGISTDERR              7
#define FCGIDATA                8
#define FCGIGET_VALUES          9
#define FCGIGET_VALUES_RESULT  10
#define FCGIUNKNOWN_TYPE       11
#define FCGIMAXTYPE (FcgiUNKNOWN_TYPE)

/*!
 *Value for requestId component of FcgiHeader.
 */
#define FCGINULL_REQUEST_ID     0


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
 *Mask for flags component of FcgiBeginRequestBody.
 */
#define FCGIKEEP_CONN  1

/*!
 *Values for role component of FcgiBeginRequestBody.
 */
#define FCGIRESPONDER  1
#define FCGIauthORIZER 2
#define FCGIFILTER     3


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
 *Values for protocolStatus component of FcgiEndRequestBody.
 */
#define FCGIREQUEST_COMPLETE 0
#define FCGICANT_MPX_CONN    1
#define FCGIOverLOADED       2
#define FCGIUNKNOWN_ROLE     3

typedef struct {
    unsigned char type;    
    unsigned char reserved[7];
} FcgiUnknownTypeBody;

typedef struct {
    FcgiHeader header;
    FcgiUnknownTypeBody body;
} FcgiUnknownTypeRecord;

struct sserversList
{
  /*! Server executable path. */
	string path;
	union 
	{
    unsigned long fileHandle;
		SOCKET sock;
		unsigned int value;
	}DESCRIPTOR;
	Socket socket;
	char host[128];
	Process process; 
	u_short port;
};

struct FcgiContext
{
	HttpThreadContext* td;
  sserversList* server;
	Socket sock;
	File tempOut;
};

class FastCgi  : public HttpDataHandler
{
private:
  static int initialPort;
	static int timeout;
  static int max_fcgi_servers;
	static int initialized;
  static Mutex servers_mutex;
	static HashDictionary<sserversList*> serversList;

	int fcgiConnectSocket(FcgiContext*,sserversList*);
	void generateFcgiHeader( FcgiHeader&, int ,int, int );
	Socket getFcgiConnection();
	int buildFASTCGIEnvironmentString(HttpThreadContext*,char*,char*);
	int sendFcgiBody(FcgiContext* con,char* buffer,int len,int type,int id);
	sserversList* isFcgiServerRunning(const char*);
  sserversList* runFcgiServer(FcgiContext*, const char*);
	sserversList* fcgiConnect(FcgiContext*, const char*);
  int runLocalServer(sserversList* server, const char* path, int port);
public:
  static void setInitialPort(int);
  static int getInitialPort(); 
  static int getTimeout();
  static void setTimeout(int);
  static void setMaxFcgiServers(int);
  static int getMaxFcgiServers();
	FastCgi();
	static int load(XmlParser*);
	int send(HttpThreadContext* td, ConnectionPtr connection,
                  const char* scriptpath, const char *cgipath,int execute,
                  int onlyHeader);
	static int unload();
};
#endif
