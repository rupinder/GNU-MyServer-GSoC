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

#ifndef SERVER_H
# define SERVER_H

# include "myserver.h"
# include <include/base/thread/thread.h>
# include <include/base/utility.h>
# include <include/base/xml/xml_parser.h>
# include <include/base/utility.h>
# include <include/connection/connection.h>
# include <include/base/socket/socket.h>
# include <include/base/sync/event.h>
# include <include/conf/mime/mime_manager.h>
# include <include/conf/vhost/vhost_manager.h>
# include <include/conf/vhost/xml_vhost_handler.h>
# include <include/protocol/protocols_manager.h>
# include <include/connection/connection.h>
# include <include/log/log_manager.h>
# include <include/filter/filters_factory.h>
# include <include/plugin/plugins_manager.h>
# include <include/base/hash_map/hash_map.h>
# include <include/base/home_dir/home_dir.h>
# include <include/base/files_cache/cached_file_factory.h>
# include <include/base/process/process_server_manager.h>
# include <include/connections_scheduler/listen_threads.h>
# include <include/base/multicast/multicast.h>
# include <include/connections_scheduler/connections_scheduler.h>

# include <include/conf/nodetree.h>

# include <include/conf/security/security_manager.h>
# include <include/conf/security/auth_method_factory.h>
# include <include/conf/security/validator_factory.h>
# include <include/conf/main/main_configuration.h>
# include <include/conf/mime/xml_mime_handler.h>

# include <include/base/slab/slab.h>
# include <include/base/crypt/crypt_algo_manager.h>


# include <string>
# include <list>

using namespace std;

class XmlValidator;

class Server : public MulticastRegistry<string, void*, int>
{
public:

  static inline Server* getInstance ()
  {
    return instance;
  }

  ProcessServerManager *getProcessServerManager ()
  {
    return &processServerManager;
  }
  PluginsManager* getPluginsManager (){return &pluginsManager;}
  bool stopServer (){return mustEndServer;}
  HomeDir* getHomeDir ();
  static void createInstance ();

  int loadLibraries ();

  CachedFileFactory* getCachedFiles ();
  const char *getData (const char *name, const char *defValue = NULL);

  FiltersFactory* getFiltersFactory ();
  int getMaxThreads ();
  const char *getUid ();
  const char *getGid ();
  int countAvailableThreads ();
  void checkThreadsNumber ();
  int removeThread (u_long ID);
  bool isServerReady ();
  ProtocolsManager* getProtocolsManager ();
  void disableAutoReboot ();
  void enableAutoReboot ();
  bool isAutorebootEnabled ();
  bool isRebooting (){return rebooting;}
  void delayedReboot ();
  ~Server ();
  Protocol* getProtocol (const char *protocolName);
  int addConnection (Socket*, MYSERVER_SOCKADDRIN*);
  u_long getNumConnections ();
  u_long getNumTotalConnections ();
  void getConnections (list<ConnectionPtr>&);
  ConnectionPtr getConnection (int);
  u_long getTimeout ();
  const char *getAddresses ();
  const char *getPath ();
  u_long getNumThreads ();

  NodeTree<string>* getNodeTree (string& key)
  {
    return hashedData.get (key);
  }

  const char *getServerName ();
  int getMaxLogFileSize ();
  int mustUseLogonOption ();
  void start (string &, string &, string &, string &,
              MainConfiguration* (*genMainConf) (Server *server,
                                                 const char *arg));
  void stop ();
  void finalCleanup ();
  int terminate ();
  int log (LoggingLevel level, const char *fmt, ...);
  int log (char const*, LoggingLevel level = MYSERVER_LOG_MSG_INFO);
  int log (string const &str)
  {return log (str.c_str ());};
  int setLogLocation (string);
  u_long getBuffersize ();
  u_long getThrottlingRate ();
  int waitNewConnection (u_long tid, u_long timeout);
  ListenThreads *getListenThreads (){return &listenThreads;}

