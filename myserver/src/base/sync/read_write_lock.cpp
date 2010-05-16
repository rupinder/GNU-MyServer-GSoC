/*
  MyServer
  Copyright (C) 2007, 2008, 2009, 2010 Free Software Foundation, Inc.
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


#include "myserver.h"
#include <include/base/sync/read_write_lock.h>


/*!
  Initialize the read write lock.
  \param maxReaders The max number of readers.
 */
ReadWriteLock::ReadWriteLock (int maxReaders) : semaphore (maxReaders)
{
  ReadWriteLock::maxReaders = maxReaders;
}

/*!
  Free the used resources.
 */
ReadWriteLock::~ReadWriteLock ()
{
  semaphore.destroy ();
}

/*!
  Reader access.
 */
void ReadWriteLock::readLock ()
{
  semaphore.lock (1);
}

/*!
  Reader terminate access.
 */
void ReadWriteLock::readUnlock ()
{
  semaphore.unlock (1);
}

/*!
  Writer access.
 */
void ReadWriteLock::writeLock ()
{
  for (int i = 0; i < maxReaders; i++)
    semaphore.lock ();
}

/*!
  Writer terminate access.
 */
void ReadWriteLock::writeUnlock ()
{
  for (int i = 0; i < maxReaders; i++)
    semaphore.unlock ();
}

