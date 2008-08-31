/*
MyServer
Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.
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

#ifndef DYNAMIC_FILTER_H
#define DYNAMIC_FILTER_H
#include "stdafx.h"
#include <include/filter/stream.h>
#include <include/base/dynamic_lib/dynamiclib.h>
#include <include/base/xml/xml_parser.h>
#include <include/filter/filters_factory.h>
#include <include/base/hash_map/hash_map.h>
#include <include/base/thread/thread.h>
#include <include/base/sync/mutex.h>
#include <include/plugin/plugin.h>
#include <include/plugin/filter/dyn_filter_file.h>

using namespace std;

class DynamicFilter : public Filter
{
protected:
  DynamicFilterFile* file;
  u_long id;
public:
  void setId(u_long);
  u_long getId();
  virtual int getHeader(char* buffer, u_long len, u_long* nbw);
  virtual int getFooter(char* buffer, u_long len, u_long* nbw);
  virtual int read(char* buffer, u_long len, u_long*);
  virtual int write(const char* buffer, u_long len, u_long*);
	virtual int flush(u_long*);
	virtual int modifyData();
  virtual const char* getName(char*, u_long);
  void setParent(Stream*);
  Stream* getParent();
  DynamicFilter(DynamicFilterFile*);
  DynamicFilter(DynamicFilterFile*,Stream*, u_long);
  ~DynamicFilter();
};

#endif
