/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2002, 2003, 2004, 2008, 2009 Free Software Foundation, Inc.
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

#ifndef PROTOCOL_H
# define PROTOCOL_H
# include "stdafx.h"
# include <include/base/xml/xml_parser.h>
# include <include/connection/connection.h>
/*! Various options that can be use for the protocol.  */
# define PROTOCOL_USES_SSL 1
# define PROTOCOL_FAST_CHECK 2
# define PROTOCOL_DENY_DELETE 4

/*!
 *This is the base class to derive other protocols implementations for the server.
 */
class Protocol
{
public:
	Protocol ();
  virtual ~Protocol ();
	virtual char* registerName (char*,int len);
	virtual int controlConnection (ConnectionPtr a, char *b1, char *b2,
                                 int bs1, int bs2, u_long nbtr, u_long id);
	virtual int loadProtocol ();
	virtual int unLoadProtocol ();

  virtual int getProtocolOptions (){return protocolOptions;}

  virtual string &getProtocolPrefix (){return protocolPrefix;}
protected:
  string protocolPrefix;
	int protocolOptions;
};
#endif
