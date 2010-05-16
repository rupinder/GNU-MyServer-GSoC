/*
 MyServer
 Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.
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
#include <string.h>
extern "C"
{
#include <stdlib.h>
}
#include "vector.h"

Vector::Vector()
{
   Array = NULL;
   VectorSize = 0;
   Sorted = false;
}

Vector::~Vector()
{
   clear();
}

VectorNode * Vector::add(const char * Text, void * Data)
{
   int i;

   VectorNode * NewNode = new VectorNode;
   NewNode->Text = strdup(Text);
   NewNode->Data = Data;
   NewNode->Number = VectorSize;

   VectorNode ** NewArray = (VectorNode **) new void*[sizeof(VectorNode *)*(VectorSize + 1)];
   for(i = 0; i < VectorSize; i++)
     NewArray[i] = Array[i];

   NewArray[i] = NewNode;
   delete Array;
   Array = NewArray;
   VectorSize++;
   Sorted = false;
   return NewNode;
}

void Vector::add(Vector & cpy)
{
   for(int i = 0; i < cpy.size(); i++)
     {
	add(cpy.at(i)->Text, cpy.at(i)->Data);
     }
}

void Vector::remove(int index)
{
   if(VectorSize == 0)
     return;
   if(index > VectorSize)
     return;

   if(VectorSize - 1 == 0)
     {
	clear();
	return;
     }

   VectorNode ** NewArray = (VectorNode **) new void*[sizeof(VectorNode *)*(VectorSize - 1)];

   int a, b;
   a = b = 0;
   for(a = 0; a < VectorSize; a++)
     {
	if(a == index)
	  {
	     DeleteNode(Array[a]);
	     a++;
	  }
	NewArray[b] = Array[a];
	b++;
     }
   delete Array;
   Array = NewArray;
   VectorSize--;
   ReCount();
}

void Vector::clear()
{
   if(VectorSize == 0)
     return;
   int i;
   for(i = 0; i < VectorSize; i++)
     DeleteNode(Array[i]);
   delete Array;
   Array = NULL;
   VectorSize = 0;
   Sorted = false;
}

int Vector::size()
{
   return VectorSize;
}

// q_sort example from Michael Lamont (GPL)
// http://linux.wku.edu/~lamonml/algor/sort/quick.c
//
// Modifed to work with Vector class
static void q_sort(VectorNode ** Array, int left, int right)
{

   int index, l_hold, r_hold;

   VectorNode * pivot;

   l_hold = left;
   r_hold = right;
   pivot = Array[left];
   while (left < right)
     {
	while (strcmp(Array[right]->Text, pivot->Text) >= 0 && (left < right))
	  right--;
	if (left != right)
	  {
	     Array[left] = Array[right];
	     left++;
	  }
	while (strcmp(Array[left]->Text, pivot->Text) <= 0 && (left < right))
	  left++;
	if (left != right)
	  {
	     Array[right] = Array[left];
	     right--;
	  }
     }
   Array[left] = pivot;
   index = left;
   left = l_hold;
   right = r_hold;
   if (left < index)
     q_sort(Array, left, index-1);
   if (right > index)
     q_sort(Array, index+1, right);
}

// sort using QuickSort
void Vector::sort()
{
   if(Sorted)
     return;
   if(VectorSize < 2)
     return;

   q_sort(Array, 0, VectorSize - 1);

   ReCount();
   Sorted = true;
}

VectorNode * Vector::at(int index)
{
   if(index >= VectorSize)
     return NULL;
   return Array[index];
}

// sorted binary search
int Vector::get(const char * Text)
{
   if(VectorSize == 0)
     return -1;
   if(!Sorted)
     sort();

   int high, low, center, ret;
   high = VectorSize - 1;
   low = 0;

   while(low <= high)
     {
	center = (low + high) / 2;
	ret = strcmp(Text, Array[center]->Text);
	if(ret == 0)
	  return center;
	else if(ret < 0)
	  high = center - 1;
	else
	  low = center + 1;
     }

   return -1;
}

bool Vector::isempty()
{
   if(VectorSize == 0)
     return true;
   else
     return false;
}

void Vector::DeleteNode(VectorNode * Node)
{
   free(Node->Text);
   delete Node;
}

void Vector::ReCount()
{
   for(int i = 0; i < VectorSize; i++)
     {
	Array[i]->Number = i;
     }
}

