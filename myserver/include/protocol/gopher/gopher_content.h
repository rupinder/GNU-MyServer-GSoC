/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2002-2009 Free Software Foundation, Inc.
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

#ifndef GOPHER_CONTENT_H
#define GOPHER_CONTENT_H

#include "stdafx.h"
#include <include/base/socket/socket.h>
#include <include/base/mem_buff/mem_buff.h>
#include <include/base/file/file.h>
#include <include/connection/connection.h>
#include <vector>
#include <string>

using namespace std;

class GopherContent
{
public:
  virtual void toProtocol (Socket *s) = 0;
};

class GopherRequest
{
public:
  GopherRequest ();
  GopherRequest (string s){ data = s; }
  GopherRequest (string s, ConnectionPtr conn)
  {
    data = s;
    baseReq = conn;
  }
  void setConnection (ConnectionPtr conn){ baseReq = conn; }
  string getRequest (){ return data; }
  ConnectionPtr getConnection (){ return baseReq; }
protected:
  string data;
  ConnectionPtr baseReq;
};

class GopherItem: public GopherContent
{
 public:
  GopherItem ();
  GopherItem (char, const char[], const char[], const char[], const char[]);

  ~GopherItem ();

  void Type (char T){ type=T; }
  char Type (){ return type; }
  void Desc (string D){ desc=D; }
  string Desc (){ return desc; }
  void Rlocation (string R){ rlocation=R; }
  string Rlocation (){ return rlocation; }
  void Host (string H){ host = H; }
  string Host (){ return host; }
  void Port (string P){ port=P; }
  string Port (){ return port; }

  void toProtocol (Socket *s);

 protected:
  char type;
  string desc;
  string rlocation;
  string host;
  string port;

  void init (char, const char[], const char[], const char[], const char[]);

};

class GopherMenu: public GopherContent
{
public:
  void addIntro (string tmp, string host, string port);
  void addItem (GopherItem data);

  void toProtocol (Socket *s);

private:
  vector<GopherItem> Items;

};


class GopherInfo: public GopherItem
{
public:
  GopherInfo ();
  GopherInfo (const char[], const char[], const char[], const char[]);
  ~GopherInfo ();

  void Type (char T){ type='i'; }
};

class GopherFile: public GopherItem
{
public:
  GopherFile ();
  GopherFile (const char[], const char[], const char[], const char[]);
  ~GopherFile ();

  void Type (char T){ type='0'; }
};

class GopherDirectory: public GopherItem
{
public:
  GopherDirectory ();
  GopherDirectory (const char[], const char[], const char[], const char[]);
  ~GopherDirectory ();

  void Type (char T){ type='1'; }
};

class GopherCSO: public GopherItem
{
public:
  GopherCSO ();
  GopherCSO (const char[], const char[], const char[], const char[]);
  ~GopherCSO ();

  void Type (char T){ type='2'; }
};
class GopherError: public GopherItem
{
public:
  GopherError ();
  GopherError (const char[], const char[], const char[], const char[]);
  ~GopherError ();

  void Type (char T){ type='3'; }
};

class GopherMacBinHex: public GopherItem
{
public:
  GopherMacBinHex ();
  GopherMacBinHex (const char[], const char[], const char[], const char[]);
  ~GopherMacBinHex ();

  void Type (char T){ type='4'; }
};

class GopherDosBin: public GopherItem
{
public:
  GopherDosBin ();
  GopherDosBin (const char[], const char[], const char[], const char[]);
  ~GopherDosBin ();

  void Type (char T){ type='5'; }
};

class GopherUnixUUencode: public GopherItem
{
public:
  GopherUnixUUencode ();
  GopherUnixUUencode (const char[], const char[], const char[], const char[]);
  ~GopherUnixUUencode ();

  void Type (char T){ type='6'; }
};

class GopherSearchServer: public GopherItem
{
public:
  GopherSearchServer ();
  GopherSearchServer (const char[], const char[], const char[], const char[]);
  ~GopherSearchServer ();

  void Type (char T){ type='7'; }
};

class GopherTelnet: public GopherItem
{
public:
  GopherTelnet ();
  GopherTelnet (const char[], const char[], const char[], const char[]);
  ~GopherTelnet ();

  void Type (char T){ type='8'; }
};

class GopherBin: public GopherItem
{
public:
  GopherBin ();
  GopherBin (const char[], const char[], const char[], const char[]);
  ~GopherBin ();

  void Type (char T){ type='9'; }
};

class GopherRendundantServer: public GopherItem
{
public:
  GopherRendundantServer ();
  GopherRendundantServer (const char[], const char[], const char[], const char[]);
  ~GopherRendundantServer ();

  void Type (char T){ type='+'; }
};

class GopherGif: public GopherItem
{
public:
  GopherGif ();
  GopherGif (const char[], const char[], const char[], const char[]);
  ~GopherGif ();

  void Type (char T){ type='g'; }
};

class GopherImage: public GopherItem
{
public:
  GopherImage ();
  GopherImage (const char[], const char[], const char[], const char[]);
  ~GopherImage ();

  void Type (char T){ type='I'; }
};

class GopherData: public GopherContent
{
public:
  GopherData (string);

  void toProtocol (Socket *s);

private:
  string buff;
};


class GopherFileContent: public GopherContent
{
public:
  GopherFileContent (string s);
  ~GopherFileContent ();

  void toProtocol (Socket *s);

private:
  File *sfile;
  MemBuf buf;
};

#endif
