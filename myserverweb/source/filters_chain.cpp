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

 
#include "../stdafx.h"
#include "../include/filters_chain.h"
#include "../include/stringutils.h"

#include <string>
#include <sstream>

using namespace std;

/*!
 *Read data from the chain. Returns 0 on success.
 */
int FiltersChain::read(char* buffer, u_long len, u_long* nbr)
{
  if(stream == 0)
    return -1;
  if(firstFilter)
    return firstFilter->read(buffer, len, nbr);
  
  return stream->read(buffer, len, nbr);
}

/*!
 *Set the stream where apply the filters.
 */
void FiltersChain::setStream(Stream* s)
{
  list<Filter*>::iterator i=filters.begin();

  for( ; i!=filters.end(); i++ )
    if((*i)->getParent()==stream)
    {
      (*i)->setParent(s);
    }
  stream=s;
}

/*!
 *Get the stream usedby the chain.
 */
Stream* FiltersChain::getStream()
{
  return stream;
}

/*!
 *Write data using the chain. Returns 0 on success.
 */
int FiltersChain::write(const char* buffer, u_long len, u_long* nbw)
{
  if(stream == 0)
    return -1;
  if(firstFilter)
    return firstFilter->write(buffer, len, nbw);

  return stream->write(buffer, len, nbw);
}

/*!
 *Initialize the chain object.
 */
FiltersChain::FiltersChain()
{
  acceptDuplicates=1;
  stream=0;
  firstFilter=0;
}

/*!
 *Set if the chain can cointain duplicates of the same filter on different levels.
 */
void FiltersChain::setAcceptDuplicates(int v)
{
  acceptDuplicates = v;
}

/*!
 *Return if the chain can have the same filter in multiple places. 
 */
int FiltersChain::getAcceptDuplicates()
{
  return acceptDuplicates;
}

/*!
 *Destroy the chain.
 */
FiltersChain::~FiltersChain()
{

}

/*!
 *Add a filter to the chain. 
 *Returns 0 on success.
 *the number of bytes written to initialize the filter. 
 */
int FiltersChain::addFilter(Filter* f, u_long *nbw, int sendData)
{
  u_long ret = 0;
  char buffer[512];
  u_long nbw2;
  u_long nbwFirstFilter;

  if(!firstFilter)
  {
    f->setParent(stream);
  }
  else
  {
    f->setParent(firstFilter);
  }

  /*! If the chain doesn't support duplicates check if the filter is already present.*/
  if(!acceptDuplicates)
  {
    if(isFilterPresent(f))
      return 0;
  }

  if(sendData)
  {
    /*! Write the filter header(if any) using the upper chain. */
    if(!f->getHeader(buffer, 512, &nbw2))
    {
      if(nbw2 && f->getParent()->write(buffer, nbw2, &nbwFirstFilter))
        ret = 1;
      *nbw = nbwFirstFilter;
    }
    else
    {
      return 1;
    }
  }

  /*! 
   *Add the new filter at the end of the list.
   *The new filter will write directly the old firstFilter.
   */
  firstFilter=f;

  /*! Add the filters in the list in the same order they are used. */
  filters.push_front(f);
  return ret;
}

/*! 
 *Flush remaining data.
 *Additional footer data for filters is added at the end.
 *Returns 0 on success. 
 */
int FiltersChain::flush(u_long* nbw)
{
  u_long written=0;
  char buffer[512];
  list<Filter*>::iterator i;
  *nbw=0;
  if(firstFilter!=0)
  {
    if(firstFilter->flush(nbw))
      return 1;
  }
  else if(stream)
  {
    if(stream->flush(nbw))
      return 1;
  }
  written=*nbw;

  ;
  /*!
   *Position on the last element. 
   */
  i = filters.end();

  while(i != filters.begin())
  {
    Filter* f;
    u_long tmpNbw=0;

    --i;
    f = *i;

    if(f->getFooter(buffer, 512, &tmpNbw))
      return -1;
    f->getParent()->write(buffer, tmpNbw, nbw);
    written += (*nbw);
  }
  
  /*! Set the final value. */
  *nbw=written;
  return 0;
}

/*!
 *Check if a filter is present in the chain.
 */
int FiltersChain::isFilterPresent(Filter* f)
{
  list<Filter*>::iterator i=filters.begin();

  for( ; i!=filters.end(); ++i )
    if(*i==f)
      return 1;

  return 0;
}

/*!
 *Check if a filter is present in the chain by its name.
 */
int FiltersChain::isFilterPresent(const char* name)
{
  list<Filter*>::iterator i=filters.begin();

  for( ; i!=filters.end(); ++i )
    if(!lstrcmpi((*i)->getName(0, 0), name))
      return 1;
  return 0;
}

/*!
 *Remove the first occurrence of the specified filter from the chain.
 */
int FiltersChain::removeFilter(Filter* f)
{
  list<Filter*>::iterator i=filters.begin();
  list<Filter*>::iterator prev=filters.end();   
  for( ; i!=filters.end(); ++i )
    if(*i==f)
    {
      if(prev != filters.end())
      {
        (*prev)->setParent((*i)->getParent());
      }
      else
      {
        /*! 
         *If if it the first filter according to the linked list. 
         *Do not use the getParent function here as it can be a Stream.
         */
        if(i == filters.end())
          firstFilter = 0;
        else
          firstFilter=*(++i);
      }
      filters.erase(i);
      break;
    }
      
  return 0;
}

/*!
 *Returns a nonzero value if the chain is empty.
 */
int FiltersChain::isEmpty()
{
  return !firstFilter;
}

/*!
 *Destroy filters objects. This destroys all the filters objects in the list.
 */
void FiltersChain::clearAllFilters()
{
  list<Filter*>::iterator i=filters.begin();
  for( ; i!=filters.end();i++)
  {
    delete (*i);
  }
  filters.clear();
  firstFilter=0;
}

/*!
 *Returns a nonzero value if the chain contains any modifier filter.
 */
int FiltersChain::hasModifiersFilters()
{
  list <Filter*>::iterator i=filters.begin();
  for( ;i!=filters.end(); ++i)
  {
    if((*i)->modifyData())
      return 1;
  }
  return 0;
}

/*!
 *Get the first filter of the chain.
 */
Filter* FiltersChain::getFirstFilter()
{
  return firstFilter;
}


/*!
 *Fullfill the out string with a comma separated list of the filters
 *present in the chain.
 */
void FiltersChain::getName(string& out)
{
  list<Filter*>::iterator i = filters.begin();
  out.clear();

  for(;;)
  {
    if(i != filters.end())
      out.append((*i)->getName(0, 32));
 
    i++;
    if(i != filters.end())
      out.append(",");
    else
      break;
  }

}
