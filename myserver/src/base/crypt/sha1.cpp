/*
 MyServer
 Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010 Free Software
 Foundation, Inc.
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
#include <include/base/crypt/sha1.h>

/*!
 * Start SHA1 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 */
void Sha1::init ()
{
  sha1_init_ctx (&ctx);
}

/*!
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
void Sha1::update (char const *buf, u_long len)
{
  sha1_process_bytes (buf, len, &ctx);
}

/*!
 * Initialize the object via a constructor.
 */
Sha1::Sha1 ()
{
  init ();
}

/*!
 * Destroy the object.
 */
Sha1::~Sha1 ()
{
}

/*!
 * Write the final hash to the buffer.
 */
char* Sha1::end (char *buf)
{
  unsigned char tmp[22];
  char *ret = buf;
  if (!buf)
    return NULL;

  sha1_finish_ctx (&ctx, tmp);

  static const char hex[] = "0123456789abcdef";

  for (long i = 0; i < 22; i++)
    {
      *buf++ = hex[tmp[i] >> 4];
      *buf++ = hex[tmp[i] & 0x0f];
    }

  *buf = '\0';

  return ret;
}

CryptAlgo *Sha1::sha1Builder ()
{
  return new Sha1 ();
}

/*!
 * Register the algorithm on the specified manager.
 *
 * \param manager The manager where register SHA1.
 */
void Sha1::initialize (CryptAlgoManager *manager)
{
  string name ("sha1");
  manager->registerAlgorithm (name, Sha1::sha1Builder);
}
