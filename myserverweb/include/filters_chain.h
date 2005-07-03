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

#ifndef FILTERS_CHAIN_H
#define FILTERS_CHAIN_H
#include "../stdafx.h"
#include "../include/stream.h"
#include "../include/filter.h"
#include <list>

using namespace std;

class FiltersChain 
{
protected:
  list <Filter*> filters;
  Filter* firstFilter;
  Stream *stream;
public:
  void setStream(Stream*);
  Stream* getStream();
  Filter* getFirstFilter();
  int isEmpty();
  int addFilter(Filter*,u_long *nbw);
  void clearAllFilters();
  int isFilterPresent(Filter*);
  int removeFilter(Filter*);
  int clear();
  int hasModifiersFilters();
  int read(char* buffer, u_long len, u_long*);
  int write(char* buffer, u_long len, u_long*);
	int flush(u_long*);
  FiltersChain();
  ~FiltersChain();
};


#endif
