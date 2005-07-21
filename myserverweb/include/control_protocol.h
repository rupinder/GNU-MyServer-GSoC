/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef CONTROL_PROTOCOL_H
#define CONTROL_PROTOCOL_H
#include "../stdafx.h"
#include "../include/protocol.h"
#include "../include/control_header.h"


class ControlProtocol : public Protocol
{
  static char adminLogin[64];
  static char adminPassword[64];
  static int controlEnabled;
  /*! Thread ID. */
  int id;
  /*! Input file. */
  File *Ifile;
  /*! Output file. */
  File *Ofile;
  /*! Protocol level disable */
  bool Reboot;

  /*! Use control_header to parse the request. */
  ControlHeader header;
  int checkAuth();
  int SHOWCONNECTIONS(ConnectionPtr,File* out, char *b1,int bs1);
  int SHOWDYNAMICPROTOCOLS(ConnectionPtr,File* out, char *b1,int bs1);
  int SHOWLANGUAGEFILES(ConnectionPtr, File* out, char *b1,int bs1);
  int KILLCONNECTION(ConnectionPtr,u_long ID, File* out, char *b1,int bs1);
  int GETFILE(ConnectionPtr, char*, File* in, File* out, 
              char *b1,int bs1 );
  int PUTFILE(ConnectionPtr,char*, File* in, File* out, 
              char *b1,int bs1 );
  int GETVERSION(ConnectionPtr,File* out, char *b1,int bs1);
  int addToErrorLog(ConnectionPtr con, char *b1, int bs1);
  int addToLog(int retCode, ConnectionPtr con, char *b1, int bs1);
public:
  int sendResponse(char*, int, ConnectionPtr, int, File* = 0);
  static int loadProtocol(XmlParser* languageParser);
	int controlConnection(ConnectionPtr a, char *b1, char *b2, int bs1, 
                        int bs2, u_long nbtr, u_long id);
	virtual char* registerName(char*,int len);
	ControlProtocol();
	virtual ~ControlProtocol();
};

#endif
