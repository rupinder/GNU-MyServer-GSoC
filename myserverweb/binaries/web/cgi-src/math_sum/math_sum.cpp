#pragma comment(lib,"../../../cgi-lib/CGI-LIB.lib")
#include "..\..\..\cgi-lib\cgi_manager.h" 

int EXPORTABLE main (char *cmd,cgi_data* data)
{     
	cgi_manager cm(data);     
	if(strlen(cmd)==0)     
	{  	
		cm.Write("<title>MyServer</title>\r\n<body bgcolor=\"#000000\" text=\"#00C800\"><p align=\"center\"><img border=\"0\" src=\"logo.gif\"></p><p align=\"center\"><input type=\"text\" name=\"T1\" size=\"20\"><p align=\"center\">+</p></p><p align=\"center\"> <input type=\"text\" name=\"T2\" size=\"20\"></p><p align=\"center\"><input type=\"button\" value=\"Compute!\" onclick=\"javascript:send()\" name=\"B3\"></p><SCRIPT LANGUAGE=\"JavaScript\">function send(){var url=\"math_sum.mscgi?a=\" + T1.value + \"&b=\" + T2.value;window.location.assign(url);}</SCRIPT>");
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
		 strcpy(a,tmp);
		tmp=cm.GetParam("b");
		if(tmp)
		 strcpy(b,tmp);
		cm.Write("<title>MyServer</title>\r\n<body bgcolor=\"#000000\" text=\"#00C800\"><p align=\"center\">\r\n<img border=\"0\" src=\"logo.gif\"><p align=\"center\">\r\n");
		cm.Write(a);
		cm.Write(" + ");
		cm.Write(b);
		cm.Write(" = ");
		sprintf(c,"%i",atoi(a)+atoi(b));
		cm.Write(c);

		unsigned int dim=120;
		char lb[120];
		cm.getenv("SERVER_NAME",lb,&dim);
		cm.Write("\r\n<BR>Running on: ");
		cm.Write(lb);
		cm.Write("(");
		cm.getenv("HTTP_HOST",lb,&dim);
		cm.Write(lb);
		cm.Write(")\r\n");
		
		cm.Write("</p>\r\n");
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
