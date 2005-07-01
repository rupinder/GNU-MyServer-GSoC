/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef FILTERS_FACTORY_H
#define FILTERS_FACTORY_H
#include "../stdafx.h"
#include "../include/stream.h"
#include "../include/filter.h"
#include "../include/filters_chain.h"
#include "../include/hash_dictionary.h"

#include <list>

using namespace std;

typedef Filter* (*FILTERCREATE)(const char* name); 

class FiltersFactory 
{
protected:
  HashDictionary<FILTERCREATE> dictionary;
public:
  int insert(const char*, FILTERCREATE ptr);
  Filter *getFilter(const char*);
  FiltersChain* chain(list<string*> l);
  FiltersFactory();
  ~FiltersFactory();
  void free();
};


#endif
