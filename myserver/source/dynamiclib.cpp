/*
MyServer
Copyright (C) 2005 The MyServer Team
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

#include "../include/dynamiclib.h"

/*!
 *Initialize class internal data.
 */
DynamicLibrary::DynamicLibrary()
{
  handle = 0;
}

/*!
 *Destroy the object.
 */
DynamicLibrary::~DynamicLibrary()
{
  close();
}

/*!
 *Load the specified dynamic library. It returns 0 on success.
 *\param filename Name of the file to load.
 *\param globally Set if the library is loaded globally.
 */
int DynamicLibrary::loadLibrary(const char* filename, int globally)
{
#ifdef WIN32
  handle = LoadLibrary(filename);
#endif
#ifdef HAVE_DL
  if(globally)
    handle = dlopen(filename, RTLD_NOW | RTLD_GLOBAL);
  else
    handle = dlopen(filename, RTLD_LAZY);
#endif
  return handle ? 0 : 1;
}

/*!
 *Get a pointer to the specified function. Returns 0 on errors or 
 *the function address. 
 *\param fnName Function name to find.
 */
void* DynamicLibrary::getProc(const char* fnName)
{
  if(!handle)
    return 0;
#ifdef WIN32
		return (void*) GetProcAddress((HMODULE)handle, fnName); 
#endif
#ifdef HAVE_DL
		return (void*) dlsym(handle, fnName);
#endif
}

/*!
 *Close the library. Returns 0 on success.
 */
int DynamicLibrary::close()
{
  int ret = 1;
  if(!handle)
    return 1;
#ifdef WIN32
		ret = FreeLibrary((HMODULE)handle) ? 0 : 1; 
#endif
#ifdef HAVE_DL
		ret = dlclose(handle);
#endif
    handle = 0;
    return ret;
}

/*!
 *Return if the object has a valid handle.
 */
int DynamicLibrary::validHandle()
{
  return handle ? 1 : 0;
}
