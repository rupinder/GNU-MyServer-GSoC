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

typedef char* (*loadProtocolPROC)(cXMLParser*,char*); 
typedef char* (*unloadProtocolPROC)(cXMLParser* languageParser); 
typedef int (*controlConnectionPROC)(LPCONNECTION ,char*,char*,int,int,u_long,u_long); 
typedef char* (*registerNamePROC)(char*,int); 

/*!
*Load the protocol. Called once at runtime.
*/
int dynamic_protocol::loadProtocol(cXMLParser* languageParser,char* confFile)
{
#ifdef WIN32
	hinstLib = LoadLibrary(filename);
#endif
#ifdef HAVE_DL
	hinstLib = dlopen(filename, RTLD_LAZY);
#endif

	loadProtocolPROC Proc;
#ifdef WIN32
		Proc = (loadProtocolPROC) GetProcAddress(hinstLib, "loadProtocol"); 
#endif
#ifdef HAVE_DL
		Proc = (loadProtocolPROC) dlsym(hinstLib, "loadProtocol");
#endif	
	int ret=0;
	if(Proc)
		ret = ((Proc(languageParser,confFile)[0]!='\0')?1:0);

	registerName(protocolName,16);

	return ret;
}

/*!
*Unload the protocol. Called once.
*/
int dynamic_protocol::unloadProtocol(cXMLParser* languageParser)
{
	unloadProtocolPROC Proc;
#ifdef WIN32
	Proc = (unloadProtocolPROC) GetProcAddress(hinstLib, "registerName"); 
#endif
#ifdef HAVE_DL
	Proc = (unloadProtocolPROC) dlsym(hinstLib, "registerName");
#endif	
	if(Proc)
		Proc(languageParser);
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
		return Proc(a,b1,b2,bs1,bs2,nbtr,id);
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
		return 0;
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
int protocols_manager::addProtocol(char *file)
{
	dynamic_protocol_list_element* ne=new dynamic_protocol_list_element();
	ne->data.setFilename(file);
	ne->next=list;
	list=ne;
	
	return 1;
}
/*!
*Unload evey loaded protocol.
*/
int protocols_manager::unloadProtocols()
{
	dynamic_protocol_list_element* ce=list;
	dynamic_protocol_list_element* ne=list->next;
	while(ce)
	{
		delete ce;
		ce=ne;
		ne=ne->next;
		
	}
	list=0;
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
		if(!strcmpi(protocolName,ne->data.getProtocolName()))
			return &(ne->data);
		ne=ne->next;
		
	}
	return 0;
	
}
