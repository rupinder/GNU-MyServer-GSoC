
#pragma comment(lib,"../../../cgi-lib/CGI-LIB.lib")
#include "../../../cgi-lib/cgi_manager.h" 

#ifdef WIN32
int EXPORTABLE main (char *cmd,cgi_data* data)
#else
extern "C" int main (char *cmd,cgi_data* data)
#endif
{     
	cgi_manager cm(data);     
	if(strlen(cmd)==0)     
	{  	
		cm.Write("<title>MyServer</title>\r\n<body bgcolor=\"#FFFFFF\" text=\"#666699\">\r\n<p align=\"center\">\r\n<img border=\"0\" src=\"logo.gif\">\r\n</p>\r\n<p align=\"center\">\r\n<input type=\"text\" ID=\"T1\" size=\"20\">\r\n<p align=\"center\">\r\n+</p>\r\n</p>\r\n<p align=\"center\">\r\n <input type=\"text\" ID=\"T2\" size=\"20\">\r\n</p>\r\n<p align=\"center\">\r\n<input type=\"button\" value=\"Compute!\" onclick=\"javascript:send()\" name=\"B3\">\r\n</p>\r\n<SCRIPT LANGUAGE=\"JavaScript\">\r\nfunction send()\r\n{\r\nvar url=\"math_sum.mscgi?a=\" + document.getElementById(\"T1\").value + \"&b=\" + document.getElementById(\"T2\").value;\r\nwindow.location.assign(url);\r\n}\r\n</SCRIPT>\r\n");
	}     
	else     
	{ 	
		u_int a = 0;
		u_int b = 0;
		char *tmp;
		cm.Write("<title>MyServer</title>\r\n<body bgcolor=\"#FFFFFF\" text=\"#666699\">\r\n<p align=\"center\">\r\n<img border=\"0\" src=\"logo.gif\">\r\n<p align=\"center\">\r\n\r\n");
		tmp = cm.GetParam("a");
		if (tmp)
		{
			if (strlen(tmp) > 10) // a 32-bit number has a maximun of 10 digits
				tmp[10] = '\0';
			a = atoi(tmp);
			cm.Write(tmp);
		}
		else
			cm.Write("0");
		cm.Write(" + ");
		tmp = cm.GetParam("b");
		if (tmp)
		{
			if (strlen(tmp) > 10) // a 32-bit number has a maximun of 10 digits
				tmp[10] = '\0';
			b = atoi(tmp);
			cm.Write(tmp);
		}
		else
			cm.Write("0");
		cm.Write(" = ");
		char res[11]; // a 32-bit number has a maximun of 10 digits
#ifdef	itoa
		itoa(res, a+b, 10);
#elif snprintf 
		snprintf(res,10,"%i", a+b);
#else
		sprintf(res,"%i", a+b);
#endif
		cm.Write(res);

		unsigned int dim=120;
		char lb[120];
		cm.getenv("SERVER_NAME",lb,&dim);
		cm.Write("\r\n<BR>\r\nRunning on: ");
		cm.Write(lb);
		cm.Write("(");
		cm.getenv("HTTP_HOST",lb,&dim);
		cm.Write(lb);
		cm.Write(")\r\n");
		
		cm.Write("</p>\r\n\r\n");
	}
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