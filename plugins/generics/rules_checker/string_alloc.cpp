/*
MyServer
Copyright (C) 2007 The Free Software Foundation Inc.
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
#include "heading.h"

#include <list>
#include <include/base/thread/thread.h>

static list<string*> strings;

string* allocate_new_str()
{
	string* ret = new string();
	strings.push_back(ret);
	return ret;
}

string* allocate_new_str(const char* v)
{
	string* ret = allocate_new_str();
	ret->assign(v);
	return ret;
}

void free_strings(ThreadContext* context)
{

	list<string*>::iterator it = context->strings.begin();
	while(it != context->strings.end())
	{
		delete *it;
		it++;
	}
	context->strings.clear();
}
