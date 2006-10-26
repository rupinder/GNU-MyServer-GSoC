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


#include "../include/dyn_http_manager.h"
#include "../include/xml_parser.h"
#include "../include/server.h"
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

typedef int (*loadManagerPROC)(void*, void*); 
typedef int (*unloadManagerPROC)(void* languageParser); 
typedef int (*sendManagerPROC)(void*, volatile void*, const char*, const char*, int); 
typedef char* (*registerNamePROC)(char*, int); 

/*!
 *Get the command name.
 */
char *DynamicHttpManager::getManagerName(char* out, int len)
{
  registerNamePROC name = (registerNamePROC)hinstLib.getProc("registerName");
  if(name)
    return name(out, len);
  return 0;
}

/*!
 *Default constructor.
 */
DynamicHttpManager::DynamicHttpManager()
{

}

/*!
 *Destroy the object.
 */
DynamicHttpManager::~DynamicHttpManager()
{
  hinstLib.close();
}

/*!
 *Load the plugin. Returns 0 on success.
 */
int DynamicHttpManager::loadManager(const char* name, XmlParser* parser, 
                                    Server* server)
{
  if(hinstLib.loadLibrary(name))
    return 1;
  loadManagerPROC load =(loadManagerPROC)hinstLib.getProc("loadManager");
  errorParser=parser;
  if(load)
    return load(parser, server);
  return 0;
}

/*!
 *Unload the plugin.
 */
int DynamicHttpManager::unloadManager(XmlParser*)
{
  unloadManagerPROC unload =(unloadManagerPROC) hinstLib.getProc("unloadManager");
  if(unload)
    return unload(errorParser);
  return 0;
}


/*!
 *Control a request.
 */
int DynamicHttpManager::send(HttpThreadContext* context, ConnectionPtr s, 
                             const char *filenamePath, const char* cgi, 
                             int onlyHeader)
{
  sendManagerPROC control = (sendManagerPROC)hinstLib.getProc("sendManager");
  if(control)
    return control(context, s, filenamePath, cgi, onlyHeader);
  else
    return 0;
}


/*!
 *Initialize the object.
 */
DynHttpManagerList::DynHttpManagerList()
{

}

/*!
 *Destroy the object.
 */
DynHttpManagerList::~DynHttpManagerList()
{

}

/*!
 *Load the plugins in te specified directory.
 */
int DynHttpManagerList::loadManagers(const char* directory, 
                                    XmlParser* p, Server* s)
{
	FindData fd;
  string filename;
  int ret;
  string completeFileName;
  if(directory==0)
  {
    filename.assign(s->getExternalPath());
    filename.append("/http_managers");
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
		addManager(completeFileName.c_str(), p, s);
	}while(!fd.findnext());
	fd.findclose();
  return 0;
}

/*!
 *Add a new method to the list. Returns 0 on success.
 */
int DynHttpManagerList::addManager(const char* fileName, 
                                   XmlParser* p, Server* s)
{
  DynamicHttpManager *man = new DynamicHttpManager();
  string logBuf;
  char * managerName=0;
  if(man == 0)
    return 1;
  if(man->loadManager(fileName, p, s))
  {
    delete man;
    return 1;
  }
  managerName=man->getManagerName(0);

  if(!managerName)
  {
    delete man;
    return 1;
  }  

  logBuf.assign(p->getValue("MSG_LOADED"));
  logBuf.append(" ");
  logBuf.append(fileName);
  logBuf.append(" --> ");
  logBuf.append(managerName);
  s->logWriteln( logBuf.c_str() );
	{
		DynamicHttpManager *old;
		string managerNameStr(managerName);
		old = data.put(managerNameStr, man);
		if(old)
			delete old;
	}  
	return 0;
}

/*!
 *Clean everything.
 */
int DynHttpManagerList::clean()
{
	HashMap<string, DynamicHttpManager*>::Iterator it = data.begin();
	HashMap<string, DynamicHttpManager*>::Iterator end = data.end();
	
	for (;it != end; it++)
	{
		delete (*it);
	}
  data.clear();
  return 0;
}

/*!
 *Get a method by its name. Returns 0 on errors.
 */
DynamicHttpManager* DynHttpManagerList::getManagerByName(const char* name)
{
  return data.get(name);
}

/*!
 *Returns how many plugins were loaded.
 */
int DynHttpManagerList::size()
{
  return data.size();
}
