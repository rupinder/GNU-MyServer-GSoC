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

#ifndef BITVEC_H
#define BITVEC_H

# include "stdafx.h"

class BitVec
{
public:
  BitVec (int capacity, bool val);

  int ffs ();

  void set (int i)
  {
    data[i / (sizeof (long) * 8)] |= 1 << i % (sizeof (long) * 8);
  }

  void unset (int i)
  {
    data[i / (sizeof (long) * 8)] &= ~(1 << i % (sizeof (long) * 8));
  }

  bool get (int i)
  {
    return data[i / (sizeof (long) * 8)] >> i % (sizeof (long) * 8);
  }

  ~BitVec ()
  {
    delete [] data;
  }
  

private:
  long *data;
  int dataSize;
  int capacity;
};

#endif
