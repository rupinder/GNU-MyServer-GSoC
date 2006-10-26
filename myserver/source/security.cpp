/*
MyServer
Copyright (C) 2002, 2003, 2004, 2005, 2006 The MyServer Team
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "../include/security.h"
#include "../include/utility.h"
#include "../include/http_constants.h"
#include "../include/xml_parser.h"
#include "../include/connection.h"
#include "../include/securestr.h"
#include "../include/myserver_regex.h"

#include <string>
#include <sstream>

#ifdef WIN32
#ifndef LOGON32_LOGON_NETWORK
#define LOGON32_LOGON_NETWORK 3
#endif

#ifndef LOGON32_PROVIDER_DEFAULT
#define LOGON32_PROVIDER_DEFAULT
#endif
#endif

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
  neededPassword = 0;
  permission2    = 0;
  authType      = 0;
  authTypeLen       = 0;
  throttlingRate = (u_long)-1;
}

/*!
 *Get the error file for a site. 
 *Return 0 to use the default one.
 *Return -1 on errors.
 *Return other valus on success.
 */
int SecurityManager::getErrorFileName(const char* sysDir,int error, 
                                      string &out, XmlParser* parser)
{
	xmlNode *node;
	ostringstream permissionsFile;
	XmlParser localParser;  
  xmlDocPtr doc;
	int found=0;
  out.assign("");
  if(parser == 0)
  { 
    permissionsFile << sysDir << "/security" ;
    if(!File::fileExists(permissionsFile.str().c_str()))
    {
      return 0;
    }
    if(localParser.open(permissionsFile.str().c_str()) == -1 )
      return -1;
    doc=localParser.getDoc();
  }
  else
  {
    doc=parser->getDoc();
  }

  if(doc == 0)
	{
		if(parser == 0)
			localParser.close();
    return 0;
	}
  node=doc->children->children;

  if(node == 0)
	{
		if(parser == 0)
			localParser.close();
    return 0;
	}

	while(node)
	{
		if(!xmlStrcmp(node->name, (const xmlChar *)"ERROR"))
		{
			xmlAttr *attr =  node->properties;
      char *fileName = 0;
			while(attr)
			{
				if(!xmlStrcmp(attr->name, (const xmlChar *)"FILE"))
        {
          fileName = (char*)attr->children->content;

          /* The error ID is correct.  */
          if(found)
          {
            out.assign(fileName);
          }
				}
				if(!xmlStrcmp(attr->name, (const xmlChar *)"ID"))
				{
					int errorId;
          errorId = atoi((const char*)attr->children->content);
					if(errorId == error)
						found = 1;
          else
            continue;
          /* The file name was still specified.  */
          if(fileName)
          {
            out.assign(fileName);
          }
				}
				attr = attr->next;
			}
			if(found)
				break;
		}
		node = node->next;
	}
  if(parser == 0)
    localParser.close();
  
  /* Return 1 if both it was found and well configured.  */
  if(found && out.length())
    return 1;
  
  return 0;

}


/*!
 *Get the permissions mask for the file[filename]. 
 *The file [directory]/security will be parsed. If a [parser] is specified, 
 *it will be used instead of opening the security file.
 *[permission2] is the permission mask that the [user] will have 
 *if providing a [neededPassword].
 *Returns -1 on errors.
 */
