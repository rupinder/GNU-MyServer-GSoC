/*
 *  *MyServer
 *  *Copyright (C) 2002,2003,2004 The MyServer Team
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef VHOST_H_CONF
#define VHOST_H_CONF

#include <FL/Fl.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Choice.H>

#include "../include/vector.h"
#include "../include/MemBuf.h"

enum 
{
   PROTOCOL_HTTP = 0,
   PROTOCOL_HTTPS,
   PROTOCOL_FTP,
   PROTOCOL_CONTROL,
   PROTOCOL_DYNAMIC
};

extern const char * EMPTY;

struct vHostNode
{
   Vector Host;
   Vector Ip;
   int Port;
   int Protocol;
   char * Ssl_Privatekey;
   char * Ssl_Certificate;
   char * Ssl_Password;
   char * Docroot;
   char * Sysfolder;
   char * Accesseslog;
   char * Warninglog;
};

class vHostXML
{
 public:
   ~vHostXML();
   void clear();
   int load(const char *);
   int loadMemBuf(CMemBuf &);
   int load_core(XmlParser &);
   int save(const char *);
   int saveMemBuf(CMemBuf &);
   int save_core(XmlParser &);
   void populateName(Fl_Choice *);
   void populateHost(int, Fl_Browser *);
   void populateIp(int, Fl_Browser *);
   int addName(const char *);
   int addHost(int, const char *, bool);
   int addIp(int, const char *, bool);
   void removeName(int);
   void removeHost(int, int);
   void removeIp(int, int);
   void setPort(int, int);
   void setProtocol(int, int);
   void setSsl_Privatekey(int, const char *);
   void setSsl_Certificate(int, const char *);
   void setSsl_Password(int, const char *);
   void setDocroot(int, const char *);
   void setSysfolder(int, const char *);
   void setAccesseslog(int, const char *);
   void setWarninglog(int, const char *);
   int getPort(int);
   int getProtocol(int);
   const char * getSsl_Privatekey(int);
   const char * getSsl_Certificate(int);
   const char * getSsl_Password(int);
   const char * getDocroot(int);
   const char * getSysfolder(int);
   const char * getAccesseslog(int);
   const char * getWarninglog(int);
   /* Dynamic protocals */
   void populateProtocol(Fl_Choice *);
   void loadProtocols(Vector &);
 private :
   void DeletevHostNode(vHostNode *);
   Vector vHosts;
   Vector Dynamic;
};

#endif
