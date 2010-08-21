/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2007, 2008, 2009, 2010 Free Software Foundation, Inc.
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

#ifndef PLUGIN_INFO_H
# define PLUGIN_INFO_H

# include "myserver.h"
# include <include/base/hash_map/hash_map.h>
# include <string>
# include <utility>
# include <include/plugin/plugin.h>
# include <include/base/regex/myserver_regex.h>


using namespace std;

class Server;
class XmlParser;

class PluginInfo
{
public:

  PluginInfo (string& name, bool enabled = true, bool global = false);
  ~PluginInfo ();

  bool isEnabled ();
  bool isGlobal ();

  void addDependence (string, int minVersion, int maxVersion);

  int getVersion ();
  void setVersion (int v);
  string getName ();
  int getMyServerMinVersion ();
  int getMyServerMaxVersion ();
  int setMyServerMinVersion (int v);
  int setMyServerMaxVersion (int v);

  HashMap<string, pair<int,int>* >::Iterator begin (){return dependences.begin ();}
  HashMap<string, pair<int,int>* >::Iterator end (){return dependences.end ();}

  void setPlugin (Plugin* plugin);
  Plugin* getPlugin ();
  Plugin* removePlugin ();
  void setEnabled (bool enabled);
  pair<int,int>* getDependence (string name);

  static int convertVersion (const string& s);

private:
  string name;
  bool enabled;
  bool global;
  int version;
  int msMinVersion;
  int msMaxVersion;
  Plugin* plugin;
  HashMap<string, pair<int,int>* > dependences;
  static Regex* regex;
};

#endif
