/*
 MyServer
 Copyright (C) 2002, 2003, 2004, 2007 The MyServer Team
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
#ifndef MIMETYPE_H_CONF
#define MIMETYPE_H_CONF

#include <FL/Fl.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Choice.H>

#include "vector.h"
#include "../include/mem_buff.h"
#include "../include/xml_parser.h"

///
/// CGI command type numbers.
///
enum
{
   CMD_SEND = 0,
   CMD_SENDLINK,
   CMD_RUNCGI,
   CMD_RUNFASTCGI,
   CMD_RUNSCGI,
   CMD_RUNISAPI,
   CMD_RUNMSCGI,
   CMD_EXECUTEWINCGI,
   CMD_EXECUTE,
   CMD_EXECUTEISAPI,
   CMD_EXECUTEFASTCGI,
   CMD_EXECUTESCGI

};

///
/// A pointer used to spesify "NONE"
///
extern const char * NONE;

///
/// MIME type node.
/// This is used to store the MIME type data for each ext.
///
struct MimeNode
{
   VectorNode * Type;  /// Pointer to the MIME type.
   int Cmd;            /// The cgi command number.
   char * Manager;     /// The cgi manager string.
};

///
/// Loads and saves the MIME type configuration and manipulate the data.
/// 
class MIMEtypeXML
{
 public:
   ~MIMEtypeXML();
   void clear();
   int load(const char *);
   int loadMemBuf(MemBuf &);
   int load_core(XmlParser &);
   int save(const char *);
   int saveMemBuf(MemBuf &);
   int save_core(XmlParser &);
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
