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
#include "..\include\utility.h"
#include <string.h>
extern BOOL mustEndServer; 
INT getOSVersion()
{
	int ret=0;
	/*
	*This is the code for the win32 platform
	*/
#ifdef WIN32
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
#endif
	return ret;
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
DWORD getCPUCount()
{
	DWORD ret=1;
#ifdef WIN32
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	ret=si.dwNumberOfProcessors;
#endif
	return ret;
}

DWORD execHiddenProcess(START_PROC_INFO *spi)
{
#ifdef WIN32
    /*
    *Set the standard output values for the CGI process
    */
    STARTUPINFO si;
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    si.hStdInput = (HANDLE)spi->stdIn;
    si.hStdOutput =(HANDLE)spi->stdOut;
    si.hStdError= (HANDLE)spi->stdError;
    si.dwFlags=STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi;
    ZeroMemory( &pi, sizeof(pi) );
	/*
	*To set the CGI path modify the MIMEtypes file in the bin folder
	*/
    CreateProcess(NULL, spi->cmdLine, NULL, NULL, TRUE,CREATE_SEPARATE_WOW_VDM|CREATE_NEW_CONSOLE,NULL,NULL,&si, &pi);
	/*
	*Wait until it's ending by itself
	*/
	WaitForSingleObject(pi.hProcess,0xFFFFFFFF);
	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );
	return 1;
#endif
}
/*
*Get the local machine name
*/
VOID getComputerName(char *dest,DWORD maxLen)
{
#ifdef WIN32
	if(GetComputerNameA(dest,&maxLen)==0)
	{
		lstrcpy(dest,"localhost");
	}
#endif
}
/*
*This function is similar to the Windows API WaitForSingleObject(..)
*/
INT requestAccess(DWORD* ac,DWORD id)
{
	/*
	*If the access ID is equal to the thread ID we don't do nothing
	*/
	if(*ac==id)
		return 0;
	/*
	*if the access doesn't belong to any thread set that it belongs to the caller thread
	*the check if we have the access now
	*/
	if(*ac==0)
	{
		*ac=id;
		requestAccess(ac,id);
		return 0;
	}
	/*
	*Wait until another thread end the access then set our access
	*/
	while(*ac!=id);
	*ac=id;
	requestAccess(ac,id);
	return 0;
}
INT terminateAccess(DWORD* ac,DWORD id)
{
	/*
	*Only set to Zero the owner of the access
	*/
	*ac=0;
	return 0;
}
