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

#include "string.h"
#include "../include/hash_dictionary.h"

/*!
 *Create the object.
 */
HashDictionary::HashDictionary()
{
  data.clear();
}

/*!
 *Internal hashing function. This function uses the ELF hash algorithm.
 */
unsigned int HashDictionary::hash(const char * name)
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
HashDictionary::~HashDictionary()
{
  free();
}

/*!
 *Get the data of the node by name.
 */
void *HashDictionary::getData(const char* name)
{
  unsigned int nameHash = hash(name);
  map<int, sNode*>::iterator iter = data.find(nameHash);
  if(iter != data.end())
    return (*iter).second->data;
  else
    return 0;
}

/*!
 *Remove a node by name. The function returns the data for the removed node. If
 *there is not any node then zero is returned.
 */
void* HashDictionary::removeNode(const char* name)
{
  unsigned int nodeHash = hash(name);
  map<int, sNode*>::iterator iter = data.find(nodeHash); 
  
  if(iter != data.end())
  {
    void* ret=((sNode*)(*iter).second)->data;
    data.erase(iter);
    return ret;
  }
  return 0;
}

/*!
 *Return the number of nodes currently in the dictionary.
 */
int HashDictionary::nodesNumber()
{
  return static_cast<int>(data.size());
}

/*!
 *Free the dictionary.
 */
void HashDictionary::free()
{
  map<int, sNode*>::iterator iter = data.begin(); 
  for( ; iter != data.end(); iter++)
  {
    delete (*iter).second;
  }
  data.clear();
}

/*!
 *Get the data for the node using the order position. First node has index 1.
 */
void *HashDictionary::getData(int order)
{
  int i;
  map<int, sNode*>::iterator iter;
  if(order == 0)
    return 0;
  iter = data.begin(); 
  for(i = 1; (i < order) && ( iter != data.end() ); i++);
  return (iter!=data.end() ? (*iter).second->data : 0);
}

/*!
 *Check if the dictionary is empty.
 */
int HashDictionary::isEmpty()
{
  return data.empty() ? 1 : 0;
}



/*!
 *Insert a new node at the beginning of the list. Returns zero on success.
 */
int HashDictionary::insert(const char* name,void* dataPtr)
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
void* HashDictionary::removeNodeAt(int order)
{
  map<int, sNode*>::iterator iter=data.begin();
  int pos = 1;
  void *ret = 0;
  while((pos<order) && (iter!=data.end()) )
  {
    pos++;
    iter++;
  }

  if(pos != order)
    return 0;

  ret = (*iter).second->data;

  data.erase(iter);
  return ret;
}
