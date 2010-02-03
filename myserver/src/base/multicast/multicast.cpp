/*
  MyServer
  Copyright (C) 2007, 2008, 2009, 2010 Free Software Foundation, Inc.
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

#ifndef MULTICAST_CPP
# define MULTICAST_CPP

#include "myserver.h"
#include <include/base/multicast/multicast.h>


/*!
 *Register the handler for the specified message type.
 */
template<typename MSG_TYPE, typename ARG_TYPE, typename RET_TYPE>
void MulticastRegistry<MSG_TYPE, ARG_TYPE, RET_TYPE>::addMulticast (MSG_TYPE msg, Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>* handler)
{
  vector<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*> *msgHandlers = handlers.get (msg);
  if (!msgHandlers)
  {
    msgHandlers = new vector<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*>();
    handlers.put (msg, msgHandlers);
  }
  msgHandlers->push_back (handler);
}


/*!
 *Remove the handler from the vector for a message type.
 */
template<typename MSG_TYPE, typename ARG_TYPE, typename RET_TYPE>
void MulticastRegistry<MSG_TYPE, ARG_TYPE, RET_TYPE>::removeMulticast (MSG_TYPE msg, Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>* handler)
{
  vector<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*> *msgHandlers = handlers.get (msg);
  typename vector<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE> * >::iterator it;
  if (!msgHandlers)
  {
    return;
  }

  for (it = msgHandlers->begin (); it != msgHandlers->end (); it++)
    if (*it == handler)
    {
      msgHandlers->erase (it);
      break;
    }
}

/*!
 *Remove all the handlers for a specified message.
 */
template<typename MSG_TYPE, typename ARG_TYPE, typename RET_TYPE>
void MulticastRegistry<MSG_TYPE, ARG_TYPE, RET_TYPE>::removeMulticasts (MSG_TYPE msg)
{
  vector<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*> *msgHandlers = handlers.remove (msg);
  if (msgHandlers)
  {
    delete msgHandlers;
  }
}

/*!
 *Notify the message to all the registered handlers passing an argument.
 */
template<typename MSG_TYPE, typename ARG_TYPE, typename RET_TYPE>
void MulticastRegistry<MSG_TYPE, ARG_TYPE, RET_TYPE>::notifyMulticast (MSG_TYPE& msg, ARG_TYPE arg)
{
  vector<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*> *msgHandlers = handlers.get (msg);

  if (!msgHandlers)
    return;

  for (size_t i = 0; i < msgHandlers->size (); i++)
  {
    Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>* multicast = (*msgHandlers)[i];
    multicast->updateMulticast (this, msg, arg);
  }

}

/*!
 *Get the vector of handlers for a specified message.
 */
template<typename MSG_TYPE, typename ARG_TYPE, typename RET_TYPE>
vector<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*>* MulticastRegistry<MSG_TYPE, ARG_TYPE, RET_TYPE>::getHandlers (MSG_TYPE& msg)
{
  return handlers.get (msg);
}

/*!
 *Clear the registry.
 */
template<typename MSG_TYPE, typename ARG_TYPE, typename RET_TYPE>
void MulticastRegistry<MSG_TYPE, ARG_TYPE, RET_TYPE>::clearMulticastRegistry ()
{
  /* Dirty code, but compile and works.  */
  HashMap<void*, void*>* ptrHandlers = (HashMap<void*, void*>*) &handlers;
  HashMap<void*, void*>::Iterator it = ptrHandlers->begin ();
  HashMap<void*, void*>::Iterator end = ptrHandlers->end ();
  vector<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*>* v;

  while (it != end)
  {
    v = (vector<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*>*) *it;
    delete v;
    it++;
  }
  handlers.clear ();
}


#endif
