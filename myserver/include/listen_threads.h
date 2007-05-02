/*
MyServer
Copyright (C) 2007 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#ifndef LISTEN_THREADS_H
#define LISTEN_THREADS_H
#include "../stdafx.h"
#include "../include/xml_parser.h"
#include "../include/mutex.h"
#include "../include/hash_map.h"

class ListenThreads
{
public:
	void addListeningThread(u_short port);
	u_long getThreadsCount(){return count;}
	int initialize(XmlParser* parser);
	int terminate();
	void increaseCount();
	void decreaseCount();
private:
	HashMap<u_short, bool> usedPorts;
	Mutex countMutex;
	XmlParser *languageParser;
	u_long count;
	int createServerAndListener(u_short port);

};

#endif
