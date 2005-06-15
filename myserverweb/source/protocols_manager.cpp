/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "../include/protocols_manager.h"
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

typedef int (*loadProtocolPROC)(void*,void*); 
typedef int (*unloadProtocolPROC)(void* languageParser); 
typedef int (*controlConnectionPROC)(void* ,char*,char*,int,int,u_long,u_long); 
typedef char* (*registerNamePROC)(char*,int); 

/*!
 *Load the protocol. Called once at runtime.
 */
int DynamicProtocol::loadProtocol(XmlParser* languageParser, Server* lserver)
{
	loadProtocolPROC Proc;
	int ret=0;
  errorParser = languageParser;
	ret = hinstLib.loadLibrary(filename.c_str());

	if(ret)
	{
    string log_str;
    log_str.assign(languageParser->getValue("ERR_LOADED"));
    log_str.append(" ");
    log_str.append(filename);
    lserver->logWriteln(log_str.c_str());  
		return 0;
	}
  Proc = (loadProtocolPROC) hinstLib.getProc( "loadProtocol"); 
	
	if(Proc)
  {
    ret = registerName(protocolName,16)[0] != '\0' ? 1 : 0 ;
 		if(ret)
      ret = (Proc((void*)languageParser, (void*)lserver));
	}
  else
    ret = 0;
  return ret;
}

/*!
 *Unload the protocol. Called once.
 */
int DynamicProtocol::unloadProtocol(XmlParser* languageParser)
{
	unloadProtocolPROC Proc=0;
  filename.assign("");
  Proc = (unloadProtocolPROC) hinstLib.getProc("unloadProtocol"); 
	
	if(Proc)
		Proc((void*)languageParser);
	/*!
   *Free the loaded module too.
   */
	hinstLib.close();
	return 1;
}
/*!
 *Return the protocol name.
 */
char *DynamicProtocol::getProtocolName()
{
	return protocolName;	
}

/*!
 *Get the options for the protocol.
 */
int DynamicProtocol::getOptions()
{
	return  protocolOptions;
}


/*!
 *Control the connection.
 */
int DynamicProtocol::controlConnection(ConnectionPtr a,char *b1,char *b2,
                                       int bs1, int bs2,u_long nbtr,u_long id)
{
	controlConnectionPROC proc;
	proc = (controlConnectionPROC)hinstLib.getProc("controlConnection"); 

	if(proc)
		return proc((void*)a, b1, b2, bs1, bs2, nbtr, id);
	else
		return 0;
}
/*!
 *Returns the name of the protocol. If an out buffer is defined 
 *fullfill it with the name too.
 */
char* DynamicProtocol::registerName(char* out,int len)
{
	registerNamePROC proc;
	proc =(registerNamePROC) hinstLib.getProc( "registerName"); 
	if(proc)
		return proc(out,len);
	else
	{
		return 0;
	}
}
/*!
 *Constructor for the class protocol.
 */
DynamicProtocol::DynamicProtocol()
{
	protocolOptions=0;
  filename.assign("");
  errorParser=0;
}

/*!
 *Destroy the protocol object.
 */
DynamicProtocol::~DynamicProtocol()
{
  unloadProtocol(errorParser);
  errorParser=0;
	protocolOptions=0;
  filename.assign("");
}

/*!
 *Set the right file name
 *Returns nonzero on errors.
 */
int DynamicProtocol::setFilename(const char *nf)
{
  if(nf == 0)
    return 1;
	filename.assign(nf);
	return 0;
}

/*!
 *Add a new protocol to the list by its module name.
 */
int ProtocolsManager::addProtocol(const char *file, XmlParser* parser,
                                   char* confFile, Server* lserver)
{
  string logBuf;
	DynamicProtocolListElement* ne = new DynamicProtocolListElement();
	ne->data.setFilename(file);
	ne->data.loadProtocol(parser, lserver);
	ne->next=list;
	list=ne;
  logBuf.assign(parser->getValue("MSG_LOADED"));
  logBuf.append(" ");
  logBuf.append(file);
  logBuf.append(" --> ");
  logBuf.append( ne->data.getProtocolName());
  lserver->logWriteln( logBuf.c_str() );
	return 1;
}


/*!
 *Unload evey loaded protocol.
 */
int ProtocolsManager::unloadProtocols(XmlParser *parser)
{
	DynamicProtocolListElement* ce=list;
	DynamicProtocolListElement* ne=0;
	if(ce)
		ne=list->next;
	while(ce)
	{
		ce->data.unloadProtocol(parser);
		delete ce;
		ce=ne;
		if(ne)
			ne=ne->next;
		
	}
	list=0;
	return 1;
}

/*!
 *Class constructor.
 */
ProtocolsManager::ProtocolsManager()
{
	list=0;
}

/*!
 *Get a dynamic protocol using its index in the list.
 */
DynamicProtocol* ProtocolsManager::getDynProtocolByOrder(int order)
{
  int i = 0;
	DynamicProtocolListElement* ne=list;
  while(order != i)
  {
    if(ne == 0)
      return 0;
    i++;
    ne = ne->next;
  }
  return ((ne != 0) ? (&(ne->data)) : 0) ;
}

/*!
 *Get the dynamic protocol by its name.
 */
DynamicProtocol* ProtocolsManager::getDynProtocol(const char *protocolName)
{
	DynamicProtocolListElement* ne=list;
	while(ne)
	{
		if(!lstrcmpi(protocolName,ne->data.getProtocolName()))
			return &(ne->data);
		ne=ne->next;
		
	}
	return 0;
	
}
/*!
 *Load all the protocols present in the directory.
 *Returns Nonzero on errors.
 */
int ProtocolsManager::loadProtocols(const char* directory, XmlParser* parser,
                                     char* confFile, Server* lserver)
{
	FindData fd;
  string filename;
  int ret;
  string completeFileName;
#ifdef WIN32
  filename.assign(directory);
  filename.append("/*.*");
#endif	

#ifdef NOT_WIN
	filename.assign(directory);
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
    completeFileName.assign(directory);
    completeFileName.append("/");
    completeFileName.append(fd.name);
		addProtocol(completeFileName.c_str(), parser, confFile, lserver);
	}while(!fd.findnext());
	fd.findclose();
  return 0;
}
