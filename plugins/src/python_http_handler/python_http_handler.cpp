/*
MyServer
Copyright (C) 2007, 2008, 2009 The Free Software Foundation Inc.
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
#include <myserver.h>
#include <string.h>

#include <Python.h>

#include <include/server/server.h>
#include <include/protocol/http/http_data_handler.h>
#include <include/base/multicast/multicast.h>
#include <include/protocol/http/http.h>
#include <include/plugin/plugin.h>
#include <include/conf/main/xml_main_configuration.h>
#include <include/plugin/plugin.h>


PLUGIN_NAME ("python_http_handler");

static Server* serverInstance;

typedef int (*executePROC)(char*, u_long);
typedef int (*executeFromFilePROC)(char*);

class ThreadData : public HttpDataHandler
{
  HttpThreadContext* td;
  int ret;
  bool keepalive;
  bool useChunks;
  FiltersChain chain;
public:

  ThreadData (HttpThreadContext* td)
  {
    this->td = td;
    ret = 0;
  }

  HttpThreadContext* getHttpThreadContext (){return td;}
  int getRet (){return ret;}

  void setHttpThreadContext (HttpThreadContext* td){this->td = td;}
  void setRet (int r){ret = r;}


  virtual int send (HttpThreadContext*, ConnectionPtr s,
                    const char* exec, const char* cmdLine,
                    int execute, int onlyHeader)
  {
    return 0;
  }

  int send (char* buffer, u_long size)
  {
    return appendDataToHTTPChannel (td, buffer, size, 0, &chain,
                                    0, useChunks);
  }

  void check ()
  {
    checkDataChunks (td, &keepalive, &useChunks);
    chain.setProtocol (td->http);
    chain.setProtocolData (td);
    chain.setStream (td->connection->socket);
  }
};

static HashMap<ThreadID, ThreadData*> pythonThreadData;
static Mutex mutex;

typedef int (*INIT_MODULE)(char* name, PyMethodDef methods[]);
static INIT_MODULE init;


static ThreadData* getThreadData ()
{
  ThreadID tid = Thread::threadID();
  ThreadData* ret;
  mutex.lock ();
  ret = pythonThreadData.get (tid);
  mutex.unlock ();
  return ret;
}

static HttpThreadContext* getThreadContext ()
{
  return getThreadData ()->getHttpThreadContext ();
}

static PyObject *get_remote_addr (PyObject *self, PyObject *args)
{
  HttpThreadContext* context = getThreadContext ();
  return Py_BuildValue((char*)"s", context->connection->getIpAddr ());
}

static PyObject *get_remote_port (PyObject *self, PyObject *args)
{
  HttpThreadContext* context = getThreadContext ();
  return Py_BuildValue((char*)"i", context->connection->getPort ());
}

static PyObject *get_local_addr (PyObject *self, PyObject *args)
{
  HttpThreadContext* context = getThreadContext ();
  return Py_BuildValue ((char*)"s", context->connection->getLocalIpAddr ());
}

static PyObject *get_local_port (PyObject *self, PyObject *args)
{
  HttpThreadContext* context = getThreadContext ();
  return Py_BuildValue ((char*)"i", context->connection->getLocalPort ());
}


static PyObject *get_response_header (PyObject *self, PyObject *args)
{
  char *header;
  string value;
  if (!PyArg_ParseTuple (args, (char*)"s", &header))
    return NULL;

  HttpThreadContext* context = getThreadContext ();

  context->response.getValue (header, &value);

  return Py_BuildValue ((char*)"s", value.c_str());
}

static PyObject *get_request_header (PyObject *self, PyObject *args)
{
  char *header;
  string value;
  if (!PyArg_ParseTuple(args, (char*)"s", &header))
    return NULL;

  HttpThreadContext* context = getThreadContext ();

  context->request.getValue (header, &value);

  return Py_BuildValue ((char*)"s", value.c_str ());
}

static PyObject *log_server_error (PyObject *self, PyObject *args)
{
  char *msg;

  if (!PyArg_ParseTuple(args, (char*)"s", &msg))
    return NULL;

  serverInstance->log (MYSERVER_LOG_MSG_ERROR, "%s", msg);
  return NULL;
}

static PyObject *set_response_header (PyObject *self, PyObject *args)
{
  char *header;
  char *value;
  const char* ret;
  if (!PyArg_ParseTuple(args, (char*)"ss", &header, &value))
    return NULL;

  HttpThreadContext* context = getThreadContext ();

  string *retS =  context->response.setValue (header, value);

  if (retS)
    {
      ret = retS->c_str ();
      return Py_BuildValue ((char*)"s", ret);
    }
  return Py_BuildValue ((char*)"s", "");
}

static PyObject *set_request_header (PyObject *self, PyObject *args)
{
  char *header;
  char *value;
  const char* ret;
  if (!PyArg_ParseTuple(args, (char*)"ss", &header, &value))
    return NULL;

  HttpThreadContext* context = getThreadContext ();

  string *retS =  context->request.setValue (header, value);
  if (retS)
    {
      ret = retS->c_str ();
      return Py_BuildValue ((char*)"s", ret);
    }
  return Py_BuildValue ((char*)"s", "");
}

static PyObject *send_header (PyObject *self, PyObject *args)
{
  ThreadData *tdata = getThreadData ();
  HttpThreadContext* td = tdata->getHttpThreadContext ();

  tdata->check ();

  HttpHeaders::buildHTTPResponseHeader (td->buffer->getBuffer(),
                                        &(td->response));

  if(td->connection->socket->send (td->buffer->getBuffer(),
                                   (u_long)strlen(td->buffer->getBuffer()), 0)
     < 0)
    return Py_BuildValue((char*)"i", 1);

  tdata->setRet (1);

  return Py_BuildValue ((char*)"i", 0);
}

static PyObject *get_document_root (PyObject *self, PyObject *args)
{
  char *data;
  if (!PyArg_ParseTuple(args, (char*)"s", &data))
    return NULL;

  HttpThreadContext* context = getThreadContext ();

  return Py_BuildValue ((char*)"s", context->getVhostDir ());
}

static PyObject *send_data (PyObject *self, PyObject *args)
{
  char *data;
  u_long size = 0;
  if (!PyArg_ParseTuple(args, (char*)"s", &data))
    return NULL;

  size = strlen (data);

  ThreadData *tdata = getThreadData ();

  if (tdata->send(data, size))
    return Py_BuildValue ((char*)"i", 0);

  tdata->setRet (1);

  return Py_BuildValue((char*)"i", size);
}

static PyObject *end_send_data (PyObject *, PyObject *)
{
  ThreadData *tdata = getThreadData ();

  tdata->send (0, 0);

  return Py_BuildValue ((char*)"i", 0);
}

static PyObject *raise_error (PyObject *self, PyObject *args)
{
  int error;
  if (!PyArg_ParseTuple(args, (char*)"i", &error))
    return NULL;

  ThreadData* data = getThreadData ();

  if(data->getRet())
    return NULL;

  data->getHttpThreadContext ()->http->raiseHTTPError (error);

  data->setRet (1);

  return Py_BuildValue ((char*)"s", "");
}


static PyObject *is_ssl (PyObject *self, PyObject *args)
{
  HttpThreadContext* context = getThreadContext ();
  int isSsl = context->http->getProtocolOptions () & Protocol::SSL;

  return Py_BuildValue ((char*)"b", isSsl);
}


static PyObject *send_redirect (PyObject *self, PyObject *args)
{
  char* dest;
  if (!PyArg_ParseTuple(args, (char*)"s", &dest))
    return NULL;

  ThreadData* data = getThreadData ();

  if (data->getRet())
    return NULL;

  data->getHttpThreadContext ()->http->sendHTTPRedirect (dest);

  data->setRet (1);

  return Py_BuildValue ((char*)"s", dest);
}

static PyMethodDef PythonHttpHandlerMethods[] = {
  {(char*)"get_remote_port", get_remote_port, METH_VARARGS, (char*)"Get the remote TCP port"},
  {(char*)"get_local_port", get_local_port, METH_VARARGS, (char*)"Get the local TCP port"},
  {(char*)"get_remote_addr", get_remote_addr, METH_VARARGS, (char*)"Get the remote IP address"},
  {(char*)"get_local_addr", get_local_addr, METH_VARARGS, (char*)"Get the local IP address"},
  {(char*)"get_request_header", get_request_header, METH_VARARGS, (char*)"Get HTTP request header field value"},
  {(char*)"set_request_header", set_request_header, METH_VARARGS, (char*)"Set HTTP request header field value"},
  {(char*)"get_response_header", get_response_header, METH_VARARGS, (char*)"Get HTTP response header field value"},
  {(char*)"set_response_header", set_response_header, METH_VARARGS, (char*)"Set HTTP response header field value"},
  {(char*)"send_redirect", send_redirect, METH_VARARGS, (char*)"Send a redirect to another location"},
  {(char*)"raise_error", raise_error, METH_VARARGS, (char*)"Raise HTTP error page"},
  {(char*)"send_data", send_data, METH_VARARGS, (char*)"Send data to the client"},
  {(char*)"send_header", send_header, METH_VARARGS, (char*)"Send the HTTP header to the client"},
  {(char*)"end_send_data", end_send_data, METH_VARARGS, (char*)"Complete a data transfer"},
  {(char*)"get_document_root", get_document_root, METH_VARARGS, (char*)"Return the document root directory"},
  {(char*)"is_ssl", is_ssl, METH_VARARGS, (char*)"Check if the connection is using a secure channel"},
  {(char*)"log_server_error", log_server_error, METH_VARARGS, (char*)"Log a server message"},
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

  virtual int updateMulticast (MulticastRegistry<string, void*, int>* reg,
                               string& msg, void* arg)
  {
    HttpThreadContext *td = (HttpThreadContext*)arg;
    ThreadID tid = Thread::threadID ();
    list<Item>::iterator it;
    mutex.lock ();
    ThreadData threadData (td);
    pythonThreadData.put (tid, &threadData);
    mutex.unlock ();

    init ((char*)"python_http_handler_internal", PythonHttpHandlerMethods);

    for (it = rules.begin (); it != rules.end (); it++)
      {
        if ((*it).file)
          {
            char *method = (char*)"executeFromFile";
            executeFromFilePROC execute =
              ((executeFromFilePROC)python->getDirectMethod(method);
               if (execute)
                 execute((char*)(*it).data.c_str ());
               }
            else
              {
                executePROC execute =
                ((executePROC)python->getDirectMethod((char*)"execute"));
                if (execute)
                  execute ((char*)(*it).data.c_str(), (*it).data.length());
              }
          }
        return threadData.getRet ();
      }

    void addRule (const char* rule, bool file)
    {
      Item it;
      it.data.assign (rule);
      it.file = file;
      rules.push_back (it);
    }

    void setPythonExecutor (Plugin* python){this->python = python;}
  private:
    list<Item> rules;
    Plugin* python;
  };

  static HttpObserver observer;

  EXPORTABLE(int) load (void* server)
  {
    serverInstance = (Server*)server;
    string msg("new-http-request");
    string pythonName("python");
    Plugin* python;
    MainConfiguration* configuration;
    xmlDocPtr xmlDoc;
    python = serverInstance->getPluginsManager ()->getPlugin (pythonName);

    if(!python)
      {
        serverInstance->log (MYSERVER_LOG_MSG_ERROR,
                             _("PythonHttpHandler: Cannot find python"));
        return -1;
      }
    observer.setPythonExecutor(python);

    string httpStr ("http");
    Protocol *p = serverInstance->getProtocolsManager ()->getProtocol (httpStr);
    static_cast<HttpProtocol*>(p)->addMulticast(msg, &observer);

    init = (INIT_MODULE) python->getDirectMethod((char*)"initModule");

    if(!init)
      {
        serverInstance->log (MYSERVER_LOG_MSG_ERROR,
                             _("PythonHttpHandler: Cannot find method initModule in python"));
        return -1;
      }

    configuration = serverInstance->getConfiguration ();

    /* FIXME: DON'T DO THIS.  */
    xmlDoc = ((XmlMainConfiguration*)configuration)->getDoc ();

    for (xmlNodePtr ptr = xmlDoc->children->next->children; ptr; ptr = ptr->next)
      {
        if (!xmlStrcmp(ptr->name, (const xmlChar *)"PYTHON_HTTP_HANDLER"))
          {
            bool file = false;
            xmlAttrPtr properties = ptr->properties;
            char* data = 0;
            while (properties)
              {
                if (!xmlStrcmp(properties->name, (const xmlChar *)"file"))
                  {
                    if(properties->children && properties->children->content)
                      data = (char*)properties->children->content;

                    file = true;
                  }
                properties = properties->next;
              }

            if (!file && ptr->children && ptr->children->next
                && ptr->children->next->content)
              data = (char*)ptr->children->next->content;

            if (!data)
              {
                serverInstance->log (MYSERVER_LOG_MSG_ERROR,
                                     _("PythonHttpHandler: Invalid rule"));
                return -1;
              }

            observer.addRule (data, file);
          }

      }

    mutex.init();

    return 0;
  }

  EXPORTABLE(int) postLoad (void* server)
  {
    return 0;
  }

  EXPORTABLE(int) unLoad ()
  {
    mutex.destroy ();
    return 0;
  }
