/*
MyServer
Copyright (C) 2007 The MyServer Team
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
#include <stdafx.h>
#include <string.h>
#include <include/server.h>
#include <include/multicast.h>
#include <include/http.h>
#include <include/dynamic_executor.h>
#include <Python.h>

#ifdef WIN32
#define EXPORTABLE(x) x _declspec(dllexport)
#else
#define EXPORTABLE(x) extern "C" x
#endif

struct ThreadData
{
	HttpThreadContext* td;
	int ret;
};

static HashMap<ThreadID, ThreadData*> pythonThreadData;
static Mutex mutex;

typedef int (*INIT_MODULE)(char* name, PyMethodDef methods[]);
static INIT_MODULE init;


static ThreadData* getThreadData()
{
	ThreadID tid = Thread::threadID();
	ThreadData* ret;
	mutex.lock();
	ret = pythonThreadData.get(tid);
	mutex.unlock();
	return ret;
}

static HttpThreadContext* getThreadContext()
{
	return getThreadData()->td;
}

static PyObject *get_header(PyObject *self, PyObject *args)
{
	char *header;
	string value;
	if (!PyArg_ParseTuple(args, (char*)"s", &header))
		return NULL;
	
	HttpThreadContext* context = getThreadContext();
	
	context->request.getValue(header, &value);
	
	return Py_BuildValue((char*)"s", value.c_str());
}

static PyObject *raise_error(PyObject *self, PyObject *args)
{
	int error;
	if (!PyArg_ParseTuple(args, (char*)"i", &error))
		return NULL;

	ThreadData* data = getThreadData();

	if(data->ret)
		return NULL;
	
	data->td->http->raiseHTTPError(error);
	
	data->ret = 1;
	
	return Py_BuildValue((char*)"s", "");
}

static PyMethodDef httpCheckerMethods[] = {
	{(char*)"get_header", get_header, METH_VARARGS, (char*)"Get HTTP header field value"},
	{(char*)"raise_error", raise_error, METH_VARARGS, (char*)"Raise HTTP error page"},
	{NULL, NULL, 0, NULL}
};


class HttpObserver : public Multicast<string, void*, int>
{
public:
	
	virtual int updateMulticast(MulticastRegistry<string, void*, int>* reg, string& msg, void* arg)
	{
		HttpThreadContext *td = (HttpThreadContext*)arg;
		ThreadID tid = Thread::threadID();
		list<string>::iterator it;
		mutex.lock();
		ThreadData threadData = {td, 0};
		pythonThreadData.put(tid, &threadData);
		mutex.unlock();

		init((char*)"http_checker", httpCheckerMethods);

		for(it = rules.begin(); it != rules.end(); it++)
			python->execute((char*)(*it).c_str(), (*it).length());

		return threadData.ret;
	}

	void addRule(const char* rule)
	{
		string strRule(rule);
		rules.push_back(strRule);
	}

	void setPythonExecutor(DynamicExecutor* python){this->python = python;}

private:
	list<string> rules;
	DynamicExecutor* python;
};

static HttpObserver observer;

EXPORTABLE(char*) name(char* name, u_long len)
{
	char* str = (char*)"http_checker";
	if(name)
		strncpy(name, str, len);
	return str;
}

EXPORTABLE(int) load(void* server,void* parser)
{
	Server* serverInstance = (Server*)server;
	HttpStaticData* staticData =(HttpStaticData*) serverInstance->getGlobalData("http-static");
	string msg("new-http-request");
	string pythonNamespace("executors");
	string pythonName("python");
	Plugin* python;
	XmlParser* configuration;
	xmlDocPtr xmlDoc;
	if(!staticData)
	{
		serverInstance->logLockAccess();
		serverInstance->logPreparePrintError();
		serverInstance->logWriteln("HttpChecker: Invalid HTTP static data");
		serverInstance->logEndPrintError();
		serverInstance->logUnlockAccess();
		return -1;
	}
	python = serverInstance->getPluginsManager()->getPlugin(pythonNamespace, pythonName);

	if(!python)
	{
		serverInstance->logLockAccess();
		serverInstance->logPreparePrintError();
		serverInstance->logWriteln("HttpChecker: Cannot find executors::python");
		serverInstance->logEndPrintError();
		serverInstance->logUnlockAccess();
		return -1;
	}
	observer.setPythonExecutor((DynamicExecutor*)python);

	staticData->addMulticast(msg, &observer);

	init = (INIT_MODULE) python->getDirectMethod((char*)"initModule");

	if(!init)
	{
		serverInstance->logLockAccess();
		serverInstance->logPreparePrintError();
		serverInstance->logWriteln("HttpChecker: Cannot find method initModule in executors::python");
		serverInstance->logEndPrintError();
		serverInstance->logUnlockAccess();
		return -1;
	}
	configuration = serverInstance->getConfiguration();
	xmlDoc = configuration->getDoc();

	for(xmlNodePtr ptr = xmlDoc->children->next->children; ptr; ptr = ptr->next)
	{
		if(!xmlStrcmp(ptr->name, (const xmlChar *)"HTTP_CHECKER_RULE"))
		{
			observer.addRule((char*)ptr->children->next->content);
		}
		
	}

	mutex.init();

	return 0;
}
EXPORTABLE(int) postLoad(void* server,void* parser)
{
	return 0;
}
EXPORTABLE(int) unLoad(void* parser)
{
	mutex.destroy();
	return 0;
}
