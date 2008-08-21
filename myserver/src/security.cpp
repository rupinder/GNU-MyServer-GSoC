/*
MyServer
Copyright (C) 2002-2008 Free Software Foundation, Inc.
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


#include "../include/security.h"
#include "../include/utility.h"
#include "../include/xml_parser.h"
#include "../include/connection.h"
#include "../include/securestr.h"
#include "../include/myserver_regex.h"
#include "../include/files_utility.h"
#include "../include/http_thread_context.h"

#include <string>
#include <sstream>
#include <memory>

using namespace std;

/*!
 *Create the object.
 */
SecurityToken::SecurityToken()
{
  reset();
}

/*!
 *Reset every structure member.
 */
void SecurityToken::reset()
{
  user           = 0;
  password       = 0;
  directory      = 0;
  filename       = 0;
  requiredPassword = 0;
  providedMask    = 0;
  authType      = 0;
  authTypeLen       = 0;
  throttlingRate = (int)-1;
}


/*!
 *Get the error file for a page using the specified parser. 
 *Return 0 to use the default one.
 *Return -1 on errors.
 *Return other valus on success.
 */
int SecurityManager::getErrorFileName(const char* sysDir, 
                                      int error, 
                                      string &out, 
                                      XmlParser* parser)
{
  string evalString;
  XmlXPathResult* xpathRes;
  xmlNodeSetPtr nodes;
  int ret;

  out.assign("");

  if(parser == NULL || !parser->isXpathEnabled())
    return -1;

  evalString = "/SECURITY/ERROR[@ID=\'";
  evalString += error;
  evalString += "']/@FILE";

  xpathRes = parser->evaluateXpath(evalString);
  nodes = xpathRes->getNodeSet();

  if(nodes && nodes->nodeNr)
    out.assign((const char*)nodes->nodeTab[0]->children->content);

  /* Return 1 if both it was found and well configured.  */
  ret = nodes && nodes->nodeNr && out.length() ? 1 : 0;

  delete xpathRes;
  return ret;

}


/*!
 *Get the permissions mask for the file FILENAME using the XML parser PARSER.
 *The file DIRECTORY/security will be parsed.
 *PROVIDEDMASK is the permission mask that the USER will have providing the
 *REQUIREDPASSWORD password.
 *Returns -1 on errors.
 */
int SecurityManager::getPermissionMask(SecurityToken *st, XmlParser* parser)
{
  xmlNodeSetPtr nodes;
  xmlAttr* attr;
  string evalString;
  int permissions = 0;
  const char* requiredPassword;
  bool rightPassword = false;
  auto_ptr<XmlXPathResult> itemRes;
  auto_ptr<XmlXPathResult> userRes;

  if(parser == NULL || !parser->isXpathEnabled())
    return -1;


  evalString = "/SECURITY/AUTH/@TYPE";

  auto_ptr<XmlXPathResult>authRes(parser->evaluateXpath(evalString));
  nodes = authRes.get()->getNodeSet();

  if(nodes && nodes->nodeNr)
    strncpy(st->authType,(const char*)nodes->nodeTab[0]->children->content, 
            st->authTypeLen);

  evalString = "/SECURITY/ITEM[@FILE=\'";
  evalString += st->filename;
  evalString += "\']/USER[@NAME=\'";
  evalString += st->user;
  evalString += "\']/.";

  auto_ptr<XmlXPathResult> itemUserRes(parser->evaluateXpath(evalString));

  nodes = itemUserRes.get()->getNodeSet();

  if(!nodes || !nodes->nodeNr)
  {
    evalString = "/SECURITY/ITEM[@FILE=\'";
    evalString += st->filename;
    evalString += "\']/.";

    itemRes.reset(parser->evaluateXpath(evalString));

    nodes = itemRes.get()->getNodeSet();

    if(!nodes || !nodes->nodeNr)
    {
      evalString = "/SECURITY/USER[@NAME=\'";
      evalString += st->user;
      evalString += "\']/.";

      userRes.reset(parser->evaluateXpath(evalString));

      nodes = userRes.get()->getNodeSet();
    }

  }

  if(!nodes || !nodes->nodeNr)
    return 0;

  for(attr = nodes->nodeTab[0]->properties; attr; attr = attr->next)
  {
    if(!strcmpi((const char*)attr->name, "READ") && 
       !strcmpi((const char*)attr->children->content, "TRUE"))
      permissions |= MYSERVER_PERMISSION_READ;

    if(!strcmpi((const char*)attr->name, "WRITE") && 
       !strcmpi((const char*)attr->children->content, "TRUE"))
      permissions |= MYSERVER_PERMISSION_WRITE;

    if(!strcmpi((const char*)attr->name, "EXECUTE") && 
       !strcmpi((const char*)attr->children->content, "TRUE"))
      permissions |= MYSERVER_PERMISSION_EXECUTE;

    if(!strcmpi((const char*)attr->name, "BROWSE") && 
       !strcmpi((const char*)attr->children->content, "TRUE"))
      permissions |= MYSERVER_PERMISSION_BROWSE;

    if(!strcmpi((const char*)attr->name, "PASS"))
    {
      requiredPassword = (const char*)attr->children->content;
      rightPassword = !strcmp(st->password, requiredPassword);
    }
  }

  if(rightPassword)
  {
    for(attr = nodes->nodeTab[0]->properties; attr; attr = attr->next)
    {
      if(!strcmpi((const char*)attr->name, "THROTTLING_RATE"))
        st->throttlingRate = atoi((const char*)attr->children->content);
    }
  }

  if(st->requiredPassword)
    myserver_strlcpy(st->requiredPassword, requiredPassword, 32);

  if(st->providedMask)
    *(st->providedMask) = permissions;

  if(!SecurityManager::checkActions(st->td, nodes->nodeTab[0] ))
    return 0;

  
  return rightPassword ? permissions : 0;
}

/*!
 *Check if the specified actions deny the access to the resource.
 *\param td The Thread Context.
 *\param root The root node with actions.
 *\return true if the action allows the access to the resource.
 *\return false if the action denies the access to the resource.
 */
bool SecurityManager::checkActions(HttpThreadContext* td,  xmlNode *root)
{
  xmlNode* actionsNode = root;

  for( ; td && actionsNode; actionsNode = actionsNode->next)
  {
    xmlAttr *attr = actionsNode->properties;
    int deny = 0;
    regmatch_t pm;
    const char* name = 0;
    Regex value;
    string* headerVal = 0;

    if(strcmpi((const char*)actionsNode->name, "ACTION"))
      continue;

    if(actionsNode->children && actionsNode->children->content 
       && !strcmpi((const char*)actionsNode->children->content, "DENY"))
         deny = 1;

    if(!deny)
      continue;

    for( ; attr; attr = attr->next)
    {
      if(!strcmpi((const char*)attr->name, "NAME"))
        name = (const char*) attr->children->content;
      if(!strcmpi((const char*)attr->name, "VALUE"))
        value.compile((const char*)attr->children->content, REG_EXTENDED);         
    }

    if(name)
      headerVal = td->request.getValue(name, 0);

    if(!headerVal)
      continue;

    /*
     *If the regular expression matches the header value then deny the 
     *access. 
     */
    if(value.isCompiled() && !value.exec(headerVal->c_str(), 1,&pm, 
                                         REG_NOTEOL))
      return false;
  }

  return true;

}

/*!
 *Create the object.
 */
SecurityManager::SecurityManager()
{

}

/*!
 *Destroy the SecurityManager object.
 */
SecurityManager::~SecurityManager()
{

}
