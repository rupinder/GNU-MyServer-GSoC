/*
*MyServer
*Copyright (C) 2002 The MyServer Team
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

#ifdef NOT_WIN
#include "../include/lfind.h"

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
int dynamic_protocol::loadProtocol(cXMLParser* languageParser,char* confFile,cserver* lserver)
{
#ifdef WIN32
	hinstLib = LoadLibrary(filename);
#endif
#ifdef HAVE_DL
	hinstLib = dlopen(filename, RTLD_LAZY);
#endif
	if(hinstLib==0)
	{
		printf("%s %s\n",languageParser->getValue("ERR_LOADED"),filename);		
		return 0;
	}
	loadProtocolPROC Proc;
#ifdef WIN32
		Proc = (loadProtocolPROC) GetProcAddress(hinstLib, "loadProtocol"); 
#endif
#ifdef HAVE_DL
		Proc = (loadProtocolPROC) dlsym(hinstLib, "loadProtocol");
#endif	
	int ret=0;
	if(Proc)
		ret = (Proc((void*)languageParser,confFile,(void*)lserver));
	if(ret)
		ret = registerName(protocolName,16)[0]!='\0' ?1:0;
	return ret;
}

/*!
*Unload the protocol. Called once.
*/
int dynamic_protocol::unloadProtocol(cXMLParser* languageParser)
{
	unloadProtocolPROC Proc;
#ifdef WIN32
	Proc = (unloadProtocolPROC) GetProcAddress(hinstLib, "unloadProtocol"); 
#endif
#ifdef HAVE_DL
	Proc = (unloadProtocolPROC) dlsym(hinstLib, "unloadProtocol");
#endif	
	if(Proc)
		Proc((void*)languageParser);
	/*
	*Free the loaded module too.
	*/
	if(hinstLib)
	{
#ifdef WIN32
		FreeLibrary(hinstLib); 
#endif
#ifdef HAVE_DL
		dlclose(hinstLib);
#endif	
	}
	hinstLib=0;
	return 1;
}
/*!
*Return the protocol name.
*/
char *dynamic_protocol::getProtocolName()
{
	return protocolName;	
}

/*!
*Control the connection
*/
int dynamic_protocol::controlConnection(LPCONNECTION a,char *b1,char *b2,int bs1,int bs2,u_long nbtr,u_long id)
{
	controlConnectionPROC Proc;
#ifdef WIN32
	Proc = (controlConnectionPROC) GetProcAddress(hinstLib, "controlConnection"); 
#endif
#ifdef HAVE_DL
	Proc = (controlConnectionPROC) dlsym(hinstLib, "controlConnection");
#endif	
	if(Proc)
		return Proc((void*)a,b1,b2,bs1,bs2,nbtr,id);
	else
		return 0;
}
/*!
*Returns the name of the protocol. If an out buffer is defined fullfill it with the name too.
*/
char* dynamic_protocol::registerName(char* out,int len)
{
	registerNamePROC Proc;
#ifdef WIN32
	Proc = (registerNamePROC) GetProcAddress(hinstLib, "registerName"); 
#endif
#ifdef HAVE_DL
	Proc = (registerNamePROC) dlsym(hinstLib, "registerName");
#endif	
	if(Proc)
		return Proc(out,len);
	else
	{
		return 0;
	}
}
/*!
*Constructor for the class protocol.
*/
dynamic_protocol::dynamic_protocol()
{
	hinstLib=0;
	PROTOCOL_OPTIONS=0;
}

/*!
*Set the right file name
*/
int dynamic_protocol::setFilename(char *nf)
{
	strncpy(filename,nf,MAX_PATH);
	return 1;
}

/*!
*Add a new protocol to the list by its module name.
*/
int protocols_manager::addProtocol(char *file,cXMLParser* parser,char* confFile,cserver* lserver)
{
	dynamic_protocol_list_element* ne=new dynamic_protocol_list_element();
	ne->data.setFilename(file);
	ne->data.loadProtocol(parser,confFile,lserver);
	ne->next=list;
	list=ne;
	printf("%s %s --> %s\n",parser->getValue("MSG_LOADED"),file,ne->data.getProtocolName());
	return 1;
}
/*!
*Unload evey loaded protocol.
*/
int protocols_manager::unloadProtocols(cXMLParser *parser)
{
	dynamic_protocol_list_element* ce=list;
	dynamic_protocol_list_element* ne=0;
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
*Class constructor
*/
protocols_manager::protocols_manager()
{
	list=0;
}
/*!
*Get the dynamic protocol by its name.
*/
dynamic_protocol* protocols_manager::getDynProtocol(char *protocolName)
{
	dynamic_protocol_list_element* ne=list;
	while(ne)
	{
		if(!lstrcmpi(protocolName,ne->data.getProtocolName()))
			return &(ne->data);
		ne=ne->next;
		
	}
	return 0;
	
}
/*!
*Load all the protocols present in the folder.
*/
void protocols_manager::loadProtocols(char* folder,cXMLParser* parser,char* confFile,cserver* lserver)
{
	char filename[MAX_PATH];
#ifdef WIN32
	sprintf(filename,"%s/*.*",folder);
#endif	
#ifdef NOT_WIN
	sprintf(filename,"%s/",folder);
#endif	
	
	_finddata_t fd;
	intptr_t ff;
	ff=(intptr_t)_findfirst(filename,&fd);	
#ifdef WIN32
	if(ff==-1)
#endif
#ifdef NOT_WIN
	if((int)ff==-1)
#endif
		return;	
	do
	{	
		if(fd.name[0]=='.')
			continue;
		/*
		*Do not consider file other than dynamic libraries
		*/
#ifdef WIN32
		if(!strstr(fd.name,".dll"))
#endif
#ifdef NOT_WIN
		if(!strstr(fd.name,".so"))
#endif		
			continue;
		
		sprintf(filename,"%s/%s",folder,fd.name);
		
		addProtocol(filename,parser,confFile,lserver);
		
	}while(!_findnext(ff,&fd));
	_findclose(ff);		

}
