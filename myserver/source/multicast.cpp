/*
MyServer
Copyright (C) 2007 The MyServer Team
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
#include "../include/multicast.h"
#ifndef MULTICAST_CPP
#define MULTICAST_CPP

/*!
 *Register the handler for the specified message type.
 */
template<typename MSG_TYPE, typename ARG_TYPE, typename RET_TYPE> 
void MulticastRegistry<MSG_TYPE, ARG_TYPE, RET_TYPE>::addMulticast(MSG_TYPE msg, Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>* handler)
{
	list<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*> msgHandlers = handlers.get(msg);
	if(!msgHandlers)
	{
		msgHandlers = new list<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*>();
		handlers.put(msg, msgHandlers);
	}
	msgHandlers.add(handler);
}


/*!
 *Remove the handler from the list for a message type.
 */
template<typename MSG_TYPE, typename ARG_TYPE, typename RET_TYPE> 
void MulticastRegistry<MSG_TYPE, ARG_TYPE, RET_TYPE>::removeMulticast(MSG_TYPE msg, Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>* handler)
{
	list<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*> msgHandlers = handlers.get(msg);
	if(!msgHandlers)
	{
		return;
	}
	msgHandlers.remove(handler);
}

/*!
 *Remove all the handlers for a specified message.
 */
template<typename MSG_TYPE, typename ARG_TYPE, typename RET_TYPE> 
void MulticastRegistry<MSG_TYPE, ARG_TYPE, RET_TYPE>::removeMulticasts(MSG_TYPE msg)
{
	list<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*> msgHandlers = handlers.remove(msg);
	if(msgHandlers)
		delete msgHandlers;
}

/*!
 *Notify the message to all the registered handlers passing an argument.
 */
template<typename MSG_TYPE, typename ARG_TYPE, typename RET_TYPE> 
void MulticastRegistry<MSG_TYPE, ARG_TYPE, RET_TYPE>::notifyMulticast(MSG_TYPE msg, ARG_TYPE arg)
{
	list<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*> msgHandlers = handlers.get(msg);

	/* Dirty but compile.  */
	list<void*>::iterator it;

	if(!msgHandlers)
		return;

	it = msgHandlers.begin();

	while(it != msgHandlers.end())
	{
		((Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*)*it)->updateMulticast(msg, arg);
		it++;
	}

}

/*!
 *Get the list of handlers for a specified message.
 */
template<typename MSG_TYPE, typename ARG_TYPE, typename RET_TYPE> 
list<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*>* MulticastRegistry<MSG_TYPE, ARG_TYPE, RET_TYPE>::getHandlers(MSG_TYPE msg)
{
	return handlers.get(msg);
}

/*!
 *Clear the registry.
 */
template<typename MSG_TYPE, typename ARG_TYPE, typename RET_TYPE>
void MulticastRegistry<MSG_TYPE, ARG_TYPE, RET_TYPE>::clearMulticastRegistry()
{
	HashMap<void*, list<void*>*>::Iterator it;
	it = handlers.begin();

	while(it != handlers.end())
	{
		delete (*it);
		it++;
	}
}


#endif
