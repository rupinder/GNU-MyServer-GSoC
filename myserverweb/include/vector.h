/*
 *MyServer
 *Copyright (C) 2002,2003,2004 The MyServer Team
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
#ifndef VECTOR_H_SOME_GARBAGE_HERE
#define VECTOR_H_SOME_GARBAGE_HERE


struct VectorNode
{
   char * Text;
   int Number;
   void * Data;
};


// Since VC++'s STL is buggy...
class Vector
{
 public:
   Vector();
   ~Vector();
   VectorNode * add(const char * Text, void * Data = NULL);
   void add(Vector &);
   void remove(int );  // you must free/delete "Data"
   void clear();       // before calling
   int size();
   void sort();  // Sort A-z using "Text"
   VectorNode * at(int );  // Returns NULL on out of bounds
   int get(const char *);  // Returns index of text search or -1 on failure
   bool isempty();
 private:
   void DeleteNode(VectorNode *);
   void ReCount();
   VectorNode ** Array;
   int VectorSize;
   bool Sorted;
};

#endif
