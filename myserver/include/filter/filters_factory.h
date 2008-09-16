/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007 Free Software Foundation, Inc.
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

#ifndef FILTERS_FACTORY_H
#define FILTERS_FACTORY_H

#include "stdafx.h"
#include <include/filter/stream.h>
#include <include/filter/filter.h>
#include <include/filter/filters_chain.h>
#include <include/base/hash_map/hash_map.h>

#include <list>

using namespace std;

typedef Filter* (*FILTERCREATE)(const char* name);


class FiltersFactory 
{
public:
	class FiltersSource
	{
	public:
		virtual Filter* createFilter(const char* name) = 0;
		virtual ~FiltersSource(){}
	};
  int insert(const char*, FILTERCREATE ptr);
  int insert(const char*, FiltersSource* ptr);
  Filter *getFilter(const char*);
  FiltersChain* chain(list<string> &l, Stream* out, u_long *nbw, 
											int onlyNotModifiers = 0);
  int chain(FiltersChain*, list<string> &l, Stream* out, u_long *nbw, 
             int onlyNotModifiers = 0);
  FiltersFactory();
  ~FiltersFactory();
  void free();
protected:
  HashMap<string, FILTERCREATE> staticFilters;
  HashMap<string, FiltersSource*> dynamicFilters;
};


#endif
