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

#include "stdafx.h"
#include <include/base/bitvec/bitvec.h>
#include <string.h>


#if !HAVE_FFSL
/* FIXME: move somewhere else.  */
int ffsl (long int i)
{
  for (int j = 0; i < sizeof (i)*8; i++)
    if ((i >> j) & 1)
      return j + 1;

  return 0;
}
#endif

BitVec::BitVec (int capacity, bool val)
{
  this->capacity = capacity;
  this->dataSize = capacity / (sizeof (int) * 8) + 1;
  this->data = new int[dataSize];

  for (int i = 0; i < dataSize; i++)
    this->data[i] = 0;

  if (val)
    for (int i = 0; i < capacity; i++)
      set (i);
}

int BitVec::ffs ()
{
  int p = 0;
  for (int i = 0; i < dataSize; i++)
    if (p = ffsl ((int)data[i]))
      {
        return (i * sizeof (int) * 8) + p - 1;
      }

  return -1;
}
