/*
  MyServer
   strlcpy and strlcat by codingmaster
  Copyright (C) 2002, 2003, 2004, 2007, 2008, 2009, 2010 Free Software
  Foundation, Inc.
  Copyright (C) 2004 by codingmaster
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

#include "myserver.h"

#include <include/base/string/securestr.h>
#include <string.h>

#define FAST_SECURE_STR

/*!
  Secure string concatenate routine.
 */
unsigned int myserver_strlcat (char *dst, const char *src, unsigned int size)
{
#ifdef FAST_SECURE_STR
  u_long dstLen = strlen (dst);
  u_long srcLen = strlen (src);
  u_long tc;

  size -= dstLen;

  tc = (srcLen < size ? srcLen : size);

  if (tc)
    {
      if (size <= srcLen)
        tc--;
      memcpy (dst + dstLen, src, tc);
      dst[tc] = '\0';
    }

  return dstLen + srcLen;
#else
  char *dstptr = dst;
  size_t dstlen,tocopy = size;
  const char *srcptr = src;

  while (tocopy-- && *dstptr)
    dstptr++;

  dstlen = dstptr - dst;

  tocopy = size - dstlen;
  if (!tocopy)
    return ((int)(dstlen + strlen (src)));

  while (*srcptr)
    {
      if (tocopy!=1)
        {
          *dstptr++ = *srcptr;
          tocopy--;
        }
      srcptr++;
    }

  *dstptr = 0;

  return ((int)(dstlen + (srcptr - src)));
#endif
}

/*!
  Secure string copy routine.
 */
unsigned int myserver_strlcpy (register char *dst, register const char *src,
                               unsigned int size)
{
#ifdef FAST_SECURE_STR
  u_long ret = strlen (src);
  u_long tc = (ret < size ? ret : size);

  if (tc)
    {
      if (size <= ret)
        tc--;

      memcpy (dst, src, tc);
      dst[tc] = '\0';
    }
  return ret;
#else
  char *dstptr = dst;
  size_t tocopy = size;
  const char *srcptr = src;

  if (tocopy && --tocopy)
    {
      do
        {
          if (!(*dstptr++ = *srcptr++))
            break;
        }

      while (--tocopy);
    }

  if (!tocopy)
    {
      if (size)
        *dstptr = 0;

      while (*srcptr++);
    }

  return ((int)(srcptr - src - 1));
#endif
}
