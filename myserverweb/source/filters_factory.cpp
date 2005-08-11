/*
MyServer
Copyright (C) 2002, 2003, 2004 The MyServer Team
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

 
#include "../stdafx.h"
#include "../include/filters_factory.h"

#include <string>
#include <sstream>

using namespace std;


/*!
 *Initialize the object.
 */
FiltersFactory::FiltersFactory()
{
  dictionary.free();
}

/*!
 *Destroy the object.
 */
FiltersFactory::~FiltersFactory()
{

}

/*!
 *Insert a filter by name and factory routine. Returns 0 if the entry
 *was added correctly.
 */
int FiltersFactory::insert(const char* name, FILTERCREATE fnc)
{
  return dictionary.insert(name, fnc);
}

/*!
 *Get a new filter by its name. 
 *The object have to be freed after its use to avoid memory leaks.
 *Returns the new created object on success.
 *Returns 0 on errors.
 */
Filter *FiltersFactory::getFilter(const char* name)
{
  FILTERCREATE factory = dictionary.getData(name);
  /*! If the filter exists create a new object and return it. */
  if(factory)
    return factory(name);

  return 0;
}

/*!
 *Create a FiltersChain starting from a list of strings. 
 *On success returns the new object.
 *If specified [onlyNotModifiers] the method wil check that all the filters
 *will not modify the data.
 *On errors returns 0.
 */
FiltersChain* FiltersFactory::chain(list<string*> &l, Stream* out, u_long *nbw, 
                                    int onlyNotModifiers)
{
  FiltersChain *ret = new FiltersChain();
  if(!ret)
    return 0;
  if(chain(ret, l, out, nbw, onlyNotModifiers))
  {
    ret->clearAllFilters();
    delete ret;
    return 0;
  }
  return ret;
}

/*!
 *Add new filters to an existent chain.
 *If specified [onlyNotModifiers] the method wil check that all the filters
 *will not modify the data.
 *On errors returns nonzero.
 */
int FiltersFactory::chain(FiltersChain* c, list<string*> &l, Stream* out, u_long *nbw, 
                          int onlyNotModifiers)
{

  list<string*>::iterator i=l.begin();

  if(!c)
    return 1;

  c->setStream(out);
  *nbw=0;

  for( ; i != l.end(); i++)
  {
    u_long tmp;
    Filter *n=getFilter((*i)->c_str());
    if( !n || ( onlyNotModifiers && n->modifyData() )  )
    {
      c->clearAllFilters();
      return 1;
    }
    c->addFilter(n, &tmp);
    *nbw+=tmp;
  }

  return 0;
}

/*!
 *Free the object.
 */
void FiltersFactory::free()
{
  dictionary.free();
}
