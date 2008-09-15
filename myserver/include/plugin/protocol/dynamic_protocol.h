/* -*- mode: cpp-mode */
/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007 Free Software Foundation, Inc.
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

#ifndef DYNAMIC_PROTOCOL_H
#define DYNAMIC_PROTOCOL_H

#include "stdafx.h"
#include <include/base/xml/xml_parser.h>
#include <include/protocol/protocol.h>
#include <include/connection/connection.h>
#include <include/base/dynamic_lib/dynamiclib.h>
#include <include/plugin/plugin.h>
#include <include/plugin/plugins_namespace_manager.h>

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
