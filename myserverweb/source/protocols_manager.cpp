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

#ifdef NOT_WIN
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

// Bloodshed Dev-C++ Helper
#ifndef intptr_t
#define intptr_t int
#endif

typedef int (*loadProtocolPROC)(void*,char*,void*); 
typedef int (*unloadProtocolPROC)(void* languageParser); 
typedef int (*controlConnectionPROC)(void* ,char*,char*,int,int,u_long,u_long); 
typedef char* (*registerNamePROC)(char*,int); 

/*!
 *Load the protocol. Called once at runtime.
 */
int DynamicProtocol::loadProtocol(XmlParser* languageParser,char* confFile,
                                   Server* lserver)
{
  errorParser = languageParser;
	loadProtocolPROC Proc;
	int ret=0;
	ret = hinstLib.loadLibrary(filename);

	if(!ret)
	{
    char *log_str = new char[strlen(languageParser->getValue("ERR_LOADED")) +
                             strlen(filename) +2 ];
    if(log_str == 0)
      return 0;
    sprintf(log_str,"%s %s", languageParser->getValue("ERR_LOADED"), filename);
    lserver->logWriteln(log_str);  
    delete [] log_str;
		return 0;
	}
  Proc = (loadProtocolPROC) hinstLib.getProc( "loadProtocol"); 
	
	if(Proc)
		ret = (Proc((void*)languageParser,confFile,(void*)lserver));
	if(ret)
		ret = registerName(protocolName,16)[0] != '\0' ? 1 : 0 ;
	return ret;
}

/*!
 *Unload the protocol. Called once.
 */
int DynamicProtocol::unloadProtocol(XmlParser* languageParser)
{
	unloadProtocolPROC Proc=0;
  if(filename)
    delete [] filename;
  filename = 0;
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
	return  PROTOCOL_OPTIONS;
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
	PROTOCOL_OPTIONS=0;
  filename = 0;
  errorParser=0;
}

/*!
 *Destroy the protocol object.
 */
DynamicProtocol::~DynamicProtocol()
{
  unloadProtocol(errorParser);
  errorParser=0;
	PROTOCOL_OPTIONS=0;
  filename = 0;
}

/*!
 *Set the right file name
 *Returns nonzero on errors.
 */
int DynamicProtocol::setFilename(char *nf)
{
  int filenamelen;
  if(filename)
    delete [] filename;
  filename = 0;
  filenamelen = strlen(nf) + 1;
  filename = new char[filenamelen];
  if(filename == 0)
    return 1;
	strncpy(filename, nf, filenamelen);
	return 0;
}

/*!
 *Add a new protocol to the list by its module name.
 */
int ProtocolsManager::addProtocol(char *file, XmlParser* parser,
                                   char* confFile, Server* lserver)
{
	DynamicProtocolListElement* ne = new DynamicProtocolListElement();
	ne->data.setFilename(file);
	ne->data.loadProtocol(parser, confFile, lserver);
	ne->next=list;
	list=ne;
  char *log_buf = new char[strlen(parser->getValue("MSG_LOADED")) + strlen(file) 
                           + strlen( ne->data.getProtocolName()) + 7 ];
  if(log_buf == 0)
    return 1;
  sprintf(log_buf, "%s %s --> %s", parser->getValue("MSG_LOADED"), file, 
          ne->data.getProtocolName() );
  lserver->logWriteln( log_buf );
  delete [] log_buf;
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
DynamicProtocol* ProtocolsManager::getDynProtocol(char *protocolName)
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
int ProtocolsManager::loadProtocols(char* directory, XmlParser* parser,
                                     char* confFile, Server* lserver)
{
	FindData fd;
  int filenamelen = 0;
	char *filename = 0;
  int ret;
  char *completeFileName = 0;
  int completeFileNameLen = 0;
#ifdef WIN32
  filenamelen=strlen(directory)+6;
  filename=new char[filenamelen];
  if(filename == 0)
    return -1;
	sprintf(filename,"%s/*.*", directory);
#endif	
#ifdef NOT_WIN
  filenamelen=strlen(directory)+2;
  filename=new char[filenamelen];
  if(filename == 0)
    return -1;
	strncpy(filename, directory, filenamelen);
#endif	
	
	ret = fd.findfirst(filename);	
	
  if(ret==-1)
  {
    delete [] filename;
    filename = 0;
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
    completeFileNameLen = strlen(directory) + strlen(fd.name) + 2;
		completeFileName = new char[completeFileNameLen];
    if(completeFileName == 0)
    {
      delete [] filename;
      filename = 0;
      return -1;
    }
		sprintf(completeFileName,"%s/%s", directory, fd.name);
		addProtocol(completeFileName, parser, confFile, lserver);
		delete [] completeFileName;
	}while(!fd.findnext());
	fd.findclose();
  delete [] filename;
  filename = 0;
  return 0;
}
