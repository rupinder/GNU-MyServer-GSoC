/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef MYSERVER_REGEX_IN
#define MYSERVER_REGEX_IN

#include "../stdafx.h"
#ifndef _VC
extern "C" 
{
#endif

#include <stdio.h>
#include "../contrib/rx/rxposix.h"

#ifndef _VC
}
#endif

#include <string>
using namespace std;

/*!
*This class is used to manage regular expressions in MyServer.
*/
class Regex
{
  regex_t compiled_regex;
  regmatch_t match;
  int compiled;
public:
  Regex();
  Regex(const char *pattern, int flags);
  ~Regex();
  int isCompiled();
  int compile(const char *pattern, int flags);
  int exec(const char *string, size_t nmatch, regmatch_t matchptr [], int eflags);
  void free();

  Regex(string const &pattern, int flags){Regex(pattern.c_str(), flags);}
  int compile(string const &str, int flags){return compile(str.c_str(), flags );}
  int exec(string const &str, size_t nmatch, regmatch_t matchptr [], int eflags)
    {return exec(str.c_str(), nmatch, matchptr, eflags);}
};
#endif
