/*
*MyServer
*Copyright (C) 2005 The MyServer Team
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

#include <string>
#include <stdexcept>
/*! Do not include the file multiple times. */
#define HASH_DICTIONARY_H_NO_SRC
#include "../include/hash_dictionary.h"

using namespace std;

/*!
 *Create the object.
 */
template<class T>
HashDictionary<T>::HashDictionary()
{
  data.clear();
}

/*!
 *Internal hashing function. This function uses the ELF hash algorithm.
 */
template<typename T>
unsigned int HashDictionary<T>::hash(const char * name)
{
   unsigned int ret,x;
   int i;
   int strLen = strlen(name);
   ret = x = i = 0;  
   for(i = 0; i < strLen; name++, i++)
   {
      ret = (ret << 4) + (*name);
      x = ret & 0xF0000000L;
      if(x != 0)
      {
         ret ^= (x >> 24);
         ret &= ~x;
      }
   }
   return (ret & 0x7FFFFFFF);
}

/*!
 *Destroy the object.
 */
template<typename T>
HashDictionary<T>::~HashDictionary()
{
  free();
}

/*!
 *Get the data of the node by name.
 */
template<typename T>
T HashDictionary<T>::getData(const char* name)
{
  unsigned int nameHash = hash(name);
  class map<int, sNode*>::iterator iter = data.find(nameHash);
  if(iter != data.end())
    return (*iter).second->data;
  else
    return 0;
}

/*!
 *Remove a node by name. The function returns the data for the removed node.
 *If the node doesn't exist then zero is returned.
 */
template<typename T>
T HashDictionary<T>::removeNode(const char* name)
{
  unsigned int nodeHash = hash(name);
  class map<int, sNode*>::iterator iter = data.find(nodeHash); 
  
  if(iter != data.end())
  {
    T ret=((sNode*)(*iter).second)->data;
    data.erase(iter);
    delete (*iter).second;
    return ret;
  }
  return 0;
}

/*!
 *Return the number of nodes currently in the dictionary.
 */
template<typename T>
int HashDictionary<T>::nodesNumber()
{
  return static_cast<int>(data.size());
}

/*!
 *Free the dictionary.
 */
template<typename T>
void HashDictionary<T>::free()
{
  class map<int, sNode*>::iterator iter = data.begin(); 
  for( ; iter != data.end(); iter++)
  {
    delete (*iter).second;
  }
  data.clear();
}

/*!
 *Get the data for the node using the order position. First node has index 0.
 */
template<typename T>
T HashDictionary<T>::getData(int order)
{
  int i;
  class map<int, sNode*>::iterator iter;
  if(order == 0)
    return 0;
  iter = data.begin(); 
  for(i = 0; (i < order) && ( iter != data.end() ); i++);
  return (iter!=data.end() ? (*iter).second->data : 0);
}

/*!
 *Check if the dictionary is empty.
 */
template<typename T>
int HashDictionary<T>::isEmpty()
{
  return data.empty() ? 1 : 0;
}

/*!
 *Create a copy of an existent dictionary.
 */
template<typename T>
int HashDictionary<T>::clone(HashDictionary<T>& hd)
{
  typename map<int, sNode*>::iterator i = hd.data.begin();
  
  for( ; i != hd.data.end(); i++)
  {
    try
    {
      sNode *copy=new sNode((*i)->second->hash, (*i)->data);
      data[copy->hash]=copy;
    }
    catch(...)
    {
      free();
      return 1;
    }
  }
  return 0;
}

/*!
 *Copy constructor.
 */
template<typename T>
HashDictionary<T>::HashDictionary(HashDictionary<T>& h)
{
  if(clone(h))
    throw runtime_error("");
}

/*!
 *Insert a new node at the beginning of the list. Returns zero on success.
 */
template<typename T>
int HashDictionary<T>::insert(const char* name, T dataPtr)
{
  if(name == 0)
    return -1;
  sNode *newNode = new sNode();
  if(newNode == 0)
    return -1;
  newNode->hash = hash(name);
  newNode->data = dataPtr;
  data[newNode->hash]=newNode;
  return 0;
}


/*!
 *Remove a node by its position.
 */
template<typename T>
T HashDictionary<T>::removeNodeAt(int order)
{
  class map<int, sNode*>::iterator iter=data.begin();
  int pos = 1;
  T ret = 0;
  while((pos<order) && (iter!=data.end()) )
  {
    pos++;
    iter++;
  }

  if(pos != order)
    return 0;

  ret = (*iter).second->data;

  data.erase(iter);
  
  delete (*iter).second;

  return ret;
}
