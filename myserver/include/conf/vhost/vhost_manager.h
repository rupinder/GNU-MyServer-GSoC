/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2007 Free Software Foundation, Inc.
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

#ifndef VHOST_MANAGER_H
#define VHOST_MANAGER_H

#include "stdafx.h"
#include <include/conf/vhost/vhost.h>
#include <include/log/log_manager.h>

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
  VhostManager(ListenThreads* lt, LogManager* lm);
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
  int loadXMLConfigurationFile(const char *);
	
  /*! Set the right owner for the log locations.  */
  void changeLocationsOwner ();
private:
  void loadXMLlogData (string, Vhost*, xmlNode*);
  ListenThreads* listenThreads;
  Mutex mutex;
  VhostSource* extSource;

  /*! List of virtual hosts. */
  list<Vhost*> hostList;
  LogManager* logManager;
};


#endif
