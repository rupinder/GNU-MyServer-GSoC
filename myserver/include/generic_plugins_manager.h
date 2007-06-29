/*
MyServer
Copyright (C) 2007 The MyServer Team
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

#ifndef GENERIC_PLUGINS_MANAGER_H
#define GENERIC_PLUGINS_MANAGER_H

#include "../stdafx.h"
#include "../include/plugins_namespace_manager.h"

using namespace std;

class GenericPluginsManager : public PluginsNamespaceManager
{
public:
	GenericPluginsManager();
};

#endif
