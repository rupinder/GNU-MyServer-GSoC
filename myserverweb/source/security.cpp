/*
*MyServer
*Copyright (C) 2002 The MyServer Team
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


/*
*Global values for useLogonOption flag and the guest handle.
*/
int  useLogonOption;

/*
*Do the logon of an user.
*/
int logonCurrentThread(char *name,char* password,LOGGEDUSERID *handle)
{
	int logon=false;
#ifdef WIN32
	#ifndef LOGON32_LOGON_NETWORK
	#define LOGON32_LOGON_NETWORK 3
	#endif

	#ifndef LOGON32_PROVIDER_DEFAULT
	#define LOGON32_PROVIDER_DEFAULT
	#endif
	logon=LogonUser(name,NULL,password, LOGON32_LOGON_NETWORK, LOGON32_PROVIDER_DEFAULT,(PHANDLE)(handle));
#endif
	return logon;
}
/*
*Change the owner of current thread.
*/
void impersonateLogonUser(LOGGEDUSERID* hImpersonation)
{
#ifdef WIN32
	ImpersonateLoggedOnUser((HANDLE)*hImpersonation);
#endif	
}

/*
*This function terminates the impersonation of a client application.
*/
void revertToSelf()
{
#ifdef WIN32
	RevertToSelf();
#endif
}

/*
*Close the handle of a logged user.
*/
void cleanLogonUser(LOGGEDUSERID* hImpersonation)
{
#ifdef WIN32
	CloseHandle((HANDLE)*hImpersonation);
#endif
}
/*
*Change the owner of the thread with the connection login and password informations.
*/
void logon(LPCONNECTION c,int *logonStatus,LOGGEDUSERID *hImpersonation)
{
	*hImpersonation=0;
	if(useLogonOption)
	{
		if(c->login[0])
		{
			*logonStatus=logonCurrentThread(c->login,c->password,hImpersonation);
		}
		else
		{
			*logonStatus=false;
			*hImpersonation=0;
		}
		impersonateLogonUser(hImpersonation);
	}
	else
	{
		*logonStatus=false;
	}
}
/*
*Logout the hImpersonation handle.
*/
void logout(int /*logon*/,LOGGEDUSERID *hImpersonation)
{
	if(useLogonOption)
	{
		revertToSelf();
		if(*hImpersonation)
		{
			cleanLogonUser(hImpersonation);
			hImpersonation=0;
		}
	}
}
/*
*Do the logon of the guest client.
*/
void logonGuest()
{


}
int getPermissionMask(char* user, char* password,char* folder,char* filename,char *sysfolder)
{
	char permissionsFile[MAX_PATH];
	sprintf(permissionsFile,"%s/security",folder);
	/*
	*If the file doesn't exist allow everyone to do everything
	*/
	if(!useLogonOption)
		return (-1);
	if(!MYSERVER_FILE::fileExists(permissionsFile))
	{
		/*
		*If the security file doesn't exist try with a default one.
		*/
		if(sysfolder!=0)
			return getPermissionMask(user,password,sysfolder,filename,0);
		else
		/*
		*If the default one doesn't exist too send full permissions for everyone
		*/
			return (-1);
	}
	cXMLParser parser;
	parser.open(permissionsFile);
	xmlDocPtr doc=parser.getDoc();
	xmlNode *node=doc->children->children;

	int filePermissions=0;
	int filePermissionsFound=0;

	int genericPermissions=0;
	int genericPermissionsFound=0;

	int userPermissions=0;
	int userPermissionsFound=0;

	while(node)
	{
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
				if(!xmlStrcmp(attr->name, (const xmlChar *)"NAME"))
				{
					if(!xmlStrcmp(attr->children->content, (const xmlChar *)user))
						rightUser=1;
				}
				if(!xmlStrcmp(attr->name, (const xmlChar *)"PASS"))
				{
					if(!xmlStrcmp(attr->children->content, (const xmlChar *)password))
						rightPassword=1;
				}
				attr=attr->next;
			}
			if(rightUser && rightPassword)
			{
				genericPermissionsFound=1;
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
						if(!xmlStrcmp(attr->name, (const xmlChar *)"NAME"))
						{
							if(!xmlStrcmp(attr->children->content, (const xmlChar *)user))
								rightUser=1;
						}
						if(!xmlStrcmp(attr->name, (const xmlChar *)"PASS"))
						{
							if(!xmlStrcmp(attr->children->content, (const xmlChar *)password))
								rightPassword=1;
						}
						attr=attr->next;
					}
					if(rightUser && rightPassword)
					{
						userPermissionsFound=1;
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
					if(!strcmpi((const char*)attr->children->content,filename))
						filePermissionsFound=1;
				}
				attr=attr->next;
			}
			if(filePermissionsFound)
				filePermissions=tempFilePermissions;
		
		}
		node=node->next;
	}

	parser.close();

	if(userPermissionsFound)
		return userPermissions;

	if(filePermissionsFound)
		return filePermissions;

	if(genericPermissionsFound)
		return genericPermissions;
	return 0;
}