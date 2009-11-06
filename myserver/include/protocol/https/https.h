/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2008, 2009 Free Software Foundation,
  Inc.
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

#ifndef HTTPS_H
# define HTTPS_H
# include "stdafx.h"
# include <include/protocol/http/http.h>

class Https : public Http
{
public:
  static const char* getNameImpl ();
  virtual const char* getName ();
  Https (HttpProtocol *https);
  virtual ~Https ();

};

/*!
 *Adapter class to make Https reentrant.
 */
class HttpsProtocol : public Protocol
{
public:
  HttpsProtocol (HttpProtocol *http)
  {
    protocolOptions = Protocol::SSL;
    this->http = http;
  }

  virtual ~HttpsProtocol ()
  {

  }

  virtual const char* getName ()
  {
    return Https::getNameImpl ();
  }

  virtual int controlConnection (ConnectionPtr con, char *request,
                                 char *auxBuf, u_long reqBufLen,
                                 u_long auxBufLen, u_long reqLen,
                                 u_long tid)
  {
    Https https (http);
    return https.controlConnection (con, request, auxBuf, reqBufLen, auxBufLen,
                                    reqLen, tid);
  }
protected:
  HttpProtocol *http;
};

#endif
