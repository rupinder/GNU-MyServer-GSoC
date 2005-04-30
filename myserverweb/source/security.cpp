/*
*MyServer
*Copyright (C) 2002, 2003, 2004, 2005 The MyServer Team
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


#include "../include/security.h"
#include "../include/utility.h"
#include "../include/HTTPmsg.h"
#include "../include/cXMLParser.h"
#include "../include/connectionstruct.h"
#include "../include/securestr.h"

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
  password2      = 0;
  permission2    = 0;
  auth_type      = 0;
  len_auth       = 0;
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
    return 0;

  node=doc->children->children;

  if(node == 0)
    return 0;

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

          /*! The error ID is correct. */
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
						found=1;
          else
            continue;
          /*! The file name was still specified. */
          if(fileName)
          {
            out.assign(fileName);
          }
				}
				attr=attr->next;
			}
			if(found)
				break;
		}
		node=node->next;
	}
  if(parser== 0)
    localParser.close();
  
  /*! Return 1 if both it was found and well configured. */
  if(found && out.length())
    return 1;
  
  return 0;

}

/*!
 *Get the permissions mask for the file[filename]. 
 *The file [directory]/security will be parsed. If a [parser] is specified, 
 *it will be used instead of opening the security file.
 *[permission2] is the permission mask that the [user] will have 
 *if providing a [password2].
 *Returns -1 on errors.
 */
