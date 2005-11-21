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


#include "../include/dyn_http_command.h"
#include "../include/cXMLParser.h"
#include "../include/cserver.h"
#include "../include/lfind.h"

#include <string>

#ifdef NOT_WIN
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

// Bloodshed Dev-C++ Helper
#ifndef intptr_t
#define intptr_t int
#endif

typedef int (*loadMethodPROC)(void*, void*); 
typedef int (*acceptDataPROC)();
typedef int (*unloadMethodPROC)(void* languageParser); 
typedef int (*controlMethodPROC)(void*, volatile void*, const char*, int, int, int); 
typedef char* (*registerNamePROC)(char*, int); 

/*!
 *Get the command name.
 */
char *DynamicHttpCommand::getCommandName(char* out, int len)
{
  registerNamePROC name = (registerNamePROC)hinstLib.getProc("registerName");
  if(name)
    return name(out, len);
  return 0;
}

/*!
 *Default constructor.
 */
DynamicHttpCommand::DynamicHttpCommand()
{

}

/*!
 *Destroy the object.
 */
DynamicHttpCommand::~DynamicHttpCommand()
{
  hinstLib.close();
}

/*!
 *Load the plugin. Returns 0 on success.
 */
int DynamicHttpCommand::loadCommand(const char* name, XmlParser* parser, 
                                    Server* server)
{
  if(hinstLib.loadLibrary(name))
    return 1;
  loadMethodPROC load =(loadMethodPROC)hinstLib.getProc("loadMethod");
  errorParser=parser;
  if(load)
    return load(parser, server);
  return 0;
}

/*!
 *Unload the plugin.
 */
int DynamicHttpCommand::unloadCommand(XmlParser*)
{
  unloadMethodPROC unload =(unloadMethodPROC) hinstLib.getProc("unloadMethod");
  if(unload)
    return unload(errorParser);
  return 0;
}

/*!
 *Does the method accept POST data? 
 */
int DynamicHttpCommand::acceptData()
{
  acceptDataPROC accept = (acceptDataPROC) hinstLib.getProc("acceptData");
  if(accept)
    return accept();
  else
    /*! By default assume that POST data is not used. */
    return 0;
}

/*!
 *Control a request.
 */
int DynamicHttpCommand::send(HttpThreadContext* context, ConnectionPtr lpconnection, 
                             string& Uri, int systemrequest, int OnlyHeader, int yetmapped)
{
  controlMethodPROC control = (controlMethodPROC)hinstLib.getProc("controlMethod");
  if(control)
    return control(context, lpconnection, Uri.c_str(), systemrequest, OnlyHeader, yetmapped);
  else
    return 0;
}


/*!
 *Initialize the object.
 */
DynHttpCommandManager::DynHttpCommandManager()
{

}

/*!
 *Destroy the object.
 */
DynHttpCommandManager::~DynHttpCommandManager()
{

}

/*!
 *Load the plugins in te specified directory.
 */
int DynHttpCommandManager::loadMethods(const char* directory, 
                                       XmlParser* p, Server* s)
{
	FindData fd;
  string filename;
  int ret;
  string completeFileName;
  if(directory==0)
  {
    filename.assign(s->getExternalPath());
    filename.append("/http_commands");
  }
  else
  {
    filename.assign(directory);
  }

#ifdef WIN32
  filename.append("/*.*");
#endif	

	ret = fd.findfirst(filename.c_str());	
	
  if(ret==-1)
  {
		return -1;	
  }

	do
	{	
		if(fd.name[0]=='.')
			continue;
		/*!
     *Do not consider file other than dynamic libraries.
     */
#ifdef WIN32
		if(!strstr(fd.name,".dll"))
#endif
#ifdef NOT_WIN
		if(!strstr(fd.name,".so"))
#endif		
			continue;
    completeFileName.assign(filename);
    completeFileName.append("/");
    completeFileName.append(fd.name);
		addMethod(completeFileName.c_str(), p, s);
	}while(!fd.findnext());
	fd.findclose();
  return 0;
}

/*!
 *Add a new method to the list. Returns 0 on success.
 */
int DynHttpCommandManager::addMethod(const char* fileName, 
                                     XmlParser* p, Server* s)
{
  DynamicHttpCommand *mod = new DynamicHttpCommand();
  string logBuf;
  char * methodName=0;
  if(mod == 0)
    return 1;
  if(mod->loadCommand(fileName, p, s))
  {
    delete mod;
    return 1;
  }
  methodName=mod->getCommandName(0);
  if(!methodName)
  {
    delete mod;
    return 1;
  }  
  logBuf.assign(p->getValue("MSG_LOADED"));
  logBuf.append(" ");
  logBuf.append(fileName);
  logBuf.append(" --> ");
  logBuf.append(methodName);
  s->logWriteln( logBuf.c_str() );
	{
		DynamicHttpCommand *old;
		string methodNameStr(methodName);
		old = data.put(methodNameStr, mod);
		if(old)
			delete old;
	}
  return 0;
}

/*!
 *Clean everything.
 */
int DynHttpCommandManager::clean()
{
	HashMap<string, DynamicHttpCommand*>::Iterator it = data.begin();
	HashMap<string, DynamicHttpCommand*>::Iterator end = data.end();
	
	for(;it != end; it++)
	{
		delete (*it);
	}
  data.clear();
  return 0;
}

/*!
 *Get a method by its name. Returns 0 on errors.
 */
DynamicHttpCommand* DynHttpCommandManager::getMethodByName(const char* name)
{
  return data.get(name);
}

/*!
 *Returns how many plugins were loaded.
 */
int DynHttpCommandManager::size()
{
  return data.size();
}
