/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2007, 2009, 2010 Free Software Foundation, Inc.
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

#ifndef READ_WRITE_LOCK_H
# define READ_WRITE_LOCK_H

# include "myserver.h"
# include <include/base/sync/semaphore.h>

class ReadWriteLock
{
public:
  ReadWriteLock (int maxReaders);
  ~ReadWriteLock ();
  void readLock ();
  void readUnlock ();

  void writeLock ();
  void writeUnlock ();
private:
  int maxReaders;
  Semaphore semaphore;
};
#endif
