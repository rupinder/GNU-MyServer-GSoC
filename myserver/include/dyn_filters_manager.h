/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007 The MyServer Team
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

#ifndef DYNAMIC_FILTERS_MANAGER_H
#define DYNAMIC_FILTERS_MANAGER_H

#include "../stdafx.h"
#include "../include/stream.h"
#include "../include/dynamiclib.h"
#include "../include/xml_parser.h"
#include "../include/filters_factory.h"
#include "../include/hash_map.h"
#include "../include/thread.h"
#include "../include/mutex.h"
#include "../include/plugin.h"
#include "../include/dyn_filter_file.h"
#include "../include/plugins_namespace_manager.h"

using namespace std;

class DynamicFiltersManager : public PluginsNamespaceManager, 
	public FiltersFactory::FiltersSource
{
public:
  DynamicFiltersManager();
  ~DynamicFiltersManager();
  int registerFilters(FiltersFactory* ff);
  Filter* createFilter(const char* name); 
  DynamicFilterFile* getPlugin(string& name)
	{
		return (DynamicFilterFile*)PluginsNamespaceManager::getPlugin(name);
	}
protected:
  int add(const char*, XmlParser*, Server*);
	void clear();
private:
	u_long counter;
	Mutex counterMutex;

};

#endif
