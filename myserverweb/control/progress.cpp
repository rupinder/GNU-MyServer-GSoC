/*
  *MyServer
  *Copyright (C) 2002,2003,2004,2005 The MyServer Team
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
 
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
 
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef PROGRESS_H
#define PROGRESS_H

#include <FL/Fl.H>
#include <FL/Fl_Progress.H>
#include <stdio.h>
/* Status callback function */

void ProgressCallback(void * Object, unsigned int len, unsigned int pos)
{
   Fl_Progress * o = (Fl_Progress *)Object;  
   static char buffer[5];
   int stat = (int)((float)pos / (float)len * 100.0);
   snprintf(buffer, 5, "%d%%", stat);
   o->value(stat);
   o->label(buffer);
   // make sure fltk updates the display
   Fl::wait(0);Fl::wait(0);Fl::wait(0);Fl::wait(0);Fl::wait(0);Fl::wait(0);
//   Fl::wait(0);Fl::wait(0);Fl::wait(0);Fl::wait(0);Fl::wait(0);Fl::wait(0);
//   Fl::wait(0);Fl::wait(0);Fl::wait(0);Fl::wait(0);Fl::wait(0);Fl::wait(0);
}

#endif
