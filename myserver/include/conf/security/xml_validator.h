/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2008 Free Software Foundation, Inc.
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
#define XML_VALIDATOR_H

#include "stdafx.h"
#include <include/base/hash_map/hash_map.h>

#include <include/conf/security/security_domain.h>
#include <include/conf/security/security_manager.h>
#include <include/server/server.h>
#include <include/base/sync/mutex.h>
#include <include/conf/security/validator.h>
#include <include/base/xml/xml_parser.h>

class SecurityCache;

class XmlValidator : public Validator, public AuthMethod
{
public:
  XmlValidator ();
  virtual ~XmlValidator ();

  using Validator::getPermissionMask;

  virtual int getPermissionMask (SecurityToken* st);

  virtual int getPermissionMaskImpl (SecurityToken* st,
                                     HashMap<string, SecurityDomain*> *hashedDomains,
                                     AuthMethod* authMethod);

private:
  XmlParser* getParser(SecurityToken* st);
  bool doCondition (xmlNodePtr node, 
                    HashMap<string, SecurityDomain*> *hashedDomains);

  void doReturn (xmlNodePtr node, 
                 int *cmd, 
                 HashMap<string, SecurityDomain*> *hashedDomains);

  void doDefine (xmlNodePtr node,
                 SecurityToken *st, 
                 HashMap<string, SecurityDomain*> *hashedDomains);

  void doPermission (xmlNodePtr node,
                     SecurityToken *st, 
                     HashMap<string, SecurityDomain*> *hashedDomains);

  int computeXmlNode (xmlNodePtr node, 
                      SecurityToken *st, 
                      int *cmd, 
                      HashMap<string, SecurityDomain*> *hashedDomains);

  int getPermissions (xmlAttr* attrs, xmlChar** user = NULL, xmlChar** password = NULL);

  SecurityCache *getCache(Server*);
  SecurityCache *secCache;
  Mutex cacheMutex;
};

#endif
