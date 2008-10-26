/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008 Free Software Foundation, Inc.
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
#define SERVER_H

#include "stdafx.h"
#include <include/base/thread/thread.h>
#include <include/base/utility.h>
#include <include/base/xml/xml_parser.h>
#include <include/base/utility.h>
#include <include/connection/connection.h>
#include <include/base/socket/socket.h>
#include <include/base/sync/event.h>
#include <include/conf/mime/mime_manager.h>
#include <include/conf/vhost/vhost_manager.h>
#include <include/protocol/protocols_manager.h>
#include <include/connection/connection.h>
#include <include/log/log_manager.h>
#include <include/filter/filters_factory.h>
#include <include/plugin/plugins_manager.h>
//#include <include/plugin/filter/dyn_filters_manager.h>
//#include <include/plugin/filter/dyn_filter.h>
//#include <include/plugin/executor/dyn_executor_manager.h>
#include <include/base/hash_map/hash_map.h>
#include <include/base/home_dir/home_dir.h>
#include <include/base/files_cache/cached_file_factory.h>
#include <include/base/process/process_server_manager.h>
#include <include/connections_scheduler/listen_threads.h>
#include <include/base/multicast/multicast.h>
#include <include/connections_scheduler/connections_scheduler.h>


#include <include/conf/security/security_manager.h>
#include <include/conf/security/auth_method_factory.h>
#include <include/conf/security/validator_factory.h>

#include <string>
#include <list>

using namespace std;

/*!
 *Definition for new threads entry-point.
 */
#ifdef WIN32
unsigned int __stdcall listenServer(void* pParam);
#endif
#ifdef NO_WIN
void* listenServer(void* pParam);
#endif

class Server : public MulticastRegistry<string, void*, int>
{
public:
  ProcessServerManager* getProcessServerManager()
  {
    return &processServerManager;
  }
  PluginsManager* getPluginsManager(){return &pluginsManager;}
  bool stopServer(){return mustEndServer;}
  HomeDir* getHomeDir();
  static void createInstance();
  static inline Server* getInstance()
  {
    return instance;
  }

  int loadLibraries();

  CachedFileFactory* getCachedFiles();
  const char* getHashedData(const char* name);

  void setGlobalData(const char* name, void* data);
  void* getGlobalData(const char* name);

  FiltersFactory* getFiltersFactory();
  int getMaxThreads();
  u_long getUid();
  u_long getGid();
  int countAvailableThreads();
  void checkThreadsNumber();
  int removeThread(u_long ID);
  bool isServerReady();
  ProtocolsManager* getProtocolsManager();
  void disableAutoReboot();
  void enableAutoReboot();
  bool isAutorebootEnabled();
  bool isRebooting(){return rebooting;}
  void rebootOnNextLoop();
  const char* getMainConfFile();
  const char* getVhostConfFile();
  const char* getMIMEConfFile();
  const char* getLanguagesPath();
  const char* getLanguageFile();
  const char* getExternalPath();
  XmlParser* getLanguageParser();
  ~Server();
  Protocol* getProtocol(const char *protocolName);
  int addConnection(Socket,MYSERVER_SOCKADDRIN*);
  u_long getNumConnections();
  u_long getNumTotalConnections();
  void getConnections(list<ConnectionPtr>&);
  ConnectionPtr getConnection(int);
  u_long getTimeout();
  const char *getAddresses();
  const char *getPath();
  u_long getNumThreads();
  const char *getDefaultFilenamePath(u_long ID = 0);
  const char *getServerName();
  u_long getVerbosity();
  const char *getServerAdmin();
  int getMaxLogFileSize();
  int mustUseLogonOption();
  void setVerbosity(u_long);
  void start(string &, string &, string &, string &, string &);
  void stop();
  void finalCleanup();
  int terminate();
  int logWriteln(char const*, LoggingLevel level = WARNING);
  int logWriteln(string const &str)
    {return logWriteln(str.c_str());};
  int logPreparePrintError();
  int logEndPrintError();
  int logLockAccess();
  int logUnlockAccess();
  int setLogLocation(string);
  u_long getBuffersize();
  u_long getBuffersize2();
  u_long getThrottlingRate();
  int waitNewConnection(u_long tid, u_long timeout);
  XmlParser *getConfiguration(){return &configurationFileManager;}
  ListenThreads *getListenThreads(){return &listenThreads;}

