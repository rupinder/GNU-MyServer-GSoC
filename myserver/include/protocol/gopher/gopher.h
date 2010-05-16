/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
  Free Software Foundation, Inc.
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

#ifndef GOPHER_H
# define GOPHER_H
# include "myserver.h"
# include <include/protocol/protocol.h>
# include <sstream>
# include <include/protocol/gopher/gopher_content.h>

# include <string>
# include <vector>
# include <memory>

class Gopher : public Protocol
{
public:
  Gopher ();
  virtual ~Gopher ();

  virtual const char* getName ()
  {
    return getNameImpl ();
  }

  static const char* getNameImpl ();

  int controlConnection (ConnectionPtr a,
                         char *b1,
                         char *b2,
                         u_long bs1,
                         u_long bs2,
                         u_long nbtr,
                         u_long id);


  static int loadProtocolStatic ();
  static int unLoadProtocolStatic ();

  void reply (ConnectionPtr a,
              GopherContent &data);
};


/*!
  Adapter class to make Gopher reentrant.
 */

class GopherProtocol : public Protocol
{
public:
  GopherProtocol ()
  {
    protocolOptions = 0;
  }

  virtual ~GopherProtocol ()
  {}

  virtual const char* getName ()
  {
    return Gopher::getNameImpl ();
  }

  virtual int controlConnection (ConnectionPtr con, char *request,
                                 char *auxBuf, u_long reqBufLen,
                                 u_long auxBufLen, u_long reqLen,
                                 u_long tid)
  {
    Gopher gopher;
    return gopher.controlConnection (con, request, auxBuf, reqBufLen, auxBufLen,
                                     reqLen, tid);
  }

  virtual int loadProtocol ()
  {
    return Gopher::loadProtocolStatic ();
  }

  virtual int unLoadProtocol ()
  {
    return Gopher::unLoadProtocolStatic ();
  }
};

#endif
