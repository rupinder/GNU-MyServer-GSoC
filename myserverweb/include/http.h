/*
*MyServer
*Copyright (C) 2002, 2003, 2004, 2005 The MyServer Team
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
#include "../include/protocol.h"
#include "../include/http_headers.h"
#include "../include/cgi.h"
#include "../include/winCGI.h"
#include "../include/fastCGI.h"
#include "../include/mscgi.h"
#include "../include/isapi.h"
#include "../include/security_cache.h"
#include "../include/cXMLParser.h"
#include "../include/threads.h"
#include "../include/http_file.h"
#include "../include/http_dir.h"

#include <string>
#include <sstream>

using namespace std;

/*!
 *Data used only by an HTTP user.
 */
class HttpUserData : public ProtocolBuffer
{
public:
	/*! Realm string used by Digest authorization scheme.  */
	char realm[48];
	/*! Opaque string used by Digest authorization scheme.  */
	char opaque[48];
	/*! Nonce string used by Digest authorization scheme.  */
	char nonce[48];
	/*! Cnonce string used by Digest authorization scheme.  */
	char cnonce[48];
	/*! Password string used by Digest authorization scheme.  */
	char needed_password[32];
	/*! Nonce count used by Digest authorization scheme.  */
	u_long nc;
	/*! Nonzero if the user was authenticated trough the Digest scheme.  */
	int digest;
	/*! Nonzero if the digest was already checked.  */
	int digest_checked;
  HttpUserData();
  ~HttpUserData();
	void reset();
};

class Http : public Protocol
{
private:
  static  Mutex sec_cache_mutex;
  static SecurityCache sec_cache;
	static int initialized;
	/*! Store if the MSCGI library was loaded.  */
	static int mscgiLoaded;
	static char *browseDirCSSpath;
	static u_long gzip_threshold;
	static int useMessagesFiles;	
	static char *defaultFilename;
	static u_long nDefaultFilename;	
  static int cgi_timeout;
  static int fastcgi_servers;
	MsCgi lmscgi;
	WinCgi lwincgi;
	Isapi lisapi;
	Cgi lcgi;
	FastCgi lfastcgi;
  HttpFile lhttp_file;
  HttpDir lhttp_dir;
	struct HttpThreadContext td;
  void clean();
protected:
	char protocolPrefix[12];
public:
	int PROTOCOL_OPTIONS;
	char *getDefaultFilenamePath(u_long ID);
	int sendHTTPResource(HttpThreadContext*,ConnectionPtr s,char *filename,
                       int systemrequest=0,int OnlyHeader=0,int yetmapped=0);
	int putHTTPRESOURCE(HttpThreadContext*,ConnectionPtr s,char *filename,
                      int systemrequest=0,int OnlyHeader=0,int yetmapped=0);
	int allowHTTPTRACE(HttpThreadContext*,ConnectionPtr s);
	int optionsHTTPRESOURCE(HttpThreadContext*,ConnectionPtr s,char *filename,
                          int yetmapped=0);
	int traceHTTPRESOURCE(HttpThreadContext*,ConnectionPtr s,char *filename,
                        int yetmapped=0);
	int deleteHTTPRESOURCE(HttpThreadContext*,ConnectionPtr s,char *filename,
                         int yetmapped=0);
	int raiseHTTPError(HttpThreadContext*, ConnectionPtr a, int ID);
	int sendHTTPhardError500(HttpThreadContext* td, ConnectionPtr a);
	int sendAuth(HttpThreadContext* td, ConnectionPtr a);
	int getPath(HttpThreadContext* td, ConnectionPtr s, char **filenamePath,
               const char *filename,int systemrequest);
	int getMIME(HttpThreadContext* td, char *MIME, char *filename, 
              char *dest, char **dest2);
	int getMIME(HttpThreadContext* td, string& MIME, string& filename, 
              string& dest, string& dest2);
	int logHTTPaccess(HttpThreadContext* td,ConnectionPtr a);
	int sendHTTPRedirect(HttpThreadContext* td,ConnectionPtr a, const char *newURL);
	int sendHTTPNonModified(HttpThreadContext* td,ConnectionPtr a);
	Http();
	virtual ~Http();
	void computeDigest(HttpThreadContext* td, char*, char*);
	u_long checkDigest(HttpThreadContext* td, ConnectionPtr s);
  char* getBrowseDirCSSFile();
	u_long getGzipThreshold();
	/*! The function is used to the request and build a response.  */
	virtual char* registerName(char*,int len);
	int controlConnection(ConnectionPtr a, char *b1, char *b2, int bs1, 
                        int bs2, u_long nbtr, u_long id);
	static int loadProtocol(XmlParser*,char*);
	static int unloadProtocol(XmlParser*);
  int getCGItimeout();
};

#endif