int SecurityManager::getPermissionMask(SecurityToken *st, XmlParser* parser)
{

	ostringstream permissionsFile;

	char tempPassword[32];

  /*! Generic permission data mask for the user. */
	int genericPermissions=0;
	int genericPermissionsFound=0;

  /*! Permission data for the file. */
	int filePermissions=0;
	int filePermissionsFound=0;

  /*! Permission data for the user and the file. */
	int userPermissions=0;
	int userPermissionsFound=0;

  /*! Store what we found for password2. */
	int filePermissions2Found=0;
	int userPermissions2Found=0;
	int genericPermissions2Found=0;

  u_long tempThrottlingRate=(u_long)-1;
  xmlAttr *attr;
	xmlNode *node;

	XmlParser localParser;
  xmlDocPtr doc;

	tempPassword[0]='\0';
	if(st && st->auth_type)
		st->auth_type[0]='\0';
  if(st->user == 0)
    return -1;
  if(st->directory == 0)
    return -1;
  if(st->filename == 0)
    return -1;
  /*! 
   *If there is not specified the parser to use, create a new parser
   *object and initialize it. 
   */
  if(parser == 0)
  {
    permissionsFile << st->directory << "/security";

    /*! If the specified file doesn't exist return 0. */
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
      doc=localParser.getDoc();
    }
  }
  else
  {
    /*! If the parser object is specified get the doc from it. */
    doc=parser->getDoc();
  }

  if(!doc)
    return -1;

  /*! 
   *If the file is not valid, returns 0.
   *Clean the parser object if was created here.
   */
  if(doc->children && doc->children->children)
    node=doc->children->children;
  else if(parser == 0)
  {
    localParser.close();
    return -1;
  }

	while(node)
	{
    tempThrottlingRate = (u_long)-1;
    /*! Retrieve the authorization scheme to use if specified. */
		if(!xmlStrcmp(node->name, (const xmlChar *)"AUTH"))
		{
			attr = node->properties;
			while(attr)
			{
				if(!xmlStrcmp(attr->name, (const xmlChar *)"TYPE"))
				{
					if(st && st->auth_type)
						strncpy(st->auth_type,(const char*)attr->children->content, 
                    st->len_auth);
				}
				attr=attr->next;
			}
		}
    /*! USER block. */
		if(!xmlStrcmp(node->name, (const xmlChar *)"USER"))
		{
			int tempGenericPermissions=0;
			int rightUser=0;
			int rightPassword=0;
      attr =  node->properties;
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
						rightUser=1;
				}
				else if(!xmlStrcmp(attr->name, (const xmlChar *)"PASS"))
				{
          myserver_strlcpy(tempPassword,(char*)attr->children->content, 32);
          /*! If a password is provided check that it is valid. */
					if(st->password && (!xmlStrcmp(attr->children->content, 
                                         (const xmlChar *)st->password)) )
						rightPassword=1;
				}
        else if(!xmlStrcmp(attr->name, (const xmlChar *)"THROTTLING_RATE"))
				{
          if((tempThrottlingRate == (u_long)-1) || 
             ((userPermissionsFound==0)&&(filePermissionsFound==0)) )
          tempThrottlingRate = (u_long)atoi((char*)attr->children->content);
				}
        /*! 
         *USER is the weakest permission considered. Be sure that no others are
         *specified before save objects in the security token object.
         */
				if(rightUser && (filePermissionsFound==0) && (userPermissionsFound==0))
				{
          if(st->password2)
            strncpy(st->password2, tempPassword, 32);
          if(tempThrottlingRate != (u_long) -1) 
            st->throttlingRate = tempThrottlingRate;
        }
				attr=attr->next;
			}
			if(rightUser)
			{
				if(rightPassword)
					genericPermissionsFound=1;
				genericPermissions2Found=1;
				genericPermissions=tempGenericPermissions;
			}

		}
    /*! ITEM block. */
		if(!xmlStrcmp(node->name, (const xmlChar *)"ITEM"))
		{
      int tempFilePermissions;
      xmlNode *node2=node->children;
      tempThrottlingRate=(u_long)-1;
			while(node2)
			{
        tempFilePermissions=0;
        if(!xmlStrcmp(node2->name, (const xmlChar *)"USER"))
				{
					int tempUserPermissions=0;
					int rightUser=0;
					int rightPassword=0;
					attr = node2->properties;
					while(attr)
					{
						if(!xmlStrcmp(attr->name, (const xmlChar *)"READ"))
						{
							if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
								tempUserPermissions|=MYSERVER_PERMISSION_READ;
						}
						else if(!xmlStrcmp(attr->name, (const xmlChar *)"WRITE"))
						{
							if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
								tempUserPermissions|=MYSERVER_PERMISSION_WRITE;
						}
						else if(!xmlStrcmp(attr->name, (const xmlChar *)"EXECUTE"))
						{
							if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
								tempUserPermissions|=MYSERVER_PERMISSION_EXECUTE;
						}
						else if(!xmlStrcmp(attr->name, (const xmlChar *)"DELETE"))
						{
							if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
								tempUserPermissions|=MYSERVER_PERMISSION_DELETE;
						}
						else if(!lstrcmpi((const char*)attr->name,"NAME"))
						{
							if(!lstrcmpi((const char*)attr->children->content, st->user))
								rightUser=1;
						}
						else if(!xmlStrcmp(attr->name, (const xmlChar *)"PASS"))
						{
							myserver_strlcpy(tempPassword, (char*)attr->children->content, 32);
							if(st->password && 
                 (!lstrcmp((const char*)attr->children->content, st->password)))
								rightPassword=1;
						}
            else if(!xmlStrcmp(attr->name, (const xmlChar *)"THROTTLING_RATE"))
            {
              tempThrottlingRate = (u_long)atoi((char*)attr->children->content);
            }
            /*! 
             *USER inside ITEM is the strongest mask considered. 
             *Do not check for other masks to save it.
             */
						if(rightUser)
						{
              if(st->password2)
                myserver_strlcpy(st->password2, tempPassword, 32);
						}

						attr=attr->next;
					}
					if(rightUser) 
					{
						if(rightPassword)
            {
							userPermissionsFound=2;
              if(tempThrottlingRate != (u_long) -1) 
                st->throttlingRate = (u_long)tempThrottlingRate;
            }
						userPermissions2Found=2;
						userPermissions=tempUserPermissions;
					}
				}
				node2=node2->next;
			}


      {
        attr = node->properties;
        tempThrottlingRate=(u_long)-1;
        /*! Generic ITEM permissions. */
        while(attr)
        {
          if(!xmlStrcmp(attr->name, (const xmlChar *)"READ"))
          {
            if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
              tempFilePermissions|=MYSERVER_PERMISSION_READ;
          }
          else if(!xmlStrcmp(attr->name, (const xmlChar *)"WRITE"))
          {
            if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
              tempFilePermissions|=MYSERVER_PERMISSION_WRITE;
          }
          else if(!xmlStrcmp(attr->name, (const xmlChar *)"EXECUTE"))
          {
            if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
              tempFilePermissions|=MYSERVER_PERMISSION_EXECUTE;
          }
          else if(!xmlStrcmp(attr->name, (const xmlChar *)"DELETE"))
          {
            if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
              tempFilePermissions|=MYSERVER_PERMISSION_DELETE;
          }
          else if(!xmlStrcmp(attr->name, (const xmlChar *)"THROTTLING_RATE"))
          {
            if((tempThrottlingRate==(u_long)-1) || (userPermissionsFound == 0))
              tempThrottlingRate = (u_long)atoi((char*)attr->children->content);
          }
          /*! Check if the file name is correct. */
          if(!xmlStrcmp(attr->name, (const xmlChar *)"FILE"))
          {
            if(attr->children && attr->children->content &&
               (!xmlStrcmp(attr->children->content, 
                           (const xmlChar *)st->filename)))
            {					
              filePermissionsFound=1;
              filePermissions2Found=1;
              if(userPermissionsFound==2)
                userPermissionsFound=1;
              if(userPermissions2Found==2)
                userPermissions2Found=1;
              
            }
          }
          attr=attr->next;
        }/*! End attributes loop. */

        /*! 
         *Check that was not specified a file permission mask 
         *before overwrite these items.
         */
        if(filePermissionsFound && (userPermissionsFound==0))
				{
          if(tempThrottlingRate != (u_long) -1)
            st->throttlingRate = tempThrottlingRate;
        }

      }/*! End generic ITEM attributes. */

			if(filePermissionsFound)
				filePermissions=tempFilePermissions;
		
		}
		node=node->next;
	}

  if(parser == 0)
  {
    localParser.close();
  }

  if(st->permission2)
	{
		*st->permission2=0;
		if(genericPermissions2Found)
		{
			*st->permission2=genericPermissions;
		}
	
		if(filePermissions2Found==1)
		{
			*st->permission2=filePermissions;
		}
				
		if(userPermissions2Found==1)
		{
			*st->permission2=userPermissions;
		}

	}

	if(userPermissionsFound==1)
		return userPermissions;

	if(filePermissionsFound==1)
		return filePermissions;

	if(genericPermissionsFound==1)
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
