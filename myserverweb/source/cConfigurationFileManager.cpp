#include "..\include\cConfigurationFileManager.h"

void cConfigurationFileManager::open(char* filename)
{
	file = fopen(filename,"rt");
	getFileSize(&buffersize,file);
	buffer=(char*)malloc(buffersize);
	if(buffer)
		fread(buffer,buffersize,1,file);
}
/*
*Only get the the text T in <VALUENAME>T</VALUENAME>
*/
char *cConfigurationFileManager::getValue(char* vName)
{
	if(buffer==0)
		return 0;
	BOOL found;
	char *ret=NULL;
	unsigned int i,j;
	for(i=0;i<buffersize;i++)
	{
 		if(buffer[i]=='<')
		{
			/*
			*If there is a comment go to end of this
			*/
			if(buffer[i+1]=='!')
			if(buffer[i+2]=='-')
			if(buffer[i+3]=='-')
			{
				i+=3;
				while((buffer[i]!='-')&&(buffer[i+1]!='>'))
					i++;
				continue;
			}
			/*
			*If we arrive here this isn't a comment
			*/
			found=TRUE;

			for(j=0;j<lstrlen(vName);j++)
			{
				if(buffer[i+j+1]!=vName[j])
				{
					found=FALSE;
					break;
				}
				
			}
			i+=lstrlen(vName)+2;
			if(found)
			{
				for(j=0;;j++)
				{
					data[j]=buffer[i+j];
					data[j+1]='\0';
					if(buffer[i+j+1]=='<')
						break;
				}
				ret=data;
				break;
			}
		}
	}
	return ret;
}
void cConfigurationFileManager::close()
{
	if(file)
		fclose(file);
	if(buffer)
		free(buffer);
}
