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

#pragma comment(lib,"../../../cgi-lib/CGI-LIB.lib")
#include "../../../cgi-lib/cgi_manager.h"
#include "counter_output.h"
#include <limits.h>
#include <math.h>

#ifdef WIN32
int EXPORTABLE main (char *cmd,cgi_data* data)
#else
extern "C" int main (char *cmd,cgi_data* data)
#endif
{ 
	cgi_manager cm(data);
	
	Counter_Output counter;
	
	MYSERVER_FILE msfile;
	
	cm.setContentType("image/png");
	
	counter.setWrite(&cm); //set the png writer function
	
	// Lets count!
	unsigned long int count;
	u_long nbw;
	
	if(msfile.fileExists("count.dat"))
	{
		// read the last number
		msfile.openFile("count.dat", MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_ALWAYS);
		msfile.readFromFile((char *)&count, sizeof(count), &nbw);
		msfile.closeFile();
		
		count++; // add the hit
		
		if(count > ULONG_MAX - 5)
			count = 1;
			
		//now save it
		msfile.openFile("count.dat", MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_OPEN_ALWAYS);
		msfile.writeToFile((char *)&count, sizeof(count), &nbw);
		msfile.closeFile();
	}
	else
	{
		// never been counted so start
		count = 1;
		
		//now save it
		msfile.openFile("count.dat", MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_CREATE_ALWAYS);
		msfile.writeToFile((char *)&count, sizeof(count), &nbw);
		msfile.closeFile();
	}
	
	counter.setNumber(count);
	
	counter.run();
	
	cm.Clean();
	return 0; 
}  

#ifdef WIN32
BOOL APIENTRY DllMain( HANDLE,DWORD ul_reason_for_call,LPVOID) 
{ 	
	switch (ul_reason_for_call) 	
	{ 	
		case DLL_PROCESS_ATTACH: 	
		case DLL_THREAD_ATTACH: 	
		case DLL_THREAD_DETACH: 	
		case DLL_PROCESS_DETACH: 		
			break; 	
	}    
	return TRUE; 
}
#endif
	