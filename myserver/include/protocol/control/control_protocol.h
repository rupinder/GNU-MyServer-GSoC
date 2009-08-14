/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2004, 2005, 2008, 2009 Free Software Foundation, Inc.
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

#ifndef CONTROL_PROTOCOL_H
#define CONTROL_PROTOCOL_H
#include "stdafx.h"
#include <include/protocol/protocol.h>
#include <include/protocol/control/control_header.h>
#include <include/connections_scheduler/connections_scheduler.h>

#include <string>

using namespace std;

class ControlProtocol : public Protocol, public ConnectionsSchedulerVisitor
{
public:
  virtual int visitConnection(ConnectionPtr, void*);

  int sendResponse(char*, int, ConnectionPtr, int, ControlHeader& header, File* = 0);
  virtual int loadProtocol(XmlParser* languageParser);
	int controlConnection(ConnectionPtr a, char *b1, char *b2, int bs1, 
                        int bs2, u_long nbtr, u_long id);
	virtual char* registerName(char*,int len);
	ControlProtocol();
	virtual ~ControlProtocol();

protected:
  static char adminLogin[64];
  static char adminPassword[64];
  static int controlEnabled;

  int checkAuth(ControlHeader&);
  int showConnections(ConnectionPtr,File* out, char *b1,int bs1, ControlHeader&);
  int showDynamicProtocols(ConnectionPtr,File* out, char *b1,int bs1, ControlHeader&);
  int showLanguageFiles(ConnectionPtr, File* out, char *b1,int bs1, ControlHeader&);
  int killConnection(ConnectionPtr,u_long ID, File* out, char *b1,int bs1, ControlHeader&);
  int getFile(ConnectionPtr, char*, File* in, File* out, 
              char *b1,int bs1, ControlHeader&);
  int putFile(ConnectionPtr,char*, File* in, File* out, 
              char *b1,int bs1, ControlHeader&);
  int getVersion(ConnectionPtr,File* out, char *b1,int bs1, ControlHeader&);
  int addToLog(int retCode, ConnectionPtr con, char *b1, int bs1, ControlHeader&);
};

#endif
