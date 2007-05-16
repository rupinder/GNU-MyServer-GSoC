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

#ifndef READ_WRITE_LOCK_H
#define READ_WRITE_LOCK_H

#include "../stdafx.h"
#include "../include/semaphore.h"

class ReadWriteLock
{
public:
	ReadWriteLock(int maxReaders);
	~ReadWriteLock();
	void readLock();
	void readUnlock();

	void writeLock();
	void writeUnlock();
private:
	int maxReaders;
	Semaphore semaphore;
};
#endif
