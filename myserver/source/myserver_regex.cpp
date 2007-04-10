/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007 The MyServer Team
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

#include "../include/myserver_regex.h"

/*!
 *Compile the regex pattern.
 */
int Regex::compile(const char *p, int f)
{
#ifdef REGEX
  int ret = regcomp(&compiledRegex, p, f);
  pattern.assign(p);
  flags = f;
  if(!ret)
    compiled = 1;
  return ret;
#endif
  return -1;
}

/*!
 *Match the pattern against strings.
 */
int Regex::exec(const char *text, size_t nmatch, regmatch_t matchptr [], 
                int eflags)
{
#ifdef REGEX
  if(!compiled)
    return 1;
  int ret = regexec (&compiledRegex, text, nmatch, matchptr, eflags);
  return ret;
#endif
  return -1;
}

/*!
 *Free the used memory.
 */
void Regex::free()
{
#ifdef REGEX
  if(compiled)
    regfree(&compiledRegex);
  compiled = 0;
#endif
}

/*!
 *Constructor for the class.
 */
Regex::Regex()
{
  compiled = 0;
}

/*!
 *Destructor for the class
 */
Regex::~Regex()
{
#ifdef REGEX
  free();
#endif
}

/*!
 *Constructor for the class.
 */
Regex::Regex(const char *pattern, int flags)
{
#ifdef REGEX
  compile(pattern, flags);
#endif
}
/*!
 *Return a nonzero value if the regex was compiled.
 */
int Regex::isCompiled()
{
  return compiled;
}

/*!
 *Construct by copy.
 */
Regex::Regex(Regex& r)
{
  clone(r);
}

/*!
 *Create a clone.
 */
void Regex::clone(Regex& r)
{
#ifdef REGEX
  compile(r.pattern.c_str(), r.flags);
#endif
}
