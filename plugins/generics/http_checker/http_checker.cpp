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

static PyObject *get_remote_addr(PyObject *self, PyObject *args)
{
	HttpThreadContext* context = getThreadContext();
		return Py_BuildValue((char*)"s", context->connection->getIpAddr());
}

static PyObject *get_remote_port(PyObject *self, PyObject *args)
{
	HttpThreadContext* context = getThreadContext();
		return Py_BuildValue((char*)"i", context->connection->getPort());
}

static PyObject *get_local_addr(PyObject *self, PyObject *args)
{
	HttpThreadContext* context = getThreadContext();
		return Py_BuildValue((char*)"s", context->connection->getLocalIpAddr());
}

static PyObject *get_local_port(PyObject *self, PyObject *args)
{
	HttpThreadContext* context = getThreadContext();
		return Py_BuildValue((char*)"i", context->connection->getLocalPort());
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

static PyObject *set_header(PyObject *self, PyObject *args)
{
	char *header;
	char *value;
	const char* ret;
	if (!PyArg_ParseTuple(args, (char*)"ss", &header, &value))
		return NULL;
	
	HttpThreadContext* context = getThreadContext();
	
	ret = context->request.setValue(header, value)->c_str();
	
	return Py_BuildValue((char*)"s", ret);
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

static PyObject *send_redirect(PyObject *self, PyObject *args)
{
	char* dest;
	if (!PyArg_ParseTuple(args, (char*)"s", &dest))
		return NULL;

	ThreadData* data = getThreadData();

	if(data->ret)
		return NULL;
	
	data->td->http->sendHTTPRedirect(dest);
	
	data->ret = 1;
	
	return Py_BuildValue((char*)"s", dest);
}

static PyMethodDef httpCheckerMethods[] = {
	{(char*)"get_remote_port", get_remote_port, METH_VARARGS, (char*)"Get the remote TCP port"},
	{(char*)"get_local_port", get_local_port, METH_VARARGS, (char*)"Get the local TCP port"},
	{(char*)"get_remote_addr", get_remote_addr, METH_VARARGS, (char*)"Get the remote IP address"},
	{(char*)"get_local_addr", get_local_addr, METH_VARARGS, (char*)"Get the local IP address"},
	{(char*)"get_header", get_header, METH_VARARGS, (char*)"Get HTTP header field value"},
	{(char*)"set_header", set_header, METH_VARARGS, (char*)"Set HTTP header field value"},
	{(char*)"send_redirect", send_redirect, METH_VARARGS, (char*)"Send a redirect to another location"},
	{(char*)"raise_error", raise_error, METH_VARARGS, (char*)"Raise HTTP error page"},
	{NULL, NULL, 0, NULL}
};


class HttpObserver : public Multicast<string, void*, int>
{
	struct Item
	{
		string data;
		bool file;
	};
public:
	
	virtual int updateMulticast(MulticastRegistry<string, void*, int>* reg, string& msg, void* arg)
	{
		HttpThreadContext *td = (HttpThreadContext*)arg;
		ThreadID tid = Thread::threadID();
		list<Item>::iterator it;
		mutex.lock();
		ThreadData threadData = {td, 0};
		pythonThreadData.put(tid, &threadData);
		mutex.unlock();

		init((char*)"http_checker", httpCheckerMethods);

		for(it = rules.begin(); it != rules.end(); it++)
		{
			if((*it).file)
				python->executeFromFile((char*)(*it).data.c_str());
			else
				python->execute((char*)(*it).data.c_str(), (*it).data.length());
		}
		return threadData.ret;
	}

	void addRule(const char* rule, bool file)
	{
		Item it;
		it.data.assign(rule);
		it.file = file;
		rules.push_back(it);
	}

	void setPythonExecutor(DynamicExecutor* python){this->python = python;}

private:
	list<Item> rules;
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
			bool file = false;
			xmlAttrPtr properties = ptr->properties;
			char* data = 0;
			while(properties)
			{
				if(!xmlStrcmp(properties->name, (const xmlChar *)"file"))
				{
					if(properties->children && properties->children->content)
						data = (char*)properties->children->content;

					file = true;
				}
				properties = properties->next;
			}

			if(!file && ptr->children && ptr->children->next && ptr->children->next->content)
				data = (char*)ptr->children->next->content;

			if(!data)
			{
				serverInstance->logLockAccess();
				serverInstance->logPreparePrintError();
				serverInstance->logWriteln("HttpChecker: Invalid rule");
				serverInstance->logEndPrintError();
				serverInstance->logUnlockAccess();
				return -1;
			}

			observer.addRule(data, file);
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
