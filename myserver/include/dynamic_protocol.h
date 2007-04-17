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

#ifndef DYNAMIC_PROTOCOL_H
#define DYNAMIC_PROTOCOL_H

#include "../stdafx.h"
#include "../include/xml_parser.h"
#include "../include/protocol.h"
#include "../include/connection.h"
#include "../include/dynamiclib.h"
#include "../include/plugin.h"
#include "../include/plugins_namespace_manager.h"

#include <string>
using namespace std;

class DynamicProtocol : public Protocol, public Plugin
{
public:
	DynamicProtocol();
	virtual ~DynamicProtocol();
	virtual int controlConnection(ConnectionPtr a,char *b1,char *b2,int bs1,
                                int bs2,u_long nbtr,u_long id);
	int getOptions();
};

#endif
