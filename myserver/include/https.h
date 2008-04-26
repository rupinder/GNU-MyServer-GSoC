/*
MyServer
Copyright (C) 2002, 2003, 2004, 2008 The MyServer Team
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
#define HTTPS_H
#include "../stdafx.h"
#include "../include/http.h"

class Https : public Http
{
public:
  static char* registerNameImpl(char* out, int len);
	virtual char* registerName(char*,int len);
	Https();
	virtual ~Https();

  static int loadProtocolStatic(XmlParser* lang)
  {
    return Http::loadProtocolStatic(lang);
  }
  static int unLoadProtocolStatic(XmlParser* lang)
  {
    return Http::unLoadProtocolStatic(lang);
  }

};

/*!
 *Adapter class to make Https reentrant.
 */
class HttpsProtocol : public Protocol
{
public:
	HttpsProtocol()
  {
    protocolOptions = PROTOCOL_USES_SSL;
  }

  virtual ~HttpsProtocol()
  {

  }

  virtual char* registerName(char* out, int len)
  {
    return Https::registerNameImpl(out, len);
  }

	virtual int controlConnection(ConnectionPtr a, char *b1, char *b2,
                                int bs1, int bs2, u_long nbtr, u_long id)
  {
    int ret = 0;
    Https* https = new Https ();

    ret = https->controlConnection(a, b1, b2, bs1, bs2, nbtr, id);
    
    delete https;

    return ret;
  }

  virtual int loadProtocol(XmlParser* parser)
  {
    return Https::loadProtocolStatic(parser);
  }
  
	virtual int unLoadProtocol(XmlParser* parser)
  {
    return Https::unLoadProtocolStatic(parser);
  }
};

#endif
