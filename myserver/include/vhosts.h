/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006, 2007 The MyServer Team
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

#ifndef VHOSTS_H
#define VHOSTS_H

#include "../stdafx.h"
#include "../include/xml_parser.h"
#include "../include/file.h"
#include "../include/utility.h"
#include "../include/connection.h"/*! Used for protocols IDs. */
#include "../include/myserver_regex.h"
#include "../include/log_manager.h"
#include "../include/mime_manager.h"
#include "../include/thread.h"
#include "../include/hash_map.h"
#include "../include/mutex.h"
#include "../include/ssl.h"
#include "../include/listen_threads.h"
#include <string>
#include <list>


using namespace std;
typedef int (*NULL_REFERENCECB)(class Vhost*); 

class Vhost
{
public:
  friend class VhostManager;
  
	struct StringRegex
	{
		string name;
    Regex regex;
    StringRegex() : regex()
      {}
    ~StringRegex()
      {}
	};
	
  /*! Get the host name. */
  const char* getName()
    {return name.c_str();}
  
  /*! Set the host name. */
  void setName(const char* c)
    {name.assign(c);}
    
  /*! Get the accesses log file name. */
  const char* getAccessesLogFileName()
    {return accessesLogFileName.c_str();}

  /*! Set the accesses log file name. */
  void setAccessesLogFileName(const char* n)
    {accessesLogFileName.assign(n);}

  /*! Get the warnings log file name. */
  const char* getWarningsLogFileName()
    {return warningsLogFileName.c_str();}

  /*! Set the warnings log file name. */
  void setWarningsLogFileName(const char* n)
    {warningsLogFileName.assign(n);}

  /*! Get the system root. */
  const char* getSystemRoot()
    {return systemRoot.c_str();}

  /*! Set the system root. */
  void setSystemRoot(const char* n)
    {systemRoot.assign(n);}

  /*! Get the document root. */
  const char* getDocumentRoot()
    {return documentRoot.c_str();}

  /*! Set the document root. */
  void setDocumentRoot(const char* n)
    {documentRoot.assign(n);}

  /*! Get the access log file options. */
  const char* getAccessLogOpt()
    {return accessLogOpt.c_str();}

  /*! Get the warnings log file options. */
	const char* getWarningLogOpt()
    {return warningLogOpt.c_str(); }

  /*! Set the access log file options. */
  void setAccessLogOpt(const char* c)
    {accessLogOpt.assign(c);}

  /*! Set the warnings log file options. */
  void setWarningLogOpt(const char* c)
    {warningLogOpt.assign(c); }

  /*! Get a pointer to the vhost SSL context. */
  SslContext *getVhostSSLContext()
    {return &sslContext;}

	/*! Initialize SSL things. */
	int initializeSSL();
	
	/*! Clear SSL things. */
	int freeSSL();

	/*! Clear the data dictionary. */
	int freeHashedData();
	
	/*! Generate the RSA key for the SSL context. */
	void generateRsaKey();

	SSL_CTX* getSSLContext();


  /*! Get the list of hosts allowed.*/
	list<StringRegex*>* getHostList()
    {return &hostList;}
	
	/*! List of IPs allowed by the vhost. */
	list<StringRegex*>* getIpList()
    {return &ipList;}

  /*! Return the port used by the host. */
	u_short getPort()
    {return port;}

  /*! Set the port used by the host. */
	void setPort(u_short p)
    {port = p;}

  /*! Get the protocol name for the virtual host. */
	const char* getProtocolName()
    {return protocolName.c_str();}

  /*! Set the protocol name for the virtual host. */
	void setProtocolName(const char *name)
    {protocolName.assign(name);}

  /*! 
   *Get the protocol used by the virtual host. 
   *This is used only for the built in protocols. 
   */
	ConnectionProtocol getProtocol()
    {return protocol;}

  /*! 
   *Set the protocol used by the virtual host. 
   *This is used only for the built in protocols. 
   */
	void setProtocol(ConnectionProtocol cp)
    {protocol=cp;}

