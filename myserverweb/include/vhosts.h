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

#ifndef VHOST_IN
#define VHOST_IN

#include "../stdafx.h"
#include "../include/cXMLParser.h"
#include "../include/filemanager.h"
#include "../include/utility.h"
#include "../include/connectionstruct.h"/*! Used for protocols IDs. */
#include "../include/myserver_regex.h"
#include "../include/log_manager.h"
#include "../include/MIME_manager.h"

#include <string>

#ifndef DO_NOT_USE_SSL
#include<openssl/ssl.h>
#include<openssl/rsa.h>
#include<openssl/crypto.h>
#include<openssl/lhash.h>
#include<openssl/err.h>
#include<openssl/bn.h>
#include<openssl/pem.h>
#include<openssl/x509.h>
#include<openssl/rand.h>
#else
	#define SSL_CTX int;
	#define SSL_METHOD int;
#endif

using namespace std;

#define LOG_FILES_OPTS_LEN	256

class Vhost
{
	LogManager warningsLogFile;
	LogManager accessesLogFile;

  MimeManager mime_manager;

public:
	struct sHostList
	{
		char hostName[MAX_COMPUTERNAME_LENGTH+1];
    Regex hostRegex;
		sHostList *next;
	};

	/*! List of hosts allowed by the vhost. */
	sHostList *hostList;
	
	struct vhsslcontext
	{
#ifndef DO_NOT_USE_SSL
		SSL_CTX* context;
		SSL_METHOD* method;
#else
		void* context;
		void* method;
#endif
		string certificateFile;
		string privateKeyFile;
		char password[32];
	};
	
	/*! SSL context. */
	vhsslcontext sslContext;

	struct sIpList
	{
    char hostIp[32];
    Regex ipRegex;
    sIpList *next; 
	};
	
	/*! List of IPs allowed by the vhost. */
	sIpList *ipList;

	/*! TCP port used to listen on. */
	u_short port;

	/*! Protocol used by the virtual host. Used for built-in protocols. */
	CONNECTION_PROTOCOL protocol;

  /*! Throttling rate to use with the virtual host. */
	u_long throttlingRate;

	/*! Protocol used by the vhost. */
	char protocol_name[16];
	
	/*! Initialize SSL things. */
	int initializeSSL();
	
	/*! Clear SSL things. */
	int freeSSL();
	
	/*! Generate the RSA key for the SSL context. */
	void generateRsaKey();
#ifndef DO_NOT_USE_SSL
	SSL_CTX* getSSLContext();
#else
	void* getSSLContext();
#endif
  /*! Additional data for log files. Defined in configuration files. */
	char accessLogOpt[LOG_FILES_OPTS_LEN];
	char warningLogOpt[LOG_FILES_OPTS_LEN];
	
	/*! Path to the document root. */
	string documentRoot;
	
	/*! Path to the system root. */
	string systemRoot;
	
	/*! Path to the accesses log file. */
	string accessesLogFileName;
	
	/*! Path to the warnings log file. */
	string warningsLogFileName;
	
	/*! Description or name of the virtual host. */
	char name[64];
	
  u_long getThrottlingRate();
	Vhost();
	void addIP(char *, int);
	void addHost(char *, int);
	void removeIP(char *);
	void removeHost(char *);
	int areAllHostAllowed();
	int areAllIPAllowed();
	void clearIPList();
	void clearHostList();
	int isHostAllowed(char*);
	int isIPAllowed(char*);
	void setMaxLogSize(int);
	int getMaxLogSize();
  int isMIME();
  MimeManager* getMIME();

	LogManager* getWarningsLog();
	LogManager* getAccessesLog();

	u_long accesseslogRequestAccess(int id);
	u_long warningslogRequestAccess(int id);
	u_long accesseslogTerminateAccess(int id);
	u_long warningslogTerminateAccess(int id);

	~Vhost();

	int accessesLogWrite(const char*);
	File* getAccessesLogFile();

  int warningsLogWrite(const char*);
	File* getWarningsLogFile();
};

class VhostSource
{
public:
  VhostSource();
  ~VhostSource();
  int load();
  int free();
  Vhost* getVHost(char*,char*,u_short);
	Vhost* getVHostByNumber(int n);
};

class VhostManager
{
public:
	struct sVhostList
	{
		Vhost* host;
		sVhostList* next;
	};
private:
	VhostSource* extSource;
	/*! List of virtual hosts. */
	sVhostList *vhostList;
public:
  void setExternalSource(VhostSource* extSource);
	VhostManager();
	~VhostManager();
	int getHostsNumber();
	Vhost* getVHostByNumber(int n);
	void clean();
	int removeVHost(int n);
	int switchVhosts(int n1,int n2);
	int switchVhosts(sVhostList*,sVhostList*);
	VhostManager::sVhostList*  getVHostList();
	
	/*! Get a pointer to a vhost.  */
	Vhost* getVHost(char*,char*,u_short);
	
	/*! Add an element to the vhost list.  */
	void addVHost(Vhost*);
	
	/*! Load the virtual hosts list from a configuration file.  */
	int loadConfigurationFile(const char *,int maxlogSize=0);
	
	/*! Save the virtual hosts list to a configuration file.  */
	void saveConfigurationFile(const char *);
	
	/*! Load the virtual hosts list from a xml configuration file.  */
	int loadXMLConfigurationFile(const char *,int maxlogSize=0);
	
	/*! Save the virtual hosts list to a xml configuration file.  */
	void saveXMLConfigurationFile(const char *);
};

#endif
