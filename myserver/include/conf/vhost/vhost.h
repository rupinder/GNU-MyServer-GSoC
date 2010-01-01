/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008, 2009, 2010 Free
  Software Foundation, Inc.
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

#ifndef VHOST_H
# define VHOST_H

# include <list>
# include <string>

# include "stdafx.h"
# include <include/base/xml/xml_parser.h>
# include <include/base/file/file.h>
# include <include/base/utility.h>
# include <include/base/regex/myserver_regex.h>
# include <include/log/log_manager.h>
# include <include/conf/mime/mime_manager.h>
# include <include/base/thread/thread.h>
# include <include/base/hash_map/hash_map.h>
# include <include/base/sync/mutex.h>
# include <include/base/ssl/ssl.h>
# include <include/connections_scheduler/listen_threads.h>
# include <include/conf/vhost/ip.h>
# include <include/conf/nodetree.h>
# include <include/conf/mime/mime_manager.h>

using namespace std;
typedef int (*NULL_REFERENCECB)(class Vhost*);

class VhostProtocolData
{
public:
  VhostProtocolData (){}
  virtual ~VhostProtocolData (){}
};

class Vhost
{
public:
  struct StringRegex
  {
    string name;
    Regex regex;
    StringRegex () : regex ()
    {}
    ~StringRegex ()
    {}
  };

  /*! Get the host name. */
  const char* getName ()
  {return name.c_str ();}

  /*! Set the host name. */
  void setName (const char* c)
  {name.assign (c);}

  /*! Get the system root. */
  const string& getSystemRoot ()
  {return systemRoot;}

  /*! Set the system root. */
  void setSystemRoot (const char* n)
  {systemRoot.assign (n);}

  /*! Get the document root. */
  const string& getDocumentRoot ()
  {return documentRoot;}

  /*! Set the document root. */
  void setDocumentRoot (const char* n)
  {documentRoot.assign (n);}

  /*! Get a pointer to the vhost SSL context. */
  SslContext *getVhostSSLContext ()
  {return &sslContext;}

  /*! Initialize SSL things. */
  int initializeSSL ();

  /*! Clear SSL things. */
  int freeSSL ();

  /*! Clear the data dictionary. */
  int freeHashedData ();

  /*! Generate the RSA key for the SSL context. */
  void generateRsaKey ();

  SSL_CTX* getSSLContext ();

  /*! Get the list of hosts allowed.*/
  list<StringRegex*>* getHostList ()
  {return &hostList;}

  /*! Return the port used by the host. */
  u_short getPort ()
  {return port;}

  /*! Set the port used by the host. */
  void setPort (u_short p)
  {port = p;}

  /*! Get the protocol name for the virtual host. */
  const char* getProtocolName ()
  {return protocolName.c_str ();}

  /*! Set the protocol name for the virtual host. */
  void setProtocolName (const char *name)
  {protocolName.assign (name);}

  /*! Get the throttling rate for the virtual host. */
  u_long getThrottlingRate ()
  {return throttlingRate;}

  /*! Set the throttling rate for the virtual host. */
  void setThrottlingRate (u_long tr)
  {throttlingRate = tr;}

  Vhost (LogManager* lm);
  ~Vhost ();

  const char* getData (const char* name);

  NodeTree<string>* getNodeTree (string& key)
  {
    return hashedData.get (key);
  }

  void addHost (const char *, int);
  void removeHost (const char *);
  int areAllHostAllowed ();
  void addRef ();
  void removeRef ();
  int getRef ();
  void setRef (int);
  void clearHostList ();
  int isHostAllowed (const char*);
  int isMIME ();
  int getDefaultPriority (){return defaultPriority;}
  void setDefaultPriority (int priority){defaultPriority = priority;}
  void setNullRefCB (NULL_REFERENCECB);
  NULL_REFERENCECB getNullRefCB ();

  void addIP (const char *, int);
  void removeIP (const char *);
  void clearIPList ();
  int areAllIPAllowed ();
  int isIPAllowed (const char*);

  MimeManagerHandler *getMIME ();

  /*!
   * \return 0 if the LogManager contains at least one valid entry where
   * this Vhost can output both its warning and access messages.
   */
  int openLogFiles ();

  int openAccessLog (string, list<string>&, u_long);
  int openWarningLog (string, list<string>&, u_long);

  /*! Set the access log options. */
  void setAccessLogOpt (const char* c) { accessLogOpt.assign (c); }

  /*! Set the warnings log options. */
  void setWarningLogOpt (const char* c)  { warningLogOpt.assign (c); }

  /*! Get the access log options. */
  const char* getAccessLogOpt ()  { return accessLogOpt.c_str (); }

  /*! Get the warnings log options. */
  const char* getWarningLogOpt () { return warningLogOpt.c_str (); }

  /*! Write a message on the accesses log. */
  int accessesLogWrite (const char* fmt, ...);

  /*! Write a message on the warnings log. */
  int warningsLogWrite (const char* fmt, ...);

  /*! Get the protocol data. */
  VhostProtocolData* getProtocolData (){return protocolData;}

  /*! Set the protocol data. */
  void setProtocolData (VhostProtocolData* data){protocolData = data;}

  MimeRecord* getLocationMime (string& loc){return locationsMime.get (loc);}

  list<NodeTree<string>*> *getHashedDataTrees (){return &hashedDataTrees;}
  HashMap<string, NodeTree<string>*>* getHashedData (){return &hashedData;}

  HashMap<string, MimeRecord*>* getLocationsMime (){return &locationsMime;}
  MimeRecord *addLocationMime (string &val, MimeRecord *r);
  MimeManagerHandler *getMimeHandler (){return mimeHandler;}
  void setMimeHandler (MimeManagerHandler *h){ mimeHandler = h;}

private:
  const static string accessLogType;
  const static string warningLogType;

  VhostProtocolData*  protocolData;

  list<NodeTree<string>*> hashedDataTrees;
  HashMap<string, NodeTree<string>*> hashedData;


  NULL_REFERENCECB nullReferenceCb;
  Mutex refMutex;
  LogManager* logManager;

  HashMap<string, MimeRecord*> locationsMime;
  MimeManagerHandler *mimeHandler;

  /*! The default priority for the connections scheduler.  */
  int defaultPriority;

  /*! How many connections are using this virtual host? */
  int refCount;

  /*! SSL context. */
  SslContext sslContext;

  /*! List of hosts allowed by the vhost. */
  list<StringRegex*> hostList;

  /*! List of IPs allowed by the vhost. */
  list<IpRange*> ipListAllow;
  /*! List of IPs denied by the vhost. */
  list<IpRange*> ipListDeny;

  /*! TCP port used to listen on. */
  u_short port;

  /*! Throttling rate to use with the virtual host. */
  u_long throttlingRate;

  /*! Protocol used by the vhost. */
  string protocolName;

  /*! Path to the document root. */
  string documentRoot;

  /*! Path to the system root. */
  string systemRoot;

  /*! Description or name of the virtual host. */
  string name;

  /*! Additional data for logs. Defined in configuration files. */
  string accessLogOpt;
  string warningLogOpt;
};

#endif
