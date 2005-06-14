/*
*MyServer
*Copyright (C) 2005 The MyServer Team
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
#ifndef DICTIONARY_H
#define DICTIONARY_H

#include <map>

using namespace std;

/*! Handle a dictionary of strings trough hashing.*/
class HashDictionary
{
private:
  struct sNode
  {
    unsigned int hash;
    void *data;
  };
  map<int, sNode*> data;
public:
  static unsigned int hash(const char *);
  HashDictionary();
  ~HashDictionary();
  void *getData(const char*);
  void *getData(int);
  int insert(const char*, void*);
  void* removeNode(const char*);
  void* removeNodeAt(int);
  int nodesNumber();
  void free();
  int isEmpty();
};


#endif
