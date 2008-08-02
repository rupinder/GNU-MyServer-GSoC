/*
MyServer
Copyright (C) 2005, 2006, 2007, 2008 Free Software Foundation, Inc.
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

#include "../stdafx.h"
#include "../include/stream.h"
#include "../include/filter.h"
#include "../include/dyn_filter.h"
#include "../include/find_data.h"
#include "../include/server.h"
#include <string>
#include <sstream>

using namespace std;

typedef int (*getHeaderPROC)(u_long, void*, char*, u_long, u_long*); 
typedef int (*getFooterPROC)(u_long, void*, char*, u_long, u_long*);
typedef int (*readPROC)(u_long, void*, char*, u_long, u_long*);
typedef int (*writePROC)(u_long, void*, const char*, u_long, u_long*);
typedef int (*flushPROC)(u_long, void*, u_long*);
typedef int (*modifyDataPROC)(u_long, void*);

/*!
 *Read [len] characters using the filter. Returns -1 on errors.
 */
int DynamicFilter::read(char* buffer, u_long len, u_long *nbr)
{
  *nbr=0; 
  if(!file)
    return 0;

  return file->read(id, parent, buffer, len, nbr);
}

/*!
 *Write [len] characters to the stream. Returns -1 on errors.
 */
int DynamicFilter::write(const char* buffer, 
                         u_long len, u_long* nbw)
{
  *nbw=0; 
  if(!file)
    return 0;
  
  return file->write(id, parent, buffer, len, nbw);
}

/*!
 *Get an header for the filter. Returns -1 on errors.
 */
int DynamicFilter::getHeader(char* buffer, 
                             u_long len, u_long* nbw)
{
  *nbw=0; 
  if(!file)
    return 0;
  
  return file->getHeader(id, parent, buffer, len, nbw);
}

/*!
 *Get a footer for the filter. Returns -1 on errors.
 */
int DynamicFilter::getFooter(char* buffer, u_long len, u_long* nbw)
{
  *nbw=0; 
  if(!file)
    return 0;
  
  return file->getFooter(id, parent, buffer, len, nbw);
}

/*!
 *Construct the DynamicFilter object.
 */
DynamicFilter::DynamicFilter(DynamicFilterFile* f)
{
  parent = 0;
  file = f;
  id = 0;
}

/*!
 *Destroy the DynamicFilter object.
 */
DynamicFilter::~DynamicFilter()
{

}

/*!
 *Set a numeric ID for the filter object.
 */
void DynamicFilter::setId(u_long i)
{
  id = i;
}

/*!
 *Get the numeric ID for the filter.
 */
u_long DynamicFilter::getId()
{
  return id;
}

/*!
 *Set the stream where apply the filter.
 */
void DynamicFilter::setParent(Stream* p)
{
  parent = p;
}


/*!
 *Construct the object passing a stream and a numeric ID.
 */
DynamicFilter::DynamicFilter(DynamicFilterFile* f,Stream* s, u_long i)
{
  parent = s;
  id=i;
  file=f;
}

/*!
 *Flush everything to the stream. Returns -1 on errors.
 */
int DynamicFilter::flush(u_long *nbw)
{
  *nbw = 0;
  if(!file)
    return 0;

  return file->flush(id, parent, nbw);
}

/*!
 *Get the stream used by the filter.
 */
Stream* DynamicFilter::getParent()
{
  return parent;
}

/*!
 *Returns a nonzero value if the filter modify the input/output data.
 */
int DynamicFilter::modifyData()
{
  if(!file)
    return 0;
  return file->modifyData(id, parent);
}

/*!
 *Return a string with the filter name. 
 *If an external buffer is provided write the name there too.
 */
const char* DynamicFilter::getName(char* name, u_long len)
{
  if(!file)
    return 0;
  return file->getName(name, len);
}
