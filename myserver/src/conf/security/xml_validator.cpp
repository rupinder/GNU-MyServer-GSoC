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


#include <include/conf/security/xml_validator.h>
#include <include/conf/security/auth_domain.h>
#include <include/conf/security/security_cache.h>
#include <include/base/regex/myserver_regex.h>

XmlValidator::XmlValidator ()
{
  secCache = NULL;
}

XmlValidator::~XmlValidator ()
{
  if (secCache != NULL)
  {
    secCache->free ();
    delete secCache;
  }
}

/*!
 *Get the security files cache.
 */
SecurityCache* XmlValidator::getCache (SecurityToken *st)
{
  if (!secCache)
  {
    const char *data = st->getHashedData ("SECURITY_CACHE_NODES", MYSERVER_SERVER_CONF, NULL);

    secCache = new SecurityCache ();

    if (data)
    {
      int nodes = atoi (data);
      secCache->setMaxNodes (nodes);
    }
  }

  return secCache;
}

/*!
 *Get the XML parser to use.
 */
XmlParser* XmlValidator::getParser (SecurityToken* st)
{
  const char *secName;

  SecurityCache *cache = getCache (st);

  if (!cache)
    return NULL;

  secName = st->getHashedData ("SECURITY_FILE_NAME", MYSERVER_VHOST_CONF | MYSERVER_SERVER_CONF, ".security.xml");

  return cache->getParser (*(st->getResource ()), *(st->getSysDirectory ()), false, secName);
}

/*!
 *\see AuthMethod#getPermissionMask.
 */
int XmlValidator::getPermissionMask (SecurityToken* st)
{
  xmlNodePtr root;
  XmlParser* xmlFile = getParser (st);

  if (!xmlFile)
    return 0;

  for (xmlNodePtr cur = xmlFile->getDoc ()->children; cur; cur = cur->next)
    if (cur->type == XML_ELEMENT_NODE)
    {
      for (xmlNodePtr curChild = cur->children; curChild; curChild = curChild->next)
        if (curChild->type == XML_ELEMENT_NODE)
        {
          root = curChild;
          break;
        }
    }

  for (xmlNodePtr cur = root; cur; cur = cur->next)
  {
    if (xmlStrcmp (cur->name, (const xmlChar *) "USER"))
      continue;
     
    xmlAttr *attrs = cur->properties;
  
    xmlChar* name = NULL;
    xmlChar* password = NULL;

    int permissions =  getPermissions (attrs, &name, &password);

    if (!name || !password || 
        xmlStrcmp (name, (const xmlChar *)st->getUser ().c_str ()))
      continue;

    st->setProvidedMask (permissions);

    if (xmlStrcmp (password, (const xmlChar *)st->getPassword ().c_str ()))
    {
      st->setAuthenticated (false);
      st->setMask (0);
    }
    else
    {
      st->setAuthenticated (true);
      st->setMask (permissions);
    }
    
    return st->getMask ();
  }

  return 0;
}

/*!
 *Get a permission mask from the attributes.
 *\param attrs Attributes list.
 *\param user The found user name.
 *\param password The found password.
 *\return the permissions mask.
 */
int XmlValidator::getPermissions (xmlAttr* attrs, xmlChar** user, xmlChar** password )
{
    int permissions = 0;

    while (attrs)
    {
      if (user && !xmlStrcmp (attrs->name, (const xmlChar *)"name") &&
          attrs->children && attrs->children->content)
        *user = attrs->children->content;
    
      else if (password && !xmlStrcmp (attrs->name, (const xmlChar *)"password") &&
          attrs->children && attrs->children->content)
        *password = attrs->children->content;

      else if (!xmlStrcmp (attrs->name, (const xmlChar *)"READ") &&
          attrs->children && attrs->children->content &&
          !xmlStrcmp(attrs->children->content, (const xmlChar *) "YES"))
        permissions |= MYSERVER_PERMISSION_READ;

      else if (!xmlStrcmp (attrs->name, (const xmlChar *)"WRITE") &&
          attrs->children && attrs->children->content &&
          !xmlStrcmp(attrs->children->content, (const xmlChar *) "YES"))
        permissions |= MYSERVER_PERMISSION_WRITE;

      else if (!xmlStrcmp (attrs->name, (const xmlChar *)"EXECUTE") &&
          attrs->children && attrs->children->content &&
          !xmlStrcmp(attrs->children->content, (const xmlChar *) "YES"))
        permissions |= MYSERVER_PERMISSION_EXECUTE;

      else if (!xmlStrcmp (attrs->name, (const xmlChar *)"BROWSE") &&
          attrs->children && attrs->children->content &&
          !xmlStrcmp(attrs->children->content, (const xmlChar *) "YES"))
        permissions |= MYSERVER_PERMISSION_BROWSE;
      
      attrs = attrs->next;
    }

    return permissions;
}


/*!
 *\see XmlValidator#getPermissionMaskImpl.
 */
int XmlValidator::getPermissionMaskImpl (SecurityToken* st,
                                         HashMap<string, SecurityDomain*> *hashedDomains,
                                         AuthMethod* authMethod)
{
  XmlParser* xmlFile = getParser (st);

  if (!xmlFile)
    return 0;

  for (xmlNodePtr cur = xmlFile->getDoc ()->children; cur; cur = cur->next)
    if (cur->type == XML_ELEMENT_NODE)
    {
      int cmd = -1;

      computeXmlNode (cur, st, &cmd, hashedDomains);

      /* By default return ALLOW.  */
      if (cmd == -1)
        return 1;

      if (cmd == 0)
        return 0;

      if (cmd == 1)
      {
        st->setMask (MYSERVER_PERMISSION_ALL);
        return 1;
      }

    }

  return 0;
}

