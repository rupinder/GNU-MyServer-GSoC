/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007, 2008, 2009 Free Software Foundation, Inc.
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

#ifndef MYSERVER_REGEX_H

# define MYSERVER_REGEX_H

# include "stdafx.h"

extern "C" 
{
# include <stdio.h>

# ifdef TRE
#  include <tre/regex.h>
# elif REGEX
#  include <regex.h>
# else
  typedef void* regmatch_t;
  typedef void* regex_t;
# endif
}

#include <string>
using namespace std;

/*!
 *This class is used to manage regular expressions in MyServer.
 */
class Regex
{
public:
  Regex(){compiled = 0;}
  void clone(Regex&);
  Regex(Regex&);
  Regex(const char *pattern, int flags);
  ~Regex();
  int isCompiled();
  int compile(const char *pattern, int flags);
  int exec(const char *string, size_t nmatch, regmatch_t matchptr [], 
					 int eflags);
  void free();

  Regex(string const &pattern, int flags){Regex(pattern.c_str(), flags);}
  int compile(string const &str, int flags){
		return compile(str.c_str(), flags );}
  int exec(string const &str, size_t nmatch, regmatch_t matchptr [], 
					 int eflags)
    {return exec(str.c_str(), nmatch, matchptr, eflags);}
private:
  regex_t compiledRegex;
  regmatch_t match;
  int compiled;
  string pattern;
  int flags;
};

#endif
