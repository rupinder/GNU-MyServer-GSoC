/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2009 Free Software Foundation, Inc.
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

#ifndef SOCKET_PAIR_H
#define SOCKET_PAIR_H

#include "stdafx.h"
#include <include/filter/stream.h>
#include <include/base/socket/socket.h>
#include <string>

using namespace std;


class SocketPair : public Socket
{
public:
	SocketPair ();
	int create ();
	SocketHandle getFirstHandle ();
	SocketHandle getSecondHandle ();

	void inverted (SocketPair&);
	virtual int close ();
	void closeFirstHandle ();
	void closeSecondHandle ();
  void setNonBlocking (bool blocking);

  int readHandle (Handle*);
  int writeHandle (Handle);

protected:
	SocketHandle handles[2];
};
#endif
