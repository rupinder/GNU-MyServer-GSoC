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
#include <include/server/server.h>
#include <include/protocol/http/http.h>
#include "heading.h"

#ifdef WIN32
#define EXPORTABLE(x) x _declspec(dllexport)
#else
#define EXPORTABLE(x) extern "C" x
#endif


class RulesCheckerObserver : public Multicast<string, void*, int>
{
	struct Item
	{
		string condition;
		string event;
	};
public:

	virtual int updateMulticast(MulticastRegistry<string, void*, int>* reg, string& msg, void* arg)
	{
		HttpThreadContext *td = (HttpThreadContext*)arg;
		list<Item>::iterator it;

		ThreadContext context = {td};

		for(it = rules.begin(); it != rules.end(); it++)
		{
			int val;
			int ret = scan_string(&context, (*it).condition.c_str(), &val);
			if(ret)
				continue;
			if(val)
			{
				ret = scan_string(&context, (*it).event.c_str(), &val);
				free_strings(&context);

				return 1;
			}

		}
		free_strings(&context);
		return 0;
	}

	void addRule(const char* rule)
	{
		Item it;
		string ruleStr(rule);

		size_t pos = ruleStr.find("=>", 0);

		if(pos == string::npos)
			return; //TODO: error.

		it.condition.assign(ruleStr.substr(0, pos));
		it.event.assign(ruleStr.substr(pos+2, string::npos));
		rules.push_back(it);
	}

private:
	list<Item> rules;
};

static RulesCheckerObserver observer;


EXPORTABLE(char*) name(char* name, u_long len)
{
	char* str = (char*)"rules_checker";
	if(name)
		strncpy(name, str, len);
	return str;
}

EXPORTABLE(int) load(void* server,void* parser)
{
	Server* serverInstance = (Server*)server;
	HttpStaticData* staticData =(HttpStaticData*) serverInstance->getGlobalData("http-static");
	string msg("new-http-request");
	XmlParser* configuration;
	xmlDocPtr xmlDoc;
	if(!staticData)
	{
		serverInstance->logLockAccess();
		serverInstance->logPreparePrintError();
		serverInstance->logWriteln("RulesChecker: Invalid HTTP static data");
		serverInstance->logEndPrintError();
		serverInstance->logUnlockAccess();
		return -1;
	}


	staticData->addMulticast(msg, &observer);

	configuration = serverInstance->getConfiguration();
	xmlDoc = configuration->getDoc();

	for(xmlNodePtr ptr = xmlDoc->children->next->children; ptr; ptr = ptr->next)
	{
		char* data;
		if(!xmlStrcmp(ptr->name, (const xmlChar *)"RULES_CHECKER_RULE"))
		{

			if(ptr->children && ptr->children->next && ptr->children->next->content)
				data = (char*)ptr->children->next->content;

			if(!data)
			{
				serverInstance->logLockAccess();
				serverInstance->logPreparePrintError();
				serverInstance->logWriteln("RulesChecker: Invalid rule");
				serverInstance->logEndPrintError();
				serverInstance->logUnlockAccess();
				return -1;
			}

			observer.addRule(data);
		}

	}

	return 0;
}

EXPORTABLE(int) postLoad(void* server,void* parser)
{
	return 0;
}
EXPORTABLE(int) unLoad(void* parser)
{
	return 0;
}