  void *getEnvString (){return envString;}
  VhostManager *getVhosts (){return &vhostManager;}
  MimeManager *getMimeManager (){return &mimeManager;}

  void setProcessPermissions ();
  ConnectionsScheduler* getConnectionsScheduler ()
  {return &connectionsScheduler;}

  int deleteConnection (ConnectionPtr);

  int notifyDeleteConnection (ConnectionPtr);

  void increaseFreeThread ();
  void decreaseFreeThread ();

  SecurityManager *getSecurityManager (){return &securityManager;}
  AuthMethodFactory *getAuthMethodFactory () {return &authMethodFactory;}
  ValidatorFactory *getValidatorFactory (){return &validatorFactory;}

  map<string, string>& getConsoleColors () { return consoleColors; }

  LogManager *getLogManager () { return logManager; }

  MainConfiguration *getConfiguration ();
  CryptAlgoManager *getCryptAlgoManager () {return &cryptAlgoManager;}

private:
  XmlValidator *xmlValidator;
  VhostManagerHandler *vhostHandler;

  MainConfiguration *configurationFileManager;
  MainConfiguration* (*genMainConf) (Server *server, const char *arg);
  int loadVHostConf ();

  /*!
    When the flag mustEndServer is 1 all the threads are
    stopped and the application stop its execution.
  */
  int mustEndServer;

  Mutex connectionsPoolLock;
  Slab<Connection> connectionsPool;

  /*! Singleton instance.  Call createInstance before use it.  */
  static Server* instance;

  /*! Do not allow to create directly objects.  */
  Server ();

  void mainLoop ();
  void loadStaticComponents ();
  void loadPlugins ();
  void displayBoot ();
  int postLoad ();
  void initLogManager ();

  CachedFileFactory cachedFiles;

  void *envString;
  VhostManager vhostManager;
  MimeManager mimeManager;
  HomeDir homeDir;

  list<NodeTree<string>*> hashedDataTrees;
  HashMap<string, NodeTree<string>*> hashedData;

  FiltersFactory filtersFactory;
  string uid;
  string gid;
  int currentThreadID;
  ProtocolsManager protocols;

  bool autoRebootEnabled;
  bool toReboot;
  bool rebooting;

  LogManager* logManager;
  bool serverReady;
  u_long throttlingRate;
  u_long buffersize;

  /* Buffer that contains all the local machine IP values.  */
  string *ipAddresses;
  char serverName[HOST_NAME_MAX + 1];
  string* path;
  int initialize ();
  int addThread (bool staticThread = false);
  ConnectionPtr addConnectionToList (Socket* s, MYSERVER_SOCKADDRIN *asock_in,
                                     char *ipAddr, char *localIpAddr,
                                     u_short port, u_short localPort, int);
  u_long maxConnections;
  u_long maxConnectionsToAccept;
  void clearAllConnections ();
  int freeHashedData ();
  u_long connectionTimeout;
  u_long maxLogFileSize;
  bool resetConfigurationPaths (string &, string &, string &, string &);
  Mutex* connectionsMutex;
  u_long nStaticThreads;
  u_long nMaxThreads;
  u_long freeThreads;

  u_long purgeThreadsThreshold;

  Mutex* threadsMutex;
  list<ClientsThread*> threads;

  int purgeThreads ();
  int reboot ();

  string mainConfigurationFile;
  string vhostConfigurationFile;
  string mimeConfigurationFile;
  string externalPath;

  CryptAlgoManager cryptAlgoManager;
  PluginsManager pluginsManager;
  ProcessServerManager processServerManager;
  ConnectionsScheduler connectionsScheduler;
  ListenThreads listenThreads;
  bool endServer;
  string logLocation;
  AuthMethodFactory authMethodFactory;
  ValidatorFactory validatorFactory;
  SecurityManager securityManager;
  map<string, string> consoleColors;
};

#endif
