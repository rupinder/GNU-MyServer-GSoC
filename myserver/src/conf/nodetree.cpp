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

#include <list>
#include <stdlib.h>
#include <string.h>
#include <string>

using namespace std;

template <typename T>
class NodeTree
{
public:
  NodeTree ()
  {
    value = NULL;
    children = NULL;
  }

  NodeTree (T& v)
  {
    value = new T (v);
    children = NULL;
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
      {
        delete value;
      }
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
  T *value;
};
