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
int getErrorFileName(char *root,int error,char** out)
{
	char *permissionsFile;
  *out = 0;
  int permissionsFileLen = strlen(root) + 10;
  permissionsFile = new char[permissionsFileLen];
  if(permissionsFile==0)
    return 0;
	sprintf(permissionsFile,"%s/security",root);
	if(!MYSERVER_FILE::fileExists(permissionsFile))
	{
    delete [] permissionsFile;
		return 0;
	}
	cXMLParser parser;
	if(parser.open(permissionsFile)== -1 )
    return -1;
  delete [] permissionsFile;
	xmlDocPtr doc=parser.getDoc();
	xmlNode *node=doc->children->children;
	int found=0;
	while(node)
	{
		if(!xmlStrcmp(node->name, (const xmlChar *)"ERROR"))
		{
			xmlAttr *attr =  node->properties;
			while(attr)
			{
				if(!xmlStrcmp(attr->name, (const xmlChar *)"FILE"))
				{
          if(*out)
            delete [] (*out);
          *out = new char[strlen((const char*)attr->children->content)+1];
          if(*out == 0)
          {
            parser.close();
            return -1;
          }
					strcpy(*out,(const char*)attr->children->content);
				}
				if(!xmlStrcmp(attr->name, (const xmlChar *)"ID"))
				{
					int error_id=atoi((const char*)attr->children->content);
					if(error_id==error)
						found=1;
				}
				attr=attr->next;
			}
			if(found)
				break;
		}
		node=node->next;
	}
	parser.close();
	return found;
}
int getPermissionMask(char* user, char* password,char* folder,
                      char* filename,char *sysfolder,char *password2,
                      char* auth_type,int len_auth,int *permission2)
{
	char *permissionsFile;
	char tempPassword[32];
  int ret =0;
	tempPassword[0]='\0';
	if(auth_type)
		auth_type[0]='\0';
  int permissionsFileLen = strlen(folder)+10;
  permissionsFile = new char[permissionsFileLen];
  if(permissionsFile == 0)
    return 0;
	sprintf(permissionsFile,"%s/security",folder);

	u_long filenamelen=(u_long)(strlen(filename));
	while(filenamelen && filename[filenamelen]=='.')
	{
		filename[filenamelen--]='\0';
	}
	if(!MYSERVER_FILE::fileExists(permissionsFile))
	{
		/*!
     *If the security file doesn't exist try with a default one.
     */
		if(sysfolder!=0)
			ret = getPermissionMask(user,password,sysfolder,filename,0);
		else
		/*!
     *If the default one doesn't exist too send full permissions for everyone
     */
			ret = (-1);
    delete [] permissionsFile;
    return ret;
	}
	cXMLParser parser;
	if(parser.open(permissionsFile)==-1)
  {
    delete [] permissionsFile;
		return (0);
  }
  delete [] permissionsFile;
	xmlDocPtr doc=parser.getDoc();
	if(!doc)
		return (-1);
	xmlNode *node=doc->children->children;

	int filePermissions=0;
	int filePermissionsFound=0;

	int genericPermissions=0;
	int genericPermissionsFound=0;

	int userPermissions=0;
	int userPermissionsFound=0;

	int filePermissions2Found=0;
	int userPermissions2Found=0;
	int genericPermissions2Found=0;

	while(node)
	{
		if(!xmlStrcmp(node->name, (const xmlChar *)"AUTH"))
		{
			xmlAttr *attr =  node->properties;
			while(attr)
			{
				if(!xmlStrcmp(attr->name, (const xmlChar *)"TYPE"))
				{
					if(auth_type)
						strncpy(auth_type,(const char*)attr->children->content,len_auth);
				}
				attr=attr->next;
			}
		}
	
		if(!xmlStrcmp(node->name, (const xmlChar *)"USER"))
		{
			xmlAttr *attr =  node->properties;
			int tempGenericPermissions=0;
			int rightUser=0;
			int rightPassword=0;
			while(attr)
			{
				if(!xmlStrcmp(attr->name, (const xmlChar *)"READ"))
				{
					if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
						tempGenericPermissions|=MYSERVER_PERMISSION_READ;
				}
				if(!xmlStrcmp(attr->name, (const xmlChar *)"WRITE"))
				{
					if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
						tempGenericPermissions|=MYSERVER_PERMISSION_WRITE;
				}
				if(!xmlStrcmp(attr->name, (const xmlChar *)"BROWSE"))
				{
					if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
						tempGenericPermissions|=MYSERVER_PERMISSION_BROWSE;
				}
				if(!xmlStrcmp(attr->name, (const xmlChar *)"EXECUTE"))
				{
					if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
						tempGenericPermissions|=MYSERVER_PERMISSION_EXECUTE;
				}
				if(!xmlStrcmp(attr->name, (const xmlChar *)"DELETE"))
				{
					if(!xmlStrcmp(attr->children->content, (const xmlChar *)"TRUE"))
						tempGenericPermissions|=MYSERVER_PERMISSION_DELETE;
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
				if(rightUser && password2 && (filePermissions==0) && (userPermissions==0))
				{
					strncpy(password2,tempPassword,16);
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
		if(!xmlStrcmp(node->name, (const xmlChar *)"ITEM"))
		{
			xmlNode *node2=node->children;
			while(node2)
			{
				if(!xmlStrcmp(node2->name, (const xmlChar *)"USER"))
				{
					xmlAttr *attr =  node2->properties;
					int tempUserPermissions=0;
					int rightUser=0;
					int rightPassword=0;
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
			xmlAttr *attr =  node->properties;
			int tempFilePermissions=0;
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
				if(!xmlStrcmp(attr->name, (const xmlChar *)"FILE"))
				{
					if(!lstrcmpi((const char*)attr->children->content,filename))
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

	parser.close();
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
