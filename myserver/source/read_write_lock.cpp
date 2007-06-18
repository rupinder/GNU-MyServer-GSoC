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


#include "../stdafx.h"
#include "../include/read_write_lock.h"


/*!
 *Initialize the read write lock.
 *\param maxReaders The max number of readers.
 */
ReadWriteLock::ReadWriteLock(int maxReaders) : semaphore(maxReaders)
{
	ReadWriteLock::maxReaders = maxReaders;
}

/*!
 *Free the used resources.
 */
ReadWriteLock::~ReadWriteLock()
{
	semaphore.destroy();
}

/*!
 *Reader access.
 */
void ReadWriteLock::readLock()
{
	semaphore.lock(1);
}

/*!
 *Reader terminate access.
 */
void ReadWriteLock::readUnlock()
{
	semaphore.unlock(1);
}

/*!
 *Writer access.
 */
void ReadWriteLock::writeLock()
{
	semaphore.lock(maxReaders);
}

/*!
 *Writer terminate access.
 */
void ReadWriteLock::writeUnlock()
{
	semaphore.unlock(maxReaders);
}
