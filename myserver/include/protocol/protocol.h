/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2008, 2009, 2010 Free Software
  Foundation, Inc.
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
# include "myserver.h"
# include <include/base/xml/xml_parser.h>
# include <include/connection/connection.h>

/*!
  This is the base class to derive other protocols implementations for
  the server.  */
class Protocol
{
public:
  /*! Various options that can be use for the protocol.  */
  enum
    {
      SSL = 1,
      FAST_CHECK = 2,
      DENY_DELETE = 4
    };

  Protocol ();
  virtual ~Protocol ();
  virtual const char* getName ();

  /*!
    Entry point to check new data available from a client.

    \param con Connection structure with all data relative to the connection.
    \param request Request body ready from the client (it may be incomplete and
    it is protocol responsibility to check if it is complete or not.  This buffer
    can be used for other purposes too by the protocol, it is not required to be
    unchanged.
    \param auxBuf Auxiliary buffer available to the current thread.
    \param reqBufLen Length in bytes of the buffer containing the request.
    \param auxBufLen Length in bytes of the auxiliary buffer.
    \param reqLen Number of bytes read from the client in REQUEST.
    \param tid current thread id.
  */
  virtual int controlConnection (ConnectionPtr con, char *request,
                                 char *auxBuf, u_long reqBufLen,
                                 u_long auxBufLen, u_long reqLen,
                                 u_long tid) = 0;

  virtual int loadProtocol ();
  virtual int unLoadProtocol ();

  virtual int getProtocolOptions (){return protocolOptions;}

  virtual string &getProtocolPrefix (){return protocolPrefix;}
protected:
  string protocolPrefix;
  int protocolOptions;
};
#endif
