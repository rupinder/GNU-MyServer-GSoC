/*
MyServer
Copyright (C) 2007 The MyServer Team
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
#include "../include/multicast.h"
#ifndef MULTICAST_CPP
#define MULTICAST_CPP

/*!
 *Register the handler for the specified message type.
 */
template<typename MSG_TYPE, typename ARG_TYPE, typename RET_TYPE> 
void MulticastRegistry<MSG_TYPE, ARG_TYPE, RET_TYPE>::addMulticast(MSG_TYPE msg, Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>* handler)
{
	vector<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*> *msgHandlers = handlers.get(msg);
	if(!msgHandlers)
	{
		msgHandlers = new vector<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*>();
		handlers.put(msg, msgHandlers);
	}
	msgHandlers->push_back(handler);
}


/*!
 *Remove the handler from the vector for a message type.
 */
template<typename MSG_TYPE, typename ARG_TYPE, typename RET_TYPE> 
void MulticastRegistry<MSG_TYPE, ARG_TYPE, RET_TYPE>::removeMulticast(MSG_TYPE msg, Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>* handler)
{
	vector<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*> *msgHandlers = handlers.get(msg);
	if(!msgHandlers)
	{
		return;
	}
	msgHandlers->remove(handler);
}

/*!
 *Remove all the handlers for a specified message.
 */
template<typename MSG_TYPE, typename ARG_TYPE, typename RET_TYPE> 
void MulticastRegistry<MSG_TYPE, ARG_TYPE, RET_TYPE>::removeMulticasts(MSG_TYPE msg)
{
	vector<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*> *msgHandlers = handlers.remove(msg);
	if(msgHandlers)
		delete msgHandlers;
}

/*!
 *Notify the message to all the registered handlers passing an argument.
 */
template<typename MSG_TYPE, typename ARG_TYPE, typename RET_TYPE> 
void MulticastRegistry<MSG_TYPE, ARG_TYPE, RET_TYPE>::notifyMulticast(MSG_TYPE& msg, ARG_TYPE arg)
{
	vector<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*> *msgHandlers = handlers.get(msg);

	if(!msgHandlers)
		return;

	for(size_t i = 0; i < msgHandlers->size(); i++)
	{
		Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>* multicast = (*msgHandlers)[i];
		multicast->updateMulticast(this, msg, arg);
	}

}

/*!
 *Get the vector of handlers for a specified message.
 */
template<typename MSG_TYPE, typename ARG_TYPE, typename RET_TYPE> 
vector<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*>* MulticastRegistry<MSG_TYPE, ARG_TYPE, RET_TYPE>::getHandlers(MSG_TYPE& msg)
{
	return handlers.get(msg);
}

/*!
 *Clear the registry.
 */
template<typename MSG_TYPE, typename ARG_TYPE, typename RET_TYPE>
void MulticastRegistry<MSG_TYPE, ARG_TYPE, RET_TYPE>::clearMulticastRegistry()
{
	/* Dirty code, but compile and works.  */
	HashMap<void*, void*>* ptrHandlers = (HashMap<void*, void*>*) &handlers;
	HashMap<void*, void*>::Iterator it = ptrHandlers->begin();
	HashMap<void*, void*>::Iterator end = ptrHandlers->end();
	vector<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*>* v;

	while(it != end)
	{
		v = (vector<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*>*) *it;
		delete v;
		it++;
	}
}


#endif