/*!
 *Compute the current XML node.
 */
int XmlValidator::computeXmlNode (xmlNodePtr node, 
                                  SecurityToken *st, 
                                  int *cmd, 
                                  HashMap<string, SecurityDomain*> *hashedDomains)
{
  if (!node)
    return 0;

  xmlNodePtr cur = node->children;
  for (;;)
  {
    if (cur->next == NULL)
    {
      cur = cur->parent;

      /* The root is reached.  */
      if (cur == node)
        return 1;

      /* This should never happen.  */
      if (cur == NULL)
        return 0;
    }
    else
      cur = cur->next;

    if (cur->type != XML_ELEMENT_NODE)
      continue;
    
    if (!xmlStrcmp (cur->name, (const xmlChar *) "CONDITION"))
    {
      if (doCondition (cur, hashedDomains))
        cur = cur->children;
    }
    else if (!xmlStrcmp (cur->name, (const xmlChar *) "RETURN"))
    {
      doReturn (cur, cmd, hashedDomains);
      return 1;
    }
    else if (!xmlStrcmp (cur->name, (const xmlChar *) "DEFINE"))
    {
      doDefine (cur, st, hashedDomains);
    }
    else if (!xmlStrcmp (cur->name, (const xmlChar *) "PERMISSION"))
    {
      doPermission (cur, st, hashedDomains);
    }
  }

  return 0;
}

/*!
 *Handle a CONDITION.
 */
bool XmlValidator::doCondition (xmlNodePtr node, HashMap<string, SecurityDomain*> *hashedDomains)
{
  string name;
  const xmlChar *isNot = (const xmlChar*)"";
  const xmlChar *value = (const xmlChar*)"";
  const xmlChar *regex = (const xmlChar*)"";
  xmlAttr *attrs = node->properties;
  
  while (attrs)
  {
    if (!xmlStrcmp (attrs->name, (const xmlChar *)"name") &&
       attrs->children && attrs->children->content)
      name.assign ((const char*)attrs->children->content);
    
    if (!xmlStrcmp (attrs->name, (const xmlChar *)"value") &&
       attrs->children && attrs->children->content)
      value = attrs->children->content;

    if (!xmlStrcmp (attrs->name, (const xmlChar *)"not") &&
       attrs->children && attrs->children->content)
      isNot = attrs->children->content;

    if (!xmlStrcmp (attrs->name, (const xmlChar *)"regex") &&
       attrs->children && attrs->children->content)
      regex = attrs->children->content;
    
    attrs = attrs->next;
  }
      
  string *storedValue = getValue (hashedDomains, name);

  if (!storedValue)
    return false;

  bool eq;

  if (!xmlStrcmp (regex, (const xmlChar *) "yes"))
  {
    Regex regex;

    if (regex.compile ((const char*)value, REG_EXTENDED))
      return false;

    regmatch_t pm;

    eq = regex.exec (storedValue->c_str (), 1, &pm, 0) == 0;
  }
  else
    eq = storedValue->compare ((const char*)value) == 0;
  
  if (!xmlStrcmp (isNot, (const xmlChar *) "yes"))
    return !eq;
  
  return eq;
}

/*!
 *Handle a PERMISSION.
 */
void XmlValidator::doPermission (xmlNodePtr node, SecurityToken *st, HashMap<string, SecurityDomain*> *hashedDomains)
{
  string name;
  xmlAttr *attrs = node->properties;

  st->setProvidedMask (getPermissions (attrs));

  if (st->isAuthenticated ())
    st->setMask (st->getProvidedMask ());
}


/*!
 *Handle a DEFINE.
 */
void XmlValidator::doDefine (xmlNodePtr node, SecurityToken *st, HashMap<string, SecurityDomain*> *hashedDomains)
{
  string name;
  const xmlChar *value = (const xmlChar*)"";
  xmlAttr *attrs = node->properties;
  
  while (attrs)
  {
    if(!xmlStrcmp (attrs->name, (const xmlChar *)"name") &&
       attrs->children && attrs->children->content)
      name.assign ((const char*)attrs->children->content);
    
    if(!xmlStrcmp (attrs->name, (const xmlChar *)"value") &&
       attrs->children && attrs->children->content)
      value = attrs->children->content;

    attrs = attrs->next;
  }

  if (!value)
    return;

  string *valStr = new string ((const char *)value);

  string *old = st->getValues ()->put (name, valStr);

  if (old)
    delete old;
}

/*!
 *Handle a RETURN.
 */
void XmlValidator::doReturn (xmlNodePtr node, int *cmd, HashMap<string, SecurityDomain*> *hashedDomains)
{
  xmlAttr *attrs = node->properties;

  xmlChar *value = NULL;

  while (attrs)
  {
    if (!xmlStrcmp (attrs->name, (const xmlChar *)"value") &&
        attrs->children && attrs->children->content)
      value = attrs->children->content;
    
    attrs = attrs->next;
  }

  if (value && !xmlStrcmp (value, (const xmlChar *) "ALLOW"))
    *cmd = 1;
  else
    *cmd = 0;
}
