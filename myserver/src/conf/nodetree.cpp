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
#include <list>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <map>

using namespace std;

template <typename T>
class NodeTree
{
public:
  NodeTree ()
  {
    value = NULL;
    children = NULL;
    attrs = NULL;
  }

  NodeTree (T& v)
  {
    value = new T (v);
    children = NULL;
    attrs = NULL;
  }

  ~NodeTree ()
  {
    if (children)
      {
        typename list<NodeTree<T>* >::iterator it = children->begin ();
        while (it != children->end ())
          {
            delete *it;
            it++;
          }
        delete children;
      }

    if (value)
      delete value;

    if (attrs)
      delete attrs;
  }

  void setValue (T *v)
  {
    value = v;
  }

  void addChild (NodeTree<T> *child)
  {
    if (children == NULL)
      children = new list<NodeTree<T>*>;

    children->push_back (child);
  }

  void addAttr (string &name, T value)
  {
    if (attrs == NULL)
      attrs = new list<pair<string, T> > ();

    attrs->push_back (pair<string, T> (name, value));
  }

  T* getAttr (string &name)
  {
    if (attrs == NULL)
      return NULL;
    for (typename list<pair<string, T> >::iterator it = attrs->begin ();
         it != attrs->end ();
         it++)
      {
        if ((*it).first.compare (name) == 0)
          return &((*it).second);
      }

    return NULL;
  }

  bool isLeaf ()
  {
    return children == NULL;
  }

  list<NodeTree<T>*> *getChildren ()
  {
    return children;
  }

  const T *getValue ()
  {
    return value;
  }
protected:
  list<NodeTree<T>*> *children;
  list<pair<string, T> > *attrs;
  T *value;
};
