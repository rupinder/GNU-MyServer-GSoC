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
#ifndef MIMETYPE_H_CONF
#define MIMETYPE_H_CONF

#include <FL/Fl.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Choice.H>

#include "../include/vector.h"

enum
{
   CMD_SEND = 0,
   CMD_SENDLINK,
   CMD_RUNCGI,
   CMD_RUNFASTCGI,
   CMD_RUNISAPI,
   CMD_RUNMSCGI,
   CMD_EXECUTEWINCGI,
   CMD_EXECUTE,
   CMD_EXECUTEISAPI,
   CMD_EXECUTEFASTCGI
};

extern const char * NONE;

struct MimeNode
{
   VectorNode * Type;
   int Cmd;
   char * Manager;
};

class MIMEtypeXML
{
 public:
   ~MIMEtypeXML();
   void clear();
   int load(const char *);
   int loadMemBuf(CMemBuf &);
   int load_core(cXMLParser &);
   int save(const char *);
   int saveMemBuf(CMemBuf &);
   int save_core(cXMLParser &);
   void populateExt(Fl_Browser *);
   void populateMime(Fl_Choice *);
   int addExt(const char *);
   int addMime(const char *);
   void removeExt(int );
   void setType(int, int);
   void setCmd(int, int);
   void setManager(int, const char *);
   int getType(int);
   int getCmd(int);
   const char * getManager(int);
 private:
   void ClearExt();
   void DeleteMimeNode(MimeNode *);
   Vector Mime;
   Vector Ext;
};

#endif
