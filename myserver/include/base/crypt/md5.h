/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2005-2009 Free Software Foundation, Inc.
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
#ifndef MD5_H
# define MD5_H

# include "stdafx.h"

# include <md5.h>
# include <include/base/crypt/crypt_algo.h>
# include <include/base/crypt/crypt_algo_manager.h>

class Md5 : public CryptAlgo
{
public:
  Md5 ();
  virtual ~Md5 ();
  virtual void init ();
  virtual void update (char const *buf, unsigned long len);
  virtual char* end (char *buf);
  static void initialize (CryptAlgoManager *manager);
  static CryptAlgo *md5Builder ();

private:
  struct md5_ctx ctx;
};

#endif
