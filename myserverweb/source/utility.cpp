/*
*myServer
*Copyright (C) Rocky_10_Balboa
*This library is free software; you can redistribute it and/or
*modify it under the terms of the GNU Library General Public
*License as published by the Free Software Foundation; either
*version 2 of the License, or (at your option) any later version.

*This library is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*Library General Public License for more details.

*You should have received a copy of the GNU Library General Public
*License along with this library; if not, write to the
*Free Software Foundation, Inc., 59 Temple Place - Suite 330,
*Boston, MA  02111-1307, USA.
*/
#include "..\stdafx.h"
#include <string.h>
extern BOOL mustEndServer; 
static FILE* logFile=0;
INT getOSVersion()
{
	INT ret=0;
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize=sizeof(osvi);
	GetVersionEx(&osvi);
	switch(osvi.dwMinorVersion)
	{
	case 0:
		if(osvi.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS)
			ret=OS_WINDOWS_9X;
		else
			ret=OS_WINDOWS_2000;

		break;
	case 10:
		ret=OS_WINDOWS_9X;
		break;	
	case 90:
		ret=OS_WINDOWS_9X;
		break;
	case 51:
		ret=OS_WINDOWS_NT3;
		break;
	case 1:
		ret=OS_WINDOWS_XP;
		break;
	}
	return ret;
}	
void printSystemInfoOnLogFile()
{
	SYSTEM_INFO si;
	OSVERSIONINFO osvi;
	GetSystemInfo(&si);
	char localbuffer[128];
	sprintf(localbuffer,"myServer is running on a %u\n",si.dwProcessorType);
	logFileWrite(localbuffer);
	sprintf(localbuffer,"Number of processors:%u\n",si.dwNumberOfProcessors);
	logFileWrite(localbuffer);

	osvi.dwOSVersionInfoSize=sizeof(osvi);
	GetVersionEx(&osvi);
	switch(osvi.dwMinorVersion)
	{
	case 0:
		if(osvi.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS)
			logFileWrite("Running on Windows 95\n");
		else
			logFileWrite("Running on Windows 2000 or NT 4.0\n");

		break;
	case 10:
		logFileWrite("Running on Windows 98\n");
		break;	
	case 90:
		logFileWrite("Running on Windows ME\n");
		break;
	case 51:
		logFileWrite("Running on Windows NT 3.51\n");
		break;
	case 1:
		logFileWrite("Running on Windows XP or .NET SERVER family\n");
		break;
	}
	if(osvi.dwPlatformId==VER_PLATFORM_WIN32s)
	{
		logFileWrite("myServer cannot run on a 16 bit platform\n");
		mustEndServer=TRUE;
		return;
	}
}
DWORD logFileWrite(char* str)
{
	DWORD nbr=lstrlen(str);
	if(logFile)
		fwrite(str,nbr,1,logFile);
	fflush(logFile);
	return nbr;
}
void setLogFile(FILE *f)
{
	logFile =f;
}
void gotoNextLine(char* cmd)
{
	while(*cmd++!='\n')if(*cmd=='\0')break;

}
char gotoNextLine(FILE* f)
{
	char c=(char)fgetc(f);
	while(c!='\n')if(feof(f))return EOF; else c=(char)fgetc(f);
	return c;
}
void getFileExt(char* ext,char*filename)
{
	DWORD len,i;
	ext[0]='\0';
	for( i=0,len=lstrlen(filename) ; i<len ; i++ )
	{
		if((filename[i]=='.')&&(i!=len-1))
			lstrcpy(ext,&filename[i+1]);
	}
}

static char daysName[7][4]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
static char monthsName[12][4]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dic"};
static char localTimeString[31];
char *getHTTPFormattedTime(void)
{
	__int64 ltime;
	_time64( &ltime );
	tm*  gmtime = _gmtime64( &ltime );
	sprintf(localTimeString,"%s, %i %s %i %i:%i:%i GMT",daysName[gmtime->tm_wday],gmtime->tm_mday,monthsName[gmtime->tm_mon],gmtime->tm_year,gmtime->tm_hour,gmtime->tm_min,gmtime->tm_sec);
	return localTimeString;
}
char *getHTTPFormattedTime(tm*  gmtime)
{
	sprintf(localTimeString,"%s, %i %s %i %i:%i:%i GMT",daysName[gmtime->tm_wday],gmtime->tm_mday,monthsName[gmtime->tm_mon],gmtime->tm_year,gmtime->tm_hour,gmtime->tm_min,gmtime->tm_sec);
	return localTimeString;
}


void getFileSize(DWORD* fsd,FILE *f)
{
	static __int64 fs;
	fseek(f, 0, SEEK_END);
	fgetpos(f, &fs);
	fseek(f, 0, SEEK_SET);
	*fsd=(DWORD)fs;
}

int getPathRecursionLevel(char* path)
{
	static char lpath[MAX_PATH];
	lstrcpy(lpath,path);
	int rec=0;
	char *token = strtok( lpath, "\\/" );
	do
	{
		if(lstrcmpi(token,".."))
			rec++;
		else
			rec--;
		token = strtok( NULL, "\\/" );
	}
	while( token != NULL );
	return rec;
}

VOID StrTrim(LPSTR str,LPSTR trimChars)
{
	WORD lenTrimChars=lstrlen(trimChars);
	WORD lenStr=lstrlen(str);
	/*
	*Number of characters to remove from the start of the string
	*/
	WORD ncharToRemove=0;
	BOOL doBreak=FALSE;
	if(!(lenStr&lenTrimChars))
		return;
	for(int j=0;j<=lenStr;j++)
	{
		if(doBreak)
			break;
		for(int i=0;i<lenTrimChars;i++)
		{
			if(str[j]==trimChars[i])
			{
				ncharToRemove++;
			}
			else
			{
				if((i==lenTrimChars-1)||(str[j]==0))
				{
					doBreak=TRUE;
					break;
				}
			}
		}
	}
	lstrcpy(str,&str[ncharToRemove]);
	for(j=0;j=lstrlen(str)-1;j--)
	{
		if(doBreak)
			break;
		for(int i=0;i<lenTrimChars;i++)
		{
			if(str[j]==trimChars[i])
			{
				str[j]='\0';
			}
			else
			{
				if(str[j]==0)
				{
					doBreak=TRUE;
					break;
				}
			}
		}
	}
}

void printOSInfo(INT nVer)
{
	if(nVer==0)
		nVer=getOSVersion();
	switch(nVer)
	{
		case OS_WINDOWS_2000:
			printf("Current OS is Windows 2000 or NT4\n");
			break;
		case OS_WINDOWS_NT3:
			printf("Current OS is Windows NT\n");
			break;
		case OS_WINDOWS_9X:
			printf("Current OS is Windows 9X\n");
			break;
		case OS_WINDOWS_XP:
			printf("Current OS is Windows XP or .NET\n");
			break;
		default:
			printf("Unknown OS\n");
			break;
	}
}