/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2008, 2009, 2010 Free Software Foundation, Inc.
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

#ifndef XML_VALIDATOR_H
# define XML_VALIDATOR_H

# include "myserver.h"
# include <include/base/hash_map/hash_map.h>

# include <include/conf/security/security_cache.h>

# include <include/conf/security/security_domain.h>
# include <include/conf/security/security_manager.h>
# include <include/server/server.h>
# include <include/base/sync/mutex.h>
# include <include/conf/security/validator.h>
# include <include/base/xml/xml_parser.h>

# include <include/base/crypt/crypt_algo_manager.h>

class XmlValidator : public Validator, public AuthMethod
{
public:
  XmlValidator ();
  virtual ~XmlValidator ();

  using Validator::getPermissionMask;

  virtual int getPermissionMask (SecurityToken *st);

  virtual int
  getPermissionMaskImpl (SecurityToken *st,
                         HashMap<string, SecurityDomain*> *hashedDomains,
                         AuthMethod* authMethod);
private:
  SecurityCache::CacheNode *getParser (SecurityToken *st);
  bool doCondition (xmlNodePtr node,
                    HashMap<string, SecurityDomain*> *hashedDomains);

  void doReturn (xmlNodePtr node,
                 int *cmd,
                 HashMap<string, SecurityDomain*> *hashedDomains);

  void doDefine (xmlNodePtr node,
                 SecurityToken *st,
                 HashMap<string, SecurityDomain*> *hashedDomains);

  void doSetHeader (xmlNodePtr node,
                    SecurityToken *st);

  void doPermission (xmlNodePtr node,
                     SecurityToken *st,
                     HashMap<string, SecurityDomain*> *hashedDomains);

  int computeXmlNode (xmlNodePtr node,
                      SecurityToken *st,
                      int *cmd,
                      HashMap<string, SecurityDomain*> *hashedDomains);

  int getPermissions (xmlAttr *attrs, xmlChar **user = NULL,
                      xmlChar **password = NULL, xmlChar **algorithm = NULL);

  SecurityCache *getCache (SecurityToken*);
  SecurityCache *secCache;
  Mutex cacheMutex;
};

#endif
