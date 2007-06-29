/*
MyServer
Copyright (C) 2007 The MyServer Team
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

#include <Python.h>

#include <../stdafx.h>
#include <include/connection.h>
#include <include/sockets.h>
#include <include/server.h>
#include <include/mutex.h>
#include <include/file.h>
#include <include/files_utility.h>


struct PythonData
{
	PyThreadState* interp;
	PyInterpreterState* getInterpreter();
	PythonData();
	void clear();
};

extern HashMap<ThreadID, PythonData*> pythonThreadData;



#ifdef WIN32
#define EXPORTABLE(x) x _declspec(dllexport)
#else
#define EXPORTABLE(x) extern "C" x
#endif


EXPORTABLE(char*) name(char* name, u_long len);

EXPORTABLE(int) load(void* server,void* parser);
EXPORTABLE(int) unLoad(void* p);

EXPORTABLE(int) execute(char* code, u_long length);
EXPORTABLE(int) executeFromFile(char* filename);


EXPORTABLE(int) initModule(char*, PyMethodDef[]);
