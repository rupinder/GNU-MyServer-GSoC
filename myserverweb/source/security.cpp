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

#ifdef WIN32
#ifndef LOGON32_LOGON_NETWORK
#define LOGON32_LOGON_NETWORK 3
#endif

#ifndef LOGON32_PROVIDER_DEFAULT
#define LOGON32_PROVIDER_DEFAULT
#endif
#endif


/*!
 *Get the error file for a site. 
 *Return 0 to use the default one.
 *Return -1 on errors.
 *Return other valus on success, please note to free
 *out after its use.
 */
int SecurityManager::getErrorFileName(char* sysDir,int error, 
                                      char** out, XmlParser* parser)
{
	xmlNode *node;
	char *permissionsFile;
  int permissionsFileLen;
	XmlParser local_parser;  
  xmlDocPtr doc;
	int found=0;
  *out = 0;
  if(parser == 0)
  { 
    permissionsFileLen = strlen(sysDir) + 10;
    permissionsFile = new char[permissionsFileLen];
    if(permissionsFile==0)
      return 0;
    sprintf(permissionsFile,"%s/security", sysDir);
    if(!File::fileExists(permissionsFile))
    {
      delete [] permissionsFile;
      return 0;
    }
    if(local_parser.open(permissionsFile)== -1 )
      return -1;
    delete [] permissionsFile;
    doc=local_parser.getDoc();
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
            *out = new char[strlen(fileName)+1];
            if(*out == 0)
            {
              if(parser == 0)
              {
                local_parser.close();
              }
              return -1;
            }
            strcpy(*out,(const char*)fileName);
          }
       }
				if(!xmlStrcmp(attr->name, (const xmlChar *)"ID"))
				{
					int error_id = atoi((const char*)attr->children->content);
          if(*out)
            delete [] (*out);

					if(error_id==error)
						found=1;
          else
            continue;
          /*! The file name was still specified. */
          if(fileName)
          {
            *out = new char[strlen(fileName)+1];
            if(*out == 0)
            {
              if(parser == 0)
              {
                local_parser.close();
              }
              return -1;
            }
            strcpy(*out,(const char*)fileName);
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
    local_parser.close();
  
  /*! Return 1 if both it was found and well configured. */
  if(found && (*out))
    return 1;
  
  return 0;

}

/*!
 *Get the permissions mask for the file[filename]. The file [directory]/security will
 *be parsed. If a [parser] is specified, it will be used instead of opening 
 *the security file.
 *[permission2] is the permission mask that the [user] will have if providing a 
 *[password2].
 */
int SecurityManager::getPermissionMask(char* user, char* password, char* directory,
                                       char* filename, char *password2, 
                                       char* auth_type, int len_auth, 
                                       int *permission2, XmlParser* parser)
{

	char *permissionsFile;
  int permissionsFileLen;

	char tempPassword[32];
  int ret = 0;

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

  xmlAttr *attr;
	xmlNode *node;

	XmlParser local_parser;
  xmlDocPtr doc;

	tempPassword[0]='\0';
	if(auth_type)
		auth_type[0]='\0';

  /*! 
   *If there is not specified the parser to use, create a new parser
   *object and initialize it. 
   */
  if(parser == 0)
  {
    u_long filenamelen;
    permissionsFileLen = strlen(directory)+10;
    permissionsFile = new char[permissionsFileLen];
    if(permissionsFile == 0)
      return 0;
    sprintf(permissionsFile,"%s/security",directory);

    filenamelen=(u_long)(strlen(filename));
    while(filenamelen && filename[filenamelen]=='.')
    {
      filename[filenamelen--]='\0';
    }
    /*! If the specified file doesn't exist return 0. */
    if(!File::fileExists(permissionsFile))
    {
      ret = 0;
      delete [] permissionsFile;
      return ret;
    }
    else
    {
      if(local_parser.open(permissionsFile)==-1)
      {
        delete [] permissionsFile;
        return 0;
      }
      delete [] permissionsFile;
      doc=local_parser.getDoc();
    }
  }
  else
  {
    /*! If the parser object is specified get the doc from it. */
    doc=parser->getDoc();
  }

  if(!doc)
    return 0;

  /*! 
   *If the file is not valid, returns 0.
   *Clean the parser object if was created here.
   */
  if(doc->children && doc->children->children)
    node=doc->children->children;
  else if(parser == 0)
  {
    local_parser.close();
    return 0;
  }

	while(node)
	{
    /*! Retrieve the authorization scheme to use if specified. */
		if(!xmlStrcmp(node->name, (const xmlChar *)"AUTH"))
		{
			attr = node->properties;
			while(attr)
			{
				if(!xmlStrcmp(attr->name, (const xmlChar *)"TYPE"))
				{
					if(auth_type)
						strncpy(auth_type,(const char*)attr->children->content, len_auth);
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
				if(!xmlStrcmp(attr->name, (const xmlChar *)"WRITE"))
				{
					if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
						tempGenericPermissions |= MYSERVER_PERMISSION_WRITE;
				}
				if(!xmlStrcmp(attr->name, (const xmlChar *)"BROWSE"))
				{
					if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
						tempGenericPermissions |= MYSERVER_PERMISSION_BROWSE;
				}
				if(!xmlStrcmp(attr->name, (const xmlChar *)"EXECUTE"))
				{
					if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
						tempGenericPermissions |= MYSERVER_PERMISSION_EXECUTE;
				}
				if(!xmlStrcmp(attr->name, (const xmlChar *)"DELETE"))
				{
					if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
						tempGenericPermissions |= MYSERVER_PERMISSION_DELETE;
				}
				if(!lstrcmpi((const char*)attr->name,"NAME"))
				{
					if(!lstrcmpi((const char*)attr->children->content,user))
						rightUser=1;
				}
				if(!xmlStrcmp(attr->name, (const xmlChar *)"PASS"))
				{
					strcpy(tempPassword,(char*)attr->children->content);
					if(!xmlStrcmp(attr->children->content, (const xmlChar *)password))
						rightPassword=1;
				}
        /*! 
         *USER is the weakest permission considered. Be sure that no others are
         *specified before save the password2.
         */
				if(rightUser && password2 && (filePermissions==0) && (userPermissions==0))
				{
					strncpy(password2, tempPassword, 16);
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
						if(!xmlStrcmp(attr->name, (const xmlChar *)"WRITE"))
						{
							if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
								tempUserPermissions|=MYSERVER_PERMISSION_WRITE;
						}
						if(!xmlStrcmp(attr->name, (const xmlChar *)"EXECUTE"))
						{
							if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
								tempUserPermissions|=MYSERVER_PERMISSION_EXECUTE;
						}
						if(!xmlStrcmp(attr->name, (const xmlChar *)"DELETE"))
						{
							if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
								tempUserPermissions|=MYSERVER_PERMISSION_DELETE;
						}
						if(!lstrcmpi((const char*)attr->name,"NAME"))
						{
							if(!lstrcmpi((const char*)attr->children->content,user))
								rightUser=1;
						}
						if(!xmlStrcmp(attr->name, (const xmlChar *)"PASS"))
						{
							strcpy(tempPassword,(char*)attr->children->content);
							if(!lstrcmp((const char*)attr->children->content,password))
								rightPassword=1;
						}
            /*! 
             *USER inside ITEM is the strongest mask considered. 
             *Do not check for other masks to save it.
             */
						if(rightUser && password2)
						{
							strncpy(password2,tempPassword,16);
						}

						attr=attr->next;
					}
					if(rightUser) 
					{
						if(rightPassword)
							userPermissionsFound=2;
						userPermissions2Found=2;
						userPermissions=tempUserPermissions;
					}
				}
				node2=node2->next;
			}

      attr = node->properties;

      /*! Generic ITEM permissions. */
			while(attr)
			{
				if(!xmlStrcmp(attr->name, (const xmlChar *)"READ"))
				{
					if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
						tempFilePermissions|=MYSERVER_PERMISSION_READ;
				}
				if(!xmlStrcmp(attr->name, (const xmlChar *)"WRITE"))
				{
					if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
						tempFilePermissions|=MYSERVER_PERMISSION_WRITE;
				}
				if(!xmlStrcmp(attr->name, (const xmlChar *)"EXECUTE"))
				{
					if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
						tempFilePermissions|=MYSERVER_PERMISSION_EXECUTE;
				}
				if(!xmlStrcmp(attr->name, (const xmlChar *)"DELETE"))
				{
					if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
						tempFilePermissions|=MYSERVER_PERMISSION_DELETE;
				}

        /*! Check if the file name is correct. */
				if(!xmlStrcmp(attr->name, (const xmlChar *)"FILE"))
				{
          if(attr->children && attr->children->content &&
             (!xmlStrcmp(attr->children->content, (const xmlChar *)filename)))
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
			}
			if(filePermissionsFound)
				filePermissions=tempFilePermissions;
		
		}
		node=node->next;
	}

  if(parser == 0)
  {
    local_parser.close();
  }

  if(permission2)
	{
		*permission2=0;
		if(genericPermissions2Found)
		{
			*permission2=genericPermissions;
		}
	
		if(filePermissions2Found==1)
		{
			*permission2=filePermissions;
		}
				
		if(userPermissions2Found==1)
		{
			*permission2=userPermissions;
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