  /*! Get the throttling rate for the virtual host. */
  u_long getThrottlingRate()
    {return throttlingRate;}

  /*! Set the throttling rate for the virtual host. */
  void setThrottlingRate(u_long tr)
    {throttlingRate = tr;}

	Vhost();
	~Vhost();

  const char* getHashedData(const char* name);
	void addIP(const char *, int);
	void addHost(const char *, int);
	void removeIP(const char *);
	void removeHost(const char *);
	int areAllHostAllowed();
	int areAllIPAllowed();
  void addRef();
  void removeRef();
  int getRef();
  void setRef(int);
	void clearIPList();
	void clearHostList();
	int isHostAllowed(const char*);
	int isIPAllowed(const char*);
	void setMaxLogSize(int);
	int getMaxLogSize();
  int isMIME();
  void setNullRefCB(NULL_REFERENCECB);
  NULL_REFERENCECB getNullRefCB();

  MimeManager* getMIME();

	LogManager* getWarningsLog();
	LogManager* getAccessesLog();

	u_long accesseslogRequestAccess(int id);
	u_long warningslogRequestAccess(int id);
	u_long accesseslogTerminateAccess(int id);
	u_long warningslogTerminateAccess(int id);

	int accessesLogWrite(const char*);
	File* getAccessesLogFile();

  int warningsLogWrite(const char*);
	File* getWarningsLogFile();

private:
  HashMap<string, string*> hashedData;
  NULL_REFERENCECB nullReferenceCb;
  Mutex refMutex;
	LogManager warningsLogFile;
	LogManager accessesLogFile;

  MimeManager mimeManager;

  /*! How many connections are using this virtual host? */
  int refCount;

	/*! SSL context. */
	SslContext sslContext;

	/*! List of hosts allowed by the vhost. */
	list<StringRegex*> hostList;

	/*! List of IPs allowed by the vhost. */
	list<StringRegex*> ipList;

	/*! TCP port used to listen on. */
	u_short port;

	/*! Protocol used by the virtual host. Used for built-in protocols. */
	ConnectionProtocol protocol;

  /*! Throttling rate to use with the virtual host. */
	u_long throttlingRate;

	/*! Protocol used by the vhost. */
	string protocolName;

  /*! Additional data for log files. Defined in configuration files. */
	string accessLogOpt;
	string warningLogOpt;
	
	/*! Path to the document root. */
	string documentRoot;
	
	/*! Path to the system root. */
	string systemRoot;
	
	/*! Path to the accesses log file. */
	string accessesLogFileName;
	
	/*! Path to the warnings log file. */
	string warningsLogFileName;
	
	/*! Description or name of the virtual host. */
	string name;
};


class VhostSource
{
public:
  VhostSource();
  ~VhostSource();
  int load();
	int save();
  int free();
  Vhost* getVHost(const char*, const char*, u_short);
	Vhost* getVHostByNumber(int n);
	int addVHost(Vhost*);
private:
	list<Vhost*> *hostList;
};

class VhostManager
{
public:
  void setExternalSource(VhostSource* extSource);
	VhostManager(ListenThreads* lt);
	~VhostManager();
	int getHostsNumber();
	Vhost* getVHostByNumber(int n);
	void clean();
	int removeVHost(int n);
	int switchVhosts(int n1,int n2);
	list<Vhost*>* getVHostList();
	
	/*! Get a pointer to a vhost.  */
	Vhost* getVHost(const char*,const char*,u_short);
	
	/*! Add an element to the vhost list.  */
	int addVHost(Vhost*);
	
	/*! Load the virtual hosts list from a xml configuration file.  */
	int loadXMLConfigurationFile(const char *,int maxlogSize=0);
	
	/*! Save the virtual hosts list to a xml configuration file.  */
	int saveXMLConfigurationFile(const char *);
	
	/*! Set the right owner for the log files.  */
	void changeFilesOwner();
private:
	ListenThreads* listenThreads;
  Mutex mutex;
	VhostSource* extSource;

	/*! List of virtual hosts. */
	list<Vhost*> hostList;
};

#endif