int SecurityManager::getPermissionMask(SecurityToken *st, XmlParser* parser)
{

	ostringstream permissionsFile;

	char tempPassword[32];

  /* Generic permission data mask for the user.  */
	int genericPermissions = 0;
	int genericPermissionsFound = 0;

  /* Permission data for the file.  */
	int filePermissions = 0;
	int filePermissionsFound = 0;

  /* Permission data for the user and the file.  */
	int userPermissions = 0;
	int userPermissionsFound = 0;

  /* Store what we found for neededPassword.  */
	int filePermissions2Found = 0;
	int userPermissions2Found = 0;
	int genericPermissions2Found = 0;

  u_long tempThrottlingRate = (u_long)-1;
  xmlAttr *attr;
	xmlNode *node;

	XmlParser localParser;
  xmlDocPtr doc;
  
  /*
   *Store where actions are found. 
   *0 Not Found.
   *1 Globally.
   *2 User.
   *3 Item.
   *4 Item + User.
   */
	int actionsFound = 0;
	int tmpActionsFound = 0;
 	xmlNode *actionsNode = 0;
 	xmlNode *tmpActionsNode = 0;

	tempPassword[0] = '\0';
	if(st && st->authType)
		st->authType[0] = '\0';
  if(st->user == 0)
    return -1;
  if(st->directory == 0)
    return -1;
  if(st->filename == 0)
    return -1;

  /* 
   *If there is not specified the parser to use, create a new parser
   *object and initialize it. 
   */
  if(parser == 0)
  {
    permissionsFile << st->directory << "/security";

    /* If the specified file doesn't exist return 0.  */
    if(!File::fileExists(permissionsFile.str().c_str()))
    {
      return 0;
    }
    else
    {
      if(localParser.open(permissionsFile.str().c_str()) == -1)
      {
        return -1;
      }
      doc = localParser.getDoc();
    }
  }
  else
  {
    /* If the parser object is specified get the doc from it.  */
    doc = parser->getDoc();
  }

  if(!doc)
	{
		if(parser == 0)
			localParser.close();
    return -1;
	}

  /*
   *If the file is not valid, returns 0.
   *Clean the parser object if was created here.
   */
  if(doc->children && doc->children->children)
    node = doc->children->children;
  else if(parser == 0)
  {
    localParser.close();
    return -1;
  }

	while(node)
	{
    tempThrottlingRate = (u_long)-1;

    /* Retrieve the authorization scheme to use if specified.  */
		if(!xmlStrcmp(node->name, (const xmlChar *)"AUTH"))
		{
			attr = node->properties;
			while(attr)
			{
				if(!xmlStrcmp(attr->name, (const xmlChar *)"TYPE"))
				{
					if(st && st->authType)
						strncpy(st->authType,(const char*)attr->children->content, 
                    st->authTypeLen);
				}
				attr = attr->next;
			}
		}
		else if(!xmlStrcmp(node->name, (const xmlChar *)"ACTION"))
		{
	    if(actionsFound < 1)
	    {
        actionsFound = 1;
 	      actionsNode = doc->children->children;            
      }
    }

    /* USER block.  */
		else if(!xmlStrcmp(node->name, (const xmlChar *)"USER"))
		{
			int tempGenericPermissions = 0;
			int rightUser = 0;
			int rightPassword = 0;
			xmlNode *node2 = node->children;
      attr = node->properties;
      tmpActionsFound = 0;      
      while(node2)
      {
        if(!xmlStrcmp(node2->name, (const xmlChar *)"ACTION"))
        {
 	        if(actionsFound < 2)
	        {
            tmpActionsFound = 2;
 	          tmpActionsNode = node->children;  
          }                                   
        }           
        node2 = node2->next;            
      }
      
      while(attr)
			{
				if(!xmlStrcmp(attr->name, (const xmlChar *)"READ"))
				{
					if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
						tempGenericPermissions |= MYSERVER_PERMISSION_READ;
				}
				else if(!xmlStrcmp(attr->name, (const xmlChar *)"WRITE"))
				{
					if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
						tempGenericPermissions |= MYSERVER_PERMISSION_WRITE;
				}
				else if(!xmlStrcmp(attr->name, (const xmlChar *)"BROWSE"))
				{
					if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
						tempGenericPermissions |= MYSERVER_PERMISSION_BROWSE;
				}
				else if(!xmlStrcmp(attr->name, (const xmlChar *)"EXECUTE"))
				{
					if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
						tempGenericPermissions |= MYSERVER_PERMISSION_EXECUTE;
				}
				else if(!xmlStrcmp(attr->name, (const xmlChar *)"DELETE"))
				{
					if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
						tempGenericPermissions |= MYSERVER_PERMISSION_DELETE;
				}
				else if(!lstrcmpi((const char*)attr->name,"NAME"))
				{
					if(!lstrcmpi((const char*)attr->children->content, st->user))
						rightUser = 1;
				}
				else if(!xmlStrcmp(attr->name, (const xmlChar *)"PASS"))
				{
          myserver_strlcpy(tempPassword,(char*)attr->children->content, 32);
          /* If a password is provided check that it is valid.  */
					if(st->password && (!xmlStrcmp(attr->children->content, 
                                         (const xmlChar *)st->password)) )
						rightPassword = 1;
				}
        else if(!xmlStrcmp(attr->name, (const xmlChar *)"THROTTLING_RATE"))
				{
          if((tempThrottlingRate == (u_long)-1) || 
             ((userPermissionsFound == 0) && (filePermissionsFound == 0)))
          tempThrottlingRate = (u_long)atoi((char*)attr->children->content);
				}
        /*  
         *USER is the weakest permission considered. Be sure that no others are
         *specified before save objects in the security token object.
         */
				if(rightUser && (filePermissionsFound == 0) && 
					 (userPermissionsFound == 0))
				{
 	        if(tmpActionsFound == 2)
	        {
            actionsFound = 2;
 	          actionsNode = tmpActionsNode;            
          }   
          if(st->neededPassword)
            strncpy(st->neededPassword, tempPassword, 32);
          if(tempThrottlingRate != (u_long) -1) 
            st->throttlingRate = tempThrottlingRate;
        }
				attr = attr->next;
			}
			if(rightUser)
			{
				if(rightPassword)
					genericPermissionsFound = 1;
				genericPermissions2Found = 1;
				genericPermissions = tempGenericPermissions;
			}

		}
    /* ITEM block.  */
		else if(!xmlStrcmp(node->name, (const xmlChar *)"ITEM"))
		{
      int tempFilePermissions;
      xmlNode *node2 = node->children;
      tempThrottlingRate = (u_long)-1;
      tempFilePermissions = 0;
      tmpActionsFound = 0;
			while(node2)
			{
        if(!xmlStrcmp(node2->name, (const xmlChar *)"ACTION"))
        {
 	        if(actionsFound <= 3)
	        {
            tmpActionsFound = 3;
 	          tmpActionsNode = node->children;
          }                                   
        }      
        if(!xmlStrcmp(node2->name, (const xmlChar *)"USER"))
				{
					int tempUserPermissions = 0;
					int rightUser= 0;
					int rightPassword = 0;
					attr = node2->properties;
          xmlNode *node3 = node2->children;
          while(node3)
          {
            if(!xmlStrcmp(node3->name, (const xmlChar *)"ACTION"))
            {
              tmpActionsFound = 4;
              tmpActionsNode = node2->children;            
            }
            node3 = node3->next;                              
          }    
					while(attr)
					{
						if(!xmlStrcmp(attr->name, (const xmlChar *)"READ"))
						{
							if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
								tempUserPermissions |= MYSERVER_PERMISSION_READ;
						}
						else if(!xmlStrcmp(attr->name, (const xmlChar *)"WRITE"))
						{
							if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
								tempUserPermissions |= MYSERVER_PERMISSION_WRITE;
						}
						else if(!xmlStrcmp(attr->name, (const xmlChar *)"EXECUTE"))
						{
							if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
								tempUserPermissions |= MYSERVER_PERMISSION_EXECUTE;
						}
						else if(!xmlStrcmp(attr->name, (const xmlChar *)"DELETE"))
						{
							if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
								tempUserPermissions |= MYSERVER_PERMISSION_DELETE;
						}
						else if(!lstrcmpi((const char*)attr->name,"NAME"))
						{
							if(!lstrcmpi((const char*)attr->children->content, st->user))
								rightUser = 1;
						}
						else if(!xmlStrcmp(attr->name, (const xmlChar *)"PASS"))
						{
							myserver_strlcpy(tempPassword, (char*)attr->children->content, 
															 32);

							if(st->password && 
                 (!lstrcmp((const char*)attr->children->content, 
													 st->password)))
								rightPassword = 1;
						}
            else if(!xmlStrcmp(attr->name, (const xmlChar *)"THROTTLING_RATE"))
            {
              tempThrottlingRate = (u_long)atoi((char*)
																								attr->children->content);
            }
            /*
             *USER inside ITEM is the strongest mask considered. 
             *Do not check for other masks to save it.
             */
						if(rightUser)
						{
              if(st->neededPassword)
                myserver_strlcpy(st->neededPassword, tempPassword, 32);
						}

						attr = attr->next;
					}
					if(rightUser) 
					{
						if(rightPassword)
            {
							userPermissionsFound = 2;
              if(tempThrottlingRate != (u_long) -1) 
                st->throttlingRate = (u_long)tempThrottlingRate;
            }
						userPermissions2Found = 2;
						userPermissions = tempUserPermissions;
					}
				}
				node2 = node2->next;
			}


      {
        attr = node->properties;
        tempThrottlingRate = (u_long)-1;
        /* Generic ITEM permissions.  */
        while(attr)
        {
          if(!xmlStrcmp(attr->name, (const xmlChar *)"READ"))
          {
            if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
              tempFilePermissions |= MYSERVER_PERMISSION_READ;
          }
          else if(!xmlStrcmp(attr->name, (const xmlChar *)"WRITE"))
          {
            if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
              tempFilePermissions |= MYSERVER_PERMISSION_WRITE;
          }
          else if(!xmlStrcmp(attr->name, (const xmlChar *)"EXECUTE"))
          {
            if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
              tempFilePermissions |= MYSERVER_PERMISSION_EXECUTE;
          }
          else if(!xmlStrcmp(attr->name, (const xmlChar *)"DELETE"))
          {
            if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
              tempFilePermissions |= MYSERVER_PERMISSION_DELETE;
          }
          else if(!xmlStrcmp(attr->name, (const xmlChar *)"THROTTLING_RATE"))
          {
            if((tempThrottlingRate == (u_long)-1) || 
							 (userPermissionsFound == 0))
              tempThrottlingRate = (u_long)atoi(
																						(char*)attr->children->content);
          }
          /* Check if the file name is correct.  */
          if(!xmlStrcmp(attr->name, (const xmlChar *)"FILE"))
          {
            if(attr->children && attr->children->content &&
               (!xmlStrcmp(attr->children->content, 
                           (const xmlChar *)st->filename)))
            {					
              filePermissionsFound = 1;
              filePermissions2Found = 1;
              if(userPermissionsFound == 2)
                userPermissionsFound = 1;
              if(userPermissions2Found == 2)
                userPermissions2Found = 1;
                
              if(actionsFound < tmpActionsFound)
              {
                actionsFound = tmpActionsFound;
                actionsNode = tmpActionsNode;
              }
            }
          }
          attr = attr->next;
        }/* End attributes loop.  */

        /* 
         *Check that was not specified a file permission mask 
         *before overwrite these items.
         */
        if(filePermissionsFound && (userPermissionsFound==0))
				{
          if(tempThrottlingRate != (u_long) -1)
            st->throttlingRate = tempThrottlingRate;
        }

      }/* End generic ITEM attributes.  */

			if(filePermissionsFound)
				filePermissions = tempFilePermissions;
		
		}
		else if(node->children && node->children->content && st->otherValues)
		{
			string* val = new string((char*)node->children->content);
			string name((char*)node->name);
			string* old = st->td->other.put(name, val);
			/* Remove the old stored object.  */
			if(old)
				delete old;
		}
		node = node->next;
	}

  if(parser == 0)
  {
    localParser.close();
  }

  if(st->permission2)
	{
		*st->permission2 = 0;
		if(genericPermissions2Found)
		{
			*st->permission2 = genericPermissions;
		}
	
		if(filePermissions2Found == 1)
		{
			*st->permission2 = filePermissions;
		}
				
		if(userPermissions2Found == 1)
		{
			*st->permission2 = userPermissions;
		}

	}

  for( ;actionsNode; actionsNode = actionsNode->next)
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
      headerVal = st->td->request.getValue(name, 0);
    if(!headerVal)
      continue;

    /*
		 *If the regular expression matches the header value then deny the 
		 *access. 
		 */
    if(value.isCompiled() && !value.exec(headerVal->c_str(), 1,&pm, 
																				 REG_NOTEOL))
      return 0;
    else
      break;
  }

	if(userPermissionsFound == 1)
		return userPermissions;

	if(filePermissionsFound == 1)
		return filePermissions;

	if(genericPermissionsFound == 1)
		return genericPermissions;

	return 0;
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
