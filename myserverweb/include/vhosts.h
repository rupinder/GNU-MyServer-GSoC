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
#include "../include/filemanager.h"
#include "../include/utility.h"
#include "../include/connectionstruct.h"/*!Used for protocols IDs*/

#ifndef DO_NOT_USE_SSL
#include<openssl/ssl.h>
#include<openssl/rsa.h>
#include<openssl/engine.h>
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
static int password_cb(char *buf,int num,int rwflag,void *userdata);

#define LOG_FILES_OPTS_LEN	256

class vhost
{
	MYSERVER_FILE warningsLogFile;
	MYSERVER_FILE accessesLogFile;
	/*! Max size in bytes for the log files used by this host.  */
	u_long maxLogSize;
public:
	struct sHostList
	{
		char hostName[MAX_COMPUTERNAME_LENGTH+1];
		sHostList *next;
	};
	/*! List of hosts allowed by the vhost.  */
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
		char certificateFile[MAX_PATH];
		char privateKeyFile[MAX_PATH];
		char password[32];
	};
	
	/*! SSL context.  */
	vhsslcontext sslContext;

	struct sIpList
	{
		char hostIp[32];
		sIpList *next; 
	};
	
	/*! List of IPs allowed by the vhost.  */
	sIpList *ipList;

	/*! TCP port used to listen on. */
	u_short port;

	/*! Protocol used by the virtual host. Used for built-in protocols.  */
	CONNECTION_PROTOCOL protocol;
	
	/*! Protocol used by the vhost.  */
	char protocol_name[16];
	
	/*! Initialize SSL things.  */
	int initializeSSL();
	
	/*! Clear SSL things.  */
	int freeSSL();
	
	/*! Generate the RSA key for the SSL context.  */
	void generateRsaKey();
#ifndef DO_NOT_USE_SSL
	SSL_CTX* getSSLContext();
#else
	void* getSSLContext();
#endif
	char accessLogOpt[LOG_FILES_OPTS_LEN];
	char warningLogOpt[LOG_FILES_OPTS_LEN];
	
	/*! Path to the document root.  */
	char documentRoot[MAX_PATH];
	
	/*! Path to the system root.  */
	char systemRoot[MAX_PATH];
	
	/*! Path to the accesses log file.  */
	char accessesLogFileName[MAX_PATH];
	
	/*! Path to the warnings log file.  */
	char warningsLogFileName[MAX_PATH];
	
	/*! Description or name of the virtual host.  */
	char name[64];
	
	vhost();
	void addIP(char *);
	void addHost(char *);
	void removeIP(char *);
	void removeHost(char *);
	int areAllHostAllowed();
	int areAllIPAllowed();
	void clearIPList();
	void clearHostList();
	int isHostAllowed(char*);
	int isIPAllowed(char*);
	void setMaxLogSize(u_long);
	u_long getMaxLogSize();
	myserver_mutex accessesLogFileAccess;
	myserver_mutex warningsLogFileAccess;

	u_long accesseslogRequestAccess(int id);
	u_long warningslogRequestAccess(int id);
	u_long accesseslogTerminateAccess(int id);
	u_long warningslogTerminateAccess(int id);

	~vhost();
	/*!
	*Functions to manage the logs file.
	*Derived directly from the filemanager utilities.
	*/
	u_long accessesLogWrite(char*);
	MYSERVER_FILE* getAccessesLogFile();

	u_long warningsLogWrite(char*);
	MYSERVER_FILE* getWarningsLogFile();
};


class vhostmanager
{
public:
	struct sVhostList
	{
		vhost* host;
		sVhostList* next;
	};
private:
	
	/*! List of virtual hosts. */
	sVhostList *vhostList;
public:
	vhostmanager();
	~vhostmanager();
	int getHostsNumber();
	vhost* getVHostByNumber(int n);
	void clean();
	int removeVHost(int n);
	int switchVhosts(int n1,int n2);
	int switchVhosts(sVhostList*,sVhostList*);
	vhostmanager::sVhostList*  getvHostList();
	
	/*! Get a pointer to a vhost.  */
	vhost*  getvHost(char*,char*,u_short);
	
	/*! Add an element to the vhost list.  */
	void addvHost(vhost*);
	
	/*! Load the virtual hosts list from a configuration file.  */
	void loadConfigurationFile(char *,int maxlogSize=0);
	
	/*! Save the virtual hosts list to a configuration file.  */
	void saveConfigurationFile(char *);
	
	/*! Load the virtual hosts list from a xml configuration file.  */
	void loadXMLConfigurationFile(char *,int maxlogSize=0);
	
	/*! Save the virtual hosts list to a xml configuration file.  */
	void saveXMLConfigurationFile(char *);
};

#endif
