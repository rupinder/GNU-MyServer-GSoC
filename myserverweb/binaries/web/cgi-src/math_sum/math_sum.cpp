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
		int i=0;
		char a[16];
		char b[16];
		char c[16];
		char *tmp;
		a[0]=b[0]=c[0]='0';
		a[1]=b[1]=c[1]='\0';		
		tmp=cm.GetParam("a");
		if(tmp)
		 strncpy(a,tmp,16);
		tmp=cm.GetParam("b");
		if(tmp)
		 strncpy(b,tmp,16);
		cm.Write("<title>MyServer</title>\r\n<body bgcolor=\"#FFFFFF\" text=\"#666699\">\r\n<p align=\"center\">\r\n<img border=\"0\" src=\"logo.gif\">\r\n<p align=\"center\">\r\n\r\n");
		cm.Write(a);
		cm.Write(" + ");
		cm.Write(b);
		cm.Write(" = ");
		sprintf(c,"%i",atoi(a)+atoi(b));
		cm.Write(c);

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
