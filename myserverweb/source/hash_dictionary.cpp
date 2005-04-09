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
  node = 0;
  nodes_count = 0;
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
  sNode* cur = node;
  while(cur)
  {
    if(cur->hash == nameHash)
      return cur->data;
    cur = cur->next;
  }

  return 0;

}

/*!
 *Add a node at the end of the list.
 */
int HashDictionary::append(const char* name, void* data)
{
  if(name == 0)
    return 0;
  sNode *newNode = new sNode();
  if(newNode == 0)
    return 0;
  newNode->hash = hash(name);
  newNode->data = data;
  newNode->next = 0;

  if(node == 0)
  {
    node = newNode;
    nodes_count++;
    return 1;
  }
  else
  {
    int ret = 2;
    sNode *cur = node;
    while(cur->next)
    {
      cur = cur->next;
      ret++;
    }
    cur->next = newNode;
    nodes_count++;
    return ret;
  }
  return 0;
}

/*!
 *Remove a node by name. The function returns the data for the removed node. If
 *there is not any node then zero is returned.
 */
void* HashDictionary::removeNode(const char* name)
{
  unsigned int node_hash = hash(name);
  sNode *cur = node;
  sNode *prev = 0;
  while(cur)
  {
    if(node_hash == cur->hash)
    {
      void *data;
      if(prev)
      {
        prev->next = cur->next;
      }
      else
      {
        node = cur->next;
      }
      data = cur->data;
      delete cur;
      nodes_count--;
      return data;
    }
    prev = cur;
    cur = cur->next;
  }
  return 0;
}

/*!
 *Return the number of nodes currently in the dictionary.
 */
int HashDictionary::nodesNumber()
{
  return nodes_count;
}

/*!
 *Free the dictionary.
 */
void HashDictionary::free()
{
  sNode *cur = node;
  while(cur)
  {
    sNode *toremove = cur;   
    cur = cur->next;
    delete toremove;
  }
  node = 0;
  nodes_count = 0;
}

/*!
 *Get the data for the node using the order position. First node has index 1.
 */
void *HashDictionary::getData(int order)
{
  int i;
  if(order == 0)
    return 0;
  sNode *cur = node;
  for(i = 1; (i < order) && ( cur ); i++)
  {
    cur = cur->next;
  }
  return (cur ? cur->data : 0);
}

/*!
 *Check if the dictionary is empty.
 */
int HashDictionary::isEmpty()
{
  return nodes_count ? 0 : 1;
}

/*!
 *Insert a node at the specified position.
 */
int HashDictionary::insertAt(const char* name, void* data, int pos)
{
  int i;
  sNode *cur;
  sNode *prev;
  if(name == 0)
    return -1;
  sNode *newNode = new sNode();
  if(newNode == 0)
    return -1;
  newNode->hash = hash(name);
  newNode->data = data;
  cur = node;
  prev = 0;
  for(i = 1; i < pos; i++)
  {
    if(cur == 0)
    {
      delete newNode;
      return -1;
    }
    prev = cur;
    cur = cur->next;
  }
  if(prev)
  {
    prev->next = newNode;
  }
  else
  {
    node = newNode;
  }
  newNode->next = cur;
  nodes_count++;
  return pos;
}

/*!
 *Insert a new node at the beginning of the list.
 */
int HashDictionary::insert(const char* name,void* data)
{
  if(name == 0)
    return -1;
  sNode *newNode = new sNode();
  if(newNode == 0)
    return -1;
  newNode->hash = hash(name);
  newNode->data = data;
  newNode->next = node;
  return  0;
}


/*!
 *Remove a node by its position.
 */
void* HashDictionary::removeNodeAt(int order)
{
  sNode *cur = node;
  sNode *prev = 0;
  int pos = 1;
  void *data = 0;
  while((pos<order) && cur)
  {
    pos++;
    prev = cur;
    cur = cur->next;    
  }
  if(pos != order)
    return 0;
  if(prev)
  {
    prev->next = cur->next;
  }
  else
  {
    node = cur->next;
  }
  data = cur->data;
  delete cur;
  nodes_count--;
  return data;
}
