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

#ifndef SLAB_H
# define SLAB_H


# include "stdafx.h"
# include <include/base/bitvec/bitvec.h>

template<class T>
class Slab
{
public:
  Slab<T> (size_t capacity) : mask (capacity, true)
  {
    this->capacity = capacity;
    data = new T[capacity];
  }

  void init (size_t capacity)
  {
    mask.init (capacity, true);
    this->capacity = capacity;
    delete [] data;
    data = new T[capacity];
  }

  size_t getCapacity ()
  {
    return capacity;
  }

  T* forcedGet ()
  {
    T* ret = get ();

    if (ret)
      return ret;

    return new T ();
  }

  T* get ()
  {
    int i = mask.find ();

    if (i == -1 || i >= capacity)
      return NULL;

    mask.unset (i);

    return &(data[i]);
  }

  void put (T* t)
  {
    size_t i = t - data;

    if (i < capacity)
      mask.set (i);
    else
      delete t;
  }

  ~Slab<T> ()
  {
    delete [] data;
  }

private:
  BitVec mask;
  size_t capacity;
  T *data;
};

#endif
