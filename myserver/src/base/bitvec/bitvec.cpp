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
  for (int j = 0; j < sizeof (i)*8; j++)
    if ((i >> j) & 1)
      return j + 1;

  return 0;
}
#endif

/*!
 * Allocate a vector of bits.
 *\param capacity Maximum number of bits to index.
 *\param val The default value for the vector elements.
 */
BitVec::BitVec (int capacity, bool val)
{
  this->data = NULL;
  init (capacity, val);
  lastFound = 0;
}

/*!
 * Reinitialize the vector.
 *\param capacity Maximum number of bits to index.
 *\param val The default value for the vector elements.
 */
void BitVec::init (int capacity, bool val)
{
  this->capacity = capacity;
  this->dataSize = capacity / (sizeof (long int) * 8) + 1;

  if (this->data)
    delete [] this->data;

  this->data = new long int[dataSize];

  for (int i = 0; i < dataSize; i++)
    this->data[i] = 0;

  if (val)
    for (int i = 0; i < capacity; i++)
      set (i);

  lastFound = 0;
}


/*!
 * Find the first bit set to '1'.
 *
 *\return the Index of the first bit.
 *\return -1 If there are not free bits.
 */
int BitVec::ffs ()
{
  int p = 0;
  for (int i = 0; i < dataSize; i++)
    if (p = ffsl ((int)data[i]))
      {
        return (i * sizeof (long int) * 8) + p - 1;
      }

  return -1;
}

/*!
 * Find a bit set to '1'.
 *
 * The cost of adding N elements sequentially and after unset them, after the
 * index is found by ffs is O(n^2).
 *
 * Use the `find' function instead to have a O(n) cost.
 *
 *\return the Index of the first bit.
 *\return -1 If there are not free bits.
 */
int BitVec::find ()
{
  int p = 0;

  for (int i = lastFound; i < dataSize + lastFound; i++)
    if (p = ffsl ((int)data[i % dataSize]))
      {
        lastFound = i;
        return (i * sizeof (long int) * 8) + p - 1;
      }

  return -1;
}

