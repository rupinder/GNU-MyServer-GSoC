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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef DYNAMIC_FILTER_H
#define DYNAMIC_FILTER_H
#include "../stdafx.h"
#include "../include/stream.h"
#include "../include/dynamiclib.h"
#include "../include/cXMLParser.h"
#include "../include/filters_factory.h"
#include "../include/hash_dictionary.h"
#include "../include/threads.h"

using namespace std;

class DynamicFilterFile
{
protected:
  DynamicLibrary file;
public:
  DynamicFilterFile();
  ~DynamicFilterFile();
  int loadFromFile(const char* name);
  int close();
  int getHeader(u_long id, Stream* s, char* buffer, u_long len, u_long* nbw);
  int getFooter(u_long id, Stream* s, char* buffer, u_long len, u_long* nbw);
  int read(u_long id, Stream* s, char* buffer, u_long len, u_long*);
  int write(u_long id, Stream* s, const char* buffer, u_long len, u_long*);
	int flush(u_long id, Stream* s, u_long*);
	int modifyData(u_long id, Stream* s);
  const char* getName(u_long id, Stream* s, char*, u_long);
};

class DynamicFilter : public Filter
{
protected:
  Stream *stream;
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

class DynamicFiltersManager
{
protected:
  static Mutex counterMutex;
  static u_long counter;
  static DynamicFiltersManager* dynamicfiltersmanager;
  HashDictionary<DynamicFilterFile*> filters;
  int add(const char*, XmlParser*, Server*);
  static Filter* createFilter(const char* name); 
public:
  DynamicFiltersManager();
  ~DynamicFiltersManager();
  void clear();
  int registerFilters(FiltersFactory* ff);
  int loadFilters(const char*, XmlParser*, Server*);
};

#endif
