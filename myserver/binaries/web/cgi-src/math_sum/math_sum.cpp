#ifdef WIN32
#include "cgi_manager.h"
#else
#include <include/http_handler/mscgi/mscgi_manager.h>
#endif

int isNumber(char* s)
{
  if(!s)
    return 0;
  for(u_long i=0; i<strlen(s);i++)
    if(!isdigit(s[i]))
      return 0;
  return 1;
    
}
#ifdef WIN32
extern "C" int EXPORTABLE myserver_main (char *cmd, MsCgiData* data)
#else
extern "C" int myserver_main (char *cmd, MsCgiData* data)
#endif
{     
	MscgiManager cm(data);     
	if(strlen(cmd)==0)     
	{  	
		cm.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\"\r\n\
\"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\r\n\
<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\">\r\n\
<head>\r\n<title>MyServer</title>\r\n\
<meta http-equiv=\"content-type\" content=\"text/html;charset=UTF-8\" />\r\n\
</head>\r\n<body style=\"color: #666699;\">\r\n<div style=\"text-align: center;\">\r\n\
<br />\r\n<img src=\"/logo.png\" alt=\"\" style=\"border: 0px;\" />\r\n<br /><br />\r\n\
<form action=\"math_sum.mscgi\" method=\"get\" enctype=\"text/plain\">\r\n\
<div>\r\n<input type=\"text\" name=\"a\" size=\"20\" />\r\n<br /><br />\r\n+<br /><br />\r\n<input type=\"text\" name=\"b\" size=\"20\" />\r\n<br /><br />\r\n<input type=\"submit\" value=\"Compute!\" />\r\n</div>\r\n</form>\r\n<br />\r\n</div>\r\n</body>\r\n</html>");
	}     
	else     
	{ 	
		u_long dim = 120;
		char lb[120];
		int a = 0;
		int b = 0;
		char *tmp;
    int validNumbers = 1;
		int iRes;
		char res[22]; // a 64-bit number has a maximun of 20 digits and 1 for the sign
		cm.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\"\r\n\
\"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\r\n\
<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\">\r\n\
<head>\r\n<title>MyServer</title>\r\n\
<meta http-equiv=\"content-type\" content=\"text/html;charset=UTF-8\" />\r\n\
</head>\
<body style=\"color: #666699;\">\r\n<div style=\"text-align: center;\">\r\n\
<br /><br />\r\n<img src=\"/logo.png\" alt=\"\" style=\"border: 0px;\" />\r\n<br /><br />\r\n");

    tmp = cm.getParam("a");
    if(tmp && !isNumber(tmp))
    {
      validNumbers=0;
    }
    tmp = cm.getParam("b");
    if(tmp && !isNumber(tmp))
    {
      validNumbers=0;
    }

    if(validNumbers)
    {
      tmp = cm.getParam("a");
      if (tmp && tmp[0] != '\0')
      {
        if (strlen(tmp) > 11) 
          tmp[11] = '\0';
        a = atoi(tmp);
        cm.write(tmp);
      }
      else
      {
        cm.write("0");
      }
      cm.write(" + ");
		
      tmp = cm.getParam("b");

      if (tmp && tmp[0] != '\0')
      {
        if (strlen(tmp) > 11)
          tmp[11] = '\0';
        b = atoi(tmp);
        cm.write(tmp);
      }
      else
        cm.write("0");

      cm.write(" = ");
      iRes = a + b;
#ifdef	WIN32
      _i64toa(iRes, res, 10);
#else
      sprintf(res,"%i", (int)iRes);
#endif
      cm.write(res);
    }
    else
    {
      cm.write("Invalid input format");
    }

		cm.getenv("SERVER_NAME", lb, &dim);
		cm.write("\r\n<br />\r\nRunning on: ");
		cm.write(lb);
		cm.write("(");
		cm.getenv("HTTP_HOST", lb, &dim);
		cm.write(lb);
		
		cm.write(")\r\n</div>\r\n</body>\r\n</html>");
	}
     cm.clean();
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
