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

 
#include "../include/isapi.h"
#include "../include/stream.h"
#include "../include/filter.h"

#include <string>
#include <sstream>

using namespace std;

/*!
 *Read [len] characters using the filter. Returns -1 on errors.
 */
int Filter::read(char* buffer, int len)
{
  return -1;
}

/*!
 *Write [len] characters to the stream. Returns -1 on errors.
 */
int Filter::write(char* buffer, int len)
{
  return -1;
}

Filter::Filter()
{
  parent = 0;
}

Filter::~Filter()
{

}

/*!
 *Set the stream where apply the filter. The stream
 */
void Filter::setParent(Stream* p)
{
  parent = p;
}

/*!
 *Flush everything to the stream. Returns -1 on errors.
 */
int Filter::flush()
{
  return -1;
}

/*!
 *Get the stream used by the filter.
 */
Stream* Filter::getParent()
{
  return parent;
}