  void *getEnvString(){return envString;}
  VhostManager *getVhosts(){return vhostList;}
  MimeManager *getMimeManager(){return mimeManager;}

  void setProcessPermissions();
  ConnectionsScheduler* getConnectionsScheduler(){return &connectionsScheduler;}
  int deleteConnection(ConnectionPtr);

  int notifyDeleteConnection(ConnectionPtr);

  void increaseFreeThread();
  void decreaseFreeThread();

  SecurityManager *getSecurityManager (){return &securityManager;}
  AuthMethodFactory *getAuthMethodFactory () {return &authMethodFactory;}
  ValidatorFactory *getValidatorFactory (){return &validatorFactory;}


private:
  friend class ClientsThread;
#ifdef WIN32
  friend int __stdcall control_handler(u_long control_type);
#endif
#ifdef NOT_WIN
  friend int control_handler (u_long control_type);
#endif
  /*!
   *When the flag mustEndServer is 1 all the threads are
   *stopped and the application stop its execution.
   */
  int mustEndServer;

  /*! Singleton instance.  Call createInstance before use it.  */
  static Server* instance;

  /*! Do not allow to create directly objects.  */
  Server();

  void mainLoop();
  void loadPlugins();
  void displayBoot();
  int postLoad();
  void initLogManager ();

  CachedFileFactory cachedFiles;

  void *envString;
  VhostManager *vhostList;
  MimeManager *mimeManager;
  HomeDir homeDir;
  HashMap<string, string*> hashedData;
  HashMap<string, void*> globalData;
  FiltersFactory filtersFactory;
  //DynamicFiltersManager filters;
  //DynExecutorManager executors;
  u_long uid;
  u_long gid;
  int currentThreadID;
  ProtocolsManager protocols;
  XmlParser configurationFileManager;
  XmlParser languageParser;

  bool autoRebootEnabled;
  bool toReboot;
  bool rebooting;

  LogManager* logManager;
  bool serverReady;
  u_long verbosity;
  u_long throttlingRate;
  u_long buffersize;
  u_long secondaryBufferSize;
  /*! Buffer that contains all the local machine IP values.  */
  string *ipAddresses;
  char serverName[HOST_NAME_MAX + 1];
  string* path;
  string* externalPath;
  string* serverAdmin;
  int initialize();
  int addThread(bool staticThread = false);
  ConnectionPtr addConnectionToList(Socket* s, MYSERVER_SOCKADDRIN *asock_in,
                                    char *ipAddr, char *localIpAddr,
                                    u_short port, u_short localPort, int);
  u_long maxConnections;
  u_long maxConnectionsToAccept;
  void clearAllConnections();
  int freeHashedData();
  u_long connectionTimeout;
  u_long maxLogFileSize;
  int copyConfigurationFromDefault(const char *);
  void logWriteNTimes(string, unsigned);
  int checkConfigurationPaths();
  bool resetConfigurationPaths(string &, string &, string &, string &, string &);
  Mutex* connectionsMutex;
  u_long nStaticThreads;
  u_long nMaxThreads;
  u_long freeThreads;

  u_long purgeThreadsThreshold;

  Mutex* threadsMutex;
  list<ClientsThread*> threads;

  int purgeThreads();
  int reboot();
  string* languageFile;
  string* languagesPath;
  string* mainConfigurationFile;
  string* vhostConfigurationFile;
  string* mimeConfigurationFile;
  PluginsManager pluginsManager;
  ProcessServerManager processServerManager;
  ConnectionsScheduler connectionsScheduler;
  ListenThreads listenThreads;
  bool endServer;
  string logLocation;
  AuthMethodFactory authMethodFactory;
  ValidatorFactory validatorFactory;
  SecurityManager securityManager;
};

#endif
