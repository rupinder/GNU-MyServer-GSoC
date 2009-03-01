/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2002-2009 Free Software Foundation, Inc.
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
#include "stdafx.h"
#include <include/protocol/http/http_thread_context.h>
#include <include/protocol/protocol.h>
#include <include/protocol/http/http_headers.h>
#include <include/conf/security/security_cache.h>
#include <include/base/xml/xml_parser.h>
#include <include/base/thread/thread.h>
#include <include/base/sync/mutex.h>
#include <include/protocol/http/dyn_http_command_manager.h>
#include <include/protocol/http/dyn_http_command.h>
#include <include/protocol/http/dyn_http_manager_list.h>
#include <include/protocol/http/dyn_http_manager.h>
#include <include/base/multicast/multicast.h>
#include <include/protocol/http/http_data_handler.h>
#include <include/base/string/securestr.h>

#include <string>
#include <sstream>
#include <vector>
#include <memory>

using namespace std;

class HttpStaticData : public MulticastRegistry<string, void*, int>
{
public:
  vector<Multicast<string, void*, int>*>* getHandlers(string& msg)
  {
    return MulticastRegistry<string, void*, int>::getHandlers(msg);
  }

  HttpStaticData ();
  virtual ~HttpStaticData ();

  void clear()
  {
    clearMulticastRegistry();
  }

  vector<string> defaultFilename;
  int cgiTimeout;
  int allowVhostMime;

  HttpDataHandler *mscgi;
  HttpDataHandler *wincgi;
  HttpDataHandler *isapi;
  HttpDataHandler *cgi;
  HttpDataHandler *scgi;
  HttpDataHandler *fastcgi;
  HttpDataHandler *httpFile;
  HttpDataHandler *httpDir;


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

  bool allowHTTPTRACE();


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
  {return getPath(td, filenamePath, filename.c_str(), systemrequest);}

  int getPath(string& filenamePath,
                     const char *filename,
                     int systemrequest)
  {return getPath(td, filenamePath, filename, systemrequest);}


  static int getPath(HttpThreadContext* td,
                     string& filenamePath,
                     const string& filename,
                     int systemrequest)
  {return getPath(td, filenamePath, filename.c_str(), systemrequest);}

  static int getPath(HttpThreadContext* td, 
                     string& filenamePath,
                     const char *filename,
                     int systemrequest);

  MimeRecord* getMIME(string& filename);

  int logHTTPaccess();
  int sendHTTPRedirect(const char *newURL);
  int sendHTTPNonModified();
  Http();
  virtual ~Http();
  virtual char* registerName(char* out,int len){return registerNameImpl(out, len);}

  static char* registerNameImpl(char*, int len);
  int controlConnection(ConnectionPtr a, 
                        char *b1, 
                        char *b2, 
                        int bs1, 
                        int bs2, 
                        u_long nbtr, 
                        u_long id);

  static int loadProtocolStatic(XmlParser*);

  static int unLoadProtocolStatic(XmlParser*);

  int getCGItimeout();
  int preprocessHttpRequest(string& filename, int yetmapped, 
                            int* permissions);

  int getFilePermissions(string& filename, string& directory, 
                         string& file, string &filenamePath, 
                         int yetmapped, int* permissions);

  static HttpStaticData* getStaticData();

  SecurityToken *getSecurityToken (){return &(td->securityToken);}

protected:

  int processDefaultFile (string& uri, int permissions, int onlyHeader);

  struct HttpThreadContext *td;
  void clean();
  void computeDigest(char*, char*);
  u_long checkDigest();
  string protocolPrefix;
};


/*!
 *Adapter class to make Http reentrant.
 */
class HttpProtocol : public Protocol
{
public:
	HttpProtocol()
  {
    protocolOptions = 0;
  }

  virtual ~HttpProtocol()
  {

  }

  virtual char* registerName(char* out, int len)
  {
    return Http::registerNameImpl(out, len);
  }

	virtual int controlConnection(ConnectionPtr a, char *b1, char *b2,
                                int bs1, int bs2, u_long nbtr, u_long id)
  {
    Http http;
    int ret = 0;

    ret = http.controlConnection(a, b1, b2, bs1, bs2, nbtr, id);

    return ret;
  }

	virtual int loadProtocol(XmlParser* parser)
  {
    return Http::loadProtocolStatic(parser);
  }
  
  virtual int unLoadProtocol(XmlParser* parser)
  {
    return Http::unLoadProtocolStatic(parser);

  }

};


#endif
