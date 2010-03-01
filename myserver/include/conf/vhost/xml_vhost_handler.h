/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2007, 2009, 2010 Free Software Foundation, Inc.
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

#ifndef XML_VHOST_H
# define XML_VHOST_H

# include "myserver.h"
# include <include/conf/vhost/vhost_manager.h>

# include <vector>

using namespace std;

class XmlVhostHandler : public VhostManagerHandler
{
public:
  XmlVhostHandler (ListenThreads* lt, LogManager* lm);
  ~XmlVhostHandler ();
  int getHostsNumber ();
  Vhost* getVHost (int n);
  void clean ();
  int removeVHost (int n);
  int switchVhosts (int n1, int n2);
  vector<Vhost*>* getVHostList ();

  /*! Get a pointer to a vhost.  */
  Vhost* getVHost (const char*, const char*, u_short);

  /*! Add an element to the vhost list.  */
  int addVHost (Vhost*);

  /*! Load the virtual hosts list from a xml configuration file.  */
  virtual int load (const char *);

  /*! Set the right owner for the log locations.  */
  void changeLocationsOwner ();

  static void registerBuilder (VhostManager& manager);

protected:
  void loadXMLlogData (string, Vhost*, xmlNode*);
  ListenThreads* listenThreads;

  /*! List of virtual hosts. */
  vector<Vhost*> hosts;
  LogManager* logManager;
};

#endif
