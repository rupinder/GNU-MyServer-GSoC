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
#define YY_NO_UNPUT

using namespace std;

#include <iostream>
#include <stdio.h>
#include <string>
#include <list>

string* call_function(void* context, string* func);
string* call_function(void* context, string* func, string* arg);

string* allocate_new_str();
string* allocate_new_str(const char*);

struct HttpThreadContext;

typedef struct
{
	HttpThreadContext* td;
	list<string*> strings;
} ThreadContext;


void free_strings(ThreadContext* context);
int scan_string(ThreadContext* context, const char* str, int* val);
