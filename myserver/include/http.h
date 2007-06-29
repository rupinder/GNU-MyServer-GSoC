/*
MyServer
Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef HTTP_H
#define HTTP_H
#include "../stdafx.h"
#include "../include/protocol.h"
#include "../include/http_headers.h"
#include "../include/cgi.h"
#include "../include/wincgi.h"
#include "../include/fastcgi.h"
#include "../include/scgi.h"
#include "../include/mscgi.h"
#include "../include/isapi.h"
#include "../include/http_file.h"
#include "../include/http_dir.h"
#include "../include/security_cache.h"
#include "../include/xml_parser.h"
#include "../include/thread.h"
#include "../include/mutex.h"
#include "../include/dyn_http_command_manager.h"
#include "../include/dyn_http_command.h"
#include "../include/dyn_http_manager_list.h"
#include "../include/dyn_http_manager.h"
#include "../include/multicast.h"

#include <string>
#include <sstream>
#include <vector>
using namespace std;

class HttpStaticData : public MulticastRegistry<string, void*, int>
{
public:
	vector<Multicast<string, void*, int>*>* getHandlers(string& msg)
	{
		return MulticastRegistry<string, void*, int>::getHandlers(msg);
	}

  DynHttpCommandManager dynCmdManager;
  DynHttpManagerList dynManagerList;
};

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
	char requiredPassword[32];
	/*! Nonce count used by Digest authorization scheme.  */
	u_long nc;
	/*! Nonzero if the user was authenticated trough the Digest scheme.  */
	int digest;
	/*! Nonzero if the digest was already checked.  */
	int digestChecked;
  HttpUserData();
  ~HttpUserData();
	void reset();
};

class Http : public Protocol
{
public:
	int protocolOptions;
	int requestAuthorization();
	const char *getDefaultFilenamePath(u_long ID);

	int sendHTTPResource(string& filename,
                       int systemrequest = 0, 
											 int onlyHeader = 0, 
											 int yetMapped = 0);

	int putHTTPRESOURCE(string &filename,
                      int systemrequest = 0,
											int onlyHeader = 0,
											int yetMapped = 0);

	int allowHTTPTRACE();


	int optionsHTTPRESOURCE(string &filename,
                          int yetMapped = 0);

	int traceHTTPRESOURCE(string& filename,
                        int yetMapped = 0);

	int deleteHTTPRESOURCE(string& filename,
                         int yetMapped = 0);

	int raiseHTTPError(int ID);

	int sendHTTPhardError500();

	int sendAuth();

	int getPath(string& filenamePath,
               const string& filename,
							int systemrequest)
	  {return getPath(filenamePath, filename.c_str(), systemrequest);}

	int getPath(string& filenamePath,
							const char *filename,
							int systemrequest);

  MimeRecord* getMIME(string& filename);

	int logHTTPaccess();
	int sendHTTPRedirect(const char *newURL);
	int sendHTTPNonModified();
	Http();
	virtual ~Http();
  const char* getBrowseDirCSSFile();
	u_long getGzipThreshold();
	virtual char* registerName(char*,int len);
	int controlConnection(ConnectionPtr a, 
												char *b1, 
												char *b2, 
												int bs1, 
                        int bs2, 
												u_long nbtr, 
												u_long id);

	static int loadProtocol(XmlParser*);

	static int unLoadProtocol(XmlParser*);

  int getCGItimeout();

	static HttpStaticData* getStaticData();
protected:
  static Mutex secCacheMutex;
  static SecurityCache secCache;
	static int initialized;

	/*! Store if the MSCGI library was loaded.  */
	static int mscgiLoaded;
	static string browseDirCSSpath;
	static u_long gzipThreshold;
	static vector<string> defaultFilename;
  static int cgiTimeout;
  static int allowVhostMime;
	MsCgi mscgi;
	WinCgi wincgi;
	Isapi isapi;
	Cgi cgi;
	Scgi scgi;
	FastCgi fastcgi;
  HttpFile httpFile;
  HttpDir httpDir;
	struct HttpThreadContext td;
  void clean();
	int readPostData(HttpThreadContext* td, int* ret);
	void computeDigest(char*, char*);
	u_long checkDigest();
	string protocolPrefix;
private:
	int readContiguousPrimitivePostData(char* inBuffer,
																			u_long *inBufferPos,
																			u_long inBufferSize,
																			Socket *inSocket,
																			char* outBuffer,
																			u_long outBufferSize,
																			u_long* nbr,
																			u_long timeout);
	int readChunkedPostData(char* inBuffer,
													u_long *inBufferPos,
													u_long inBufferSize,
													Socket *inSocket,
													char* outBuffer,
													u_long outBufferSize,
													u_long* nbr,
													u_long timeout,
													File* out);

};

#endif
