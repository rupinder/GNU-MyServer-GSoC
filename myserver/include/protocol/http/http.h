/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
  Free Software Foundation, Inc.
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
# define HTTP_H
# include "myserver.h"
# include <include/protocol/http/http_thread_context.h>
# include <include/protocol/protocol.h>
# include <include/protocol/http/http_headers.h>
# include <include/conf/security/security_cache.h>
# include <include/base/xml/xml_parser.h>
# include <include/base/thread/thread.h>
# include <include/base/sync/mutex.h>
# include <include/base/read_directory/rec_read_directory.h>
# include <include/protocol/http/dyn_http_command_manager.h>
# include <include/protocol/http/dyn_http_command.h>
# include <include/protocol/http/dyn_http_manager_list.h>
# include <include/protocol/http/dyn_http_manager.h>
# include <include/base/multicast/multicast.h>
# include <include/protocol/http/http_data_handler.h>
# include <include/protocol/http/webdav/webdav.h>
# include <include/base/string/securestr.h>

# include <string>
# include <sstream>
# include <vector>
# include <memory>

using namespace std;

/*!
  Data used only by an HTTP user.
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
  HttpUserData ();
  ~HttpUserData ();
  void reset ();
};

class HttpProtocol;

class Http : public Protocol
{
public:
  int requestAuthorization ();

  int sendHTTPResource (string& filename,
                        bool systemrequest = false,
                        bool onlyHeader = false,
                        bool yetMapped = false);

  int putHTTPRESOURCE (string &filename,
                       bool systemrequest = false,
                       bool onlyHeader = false,
                       bool yetMapped = false);

  int optionsHTTPRESOURCE (string &filename,
                           bool yetMapped = false);

  int traceHTTPRESOURCE (string& filename,
                         bool yetMapped = false);

  int deleteHTTPRESOURCE (string &filename,
                          bool systemrequest = false,
                          bool onlyHeader = false,
                          bool yetMapped = false);

  bool allowMethod (const char *name);

  int raiseHTTPError (int ID);

  int sendHTTPhardError500();

  int sendAuth ();


  int getPath (string& filenamePath,
               const string& filename,
               bool systemrequest)
  {return getPath (td, filenamePath, filename.c_str (), systemrequest);}

  int getPath (string& filenamePath,
               const char *filename,
               bool systemrequest)
  {return getPath (td, filenamePath, filename, systemrequest);}


  static int getPath (HttpThreadContext* td,
                      string& filenamePath,
                      const string& filename,
                      bool systemrequest)
  {return getPath (td, filenamePath, filename.c_str (), systemrequest);}

  static int getPath (HttpThreadContext* td,
                      string& filenamePath,
                      const char *filename,
                      bool systemrequest);

  MimeRecord* getMIME (string& filename);

  int logHTTPaccess ();
  int sendHTTPRedirect (const char *newURL);
  int sendHTTPNonModified ();
  Http (HttpProtocol *staticData);
  virtual ~Http ();
  virtual const char* getName (){return getNameImpl ();}

  static const char* getNameImpl ();
  int controlConnection (ConnectionPtr con, char *request, char *auxBuf,
                         u_long reqBufLen, u_long auxBufLen, u_long reqLen,
                         u_long tid);

  static int loadProtocolStatic ();

  u_long getTimeout ();
  int preprocessHttpRequest (string& filename, bool yetmapped,
                             int* permissions, bool systemrequest);

  int getFilePermissions (string& filename, string& directory,
                          string& file, string &filenamePath,
                          bool yetmapped, int* permissions);

  SecurityToken *getSecurityToken (){return &(td->securityToken);}
  HttpProtocol *getStaticData () {return staticData;}

  /* Helper function used in different places.  Probably this is not
     the best place for it.  */
  bool areSymlinksAllowed ()
  {
    const char *perm = td->securityToken.getData ("symlinks.follow",
                                                  MYSERVER_VHOST_CONF
                                                  | MYSERVER_SERVER_CONF,
                                                  "NO");
    return strcasecmp (perm, "YES") == 0;
  }

protected:
  int processDefaultFile (string& uri, int permissions, bool onlyHeader);

  struct HttpThreadContext *td;
  void clean ();
  void computeDigest (char*, char*);
  u_long checkDigest ();

  HttpProtocol *staticData;
  WebDAV dav;
};


/*!
  Adapter class to make Http reentrant.
 */
class HttpProtocol : public Protocol,
                     public MulticastRegistry<string, void*, int>
{
public:
  HttpProtocol ()
  {
    protocolOptions = 0;
  }

  virtual ~HttpProtocol ()
  {
  }

  virtual const char* getName ()
  {
    return Http::getNameImpl ();
  }

  virtual int controlConnection (ConnectionPtr con, char *request,
                                 char *auxBuf, u_long reqBufLen,
                                 u_long auxBufLen, u_long reqLen,
                                 u_long tid)
  {
    Http http (this);
    return http.controlConnection (con, request, auxBuf, reqBufLen, auxBufLen,
                                   reqLen, tid);
  }

  virtual int loadProtocol ();
  virtual int unLoadProtocol ();

  vector<Multicast<string, void*, int>*>* getHandlers (string& msg)
  {
    return MulticastRegistry<string, void*, int>::getHandlers (msg);
  }

  DynHttpCommandManager *getDynCmdManager (){return &dynCmdManager;}
  DynHttpManagerList *getDynManagerList (){return &dynManagerList;}

  u_long getTimeout () {return timeout;}
  int getAllowVhostMime () {return allowVhostMime;}
private:
  u_long timeout;
  int allowVhostMime;

  DynHttpCommandManager dynCmdManager;
  DynHttpManagerList dynManagerList;
};


#endif
