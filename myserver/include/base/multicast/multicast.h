/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2007, 2008 Free Software Foundation, Inc.
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

#ifndef MULTICAST_H
#define MULTICAST_H

#include "stdafx.h"
#include <include/base/hash_map/hash_map.h>

#include <string>
#include <vector>
using namespace std;

template<typename MSG_TYPE, typename ARG_TYPE, typename RET_TYPE>
class MulticastRegistry;

template<typename MSG_TYPE, typename ARG_TYPE, typename RET_TYPE>
class Multicast
{
public:
	virtual RET_TYPE updateMulticast(MulticastRegistry<MSG_TYPE, ARG_TYPE, RET_TYPE>*, MSG_TYPE&, ARG_TYPE) = 0;
	virtual ~Multicast(){}
};

template<typename MSG_TYPE, typename ARG_TYPE, typename RET_TYPE>
class MulticastRegistry
{
public:
	void addMulticast(MSG_TYPE, Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*);
	void removeMulticast(MSG_TYPE, Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*);
	void notifyMulticast(MSG_TYPE&, ARG_TYPE);
protected:
	void removeMulticasts(MSG_TYPE);
	vector<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*>* getHandlers(MSG_TYPE&);
	void clearMulticastRegistry();
private:
	HashMap<MSG_TYPE, vector<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*>*> handlers;
};


#include <src/base/multicast/multicast.cpp>

#endif
