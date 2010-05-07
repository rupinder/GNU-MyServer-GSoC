#include <include/http_handler/mscgi/mscgi_manager.h>


const char *program_name;

EXPORTABLE(int) myserver_main (char *cmd, MsCgiData* data)
{
  try
    {
      MscgiManager cm (data);

      char *post = cm.postParam ("T1");

      program_name = cmd;
      if(post == NULL)
        {
          cm.write ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\"\r\n"
                    "\"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\r\n"
                    "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\">\r\n"
                    "<head>\r\n<title>MyServer</title>\r\n"
                    "<meta http-equiv=\"content-type\" content=\"text/html;charset=UTF-8\" />\r\n"
                    "</head>\r\n<body style=\"color: #666699;\">\r\n<div style=\"text-align: center;\">\r\n"
                    "<br />\r\n<img src=\"/logo.png\" alt=\"\" style=\"border: 0px;\" />\r\n<br /><br />\r\n"
                    "<form action=\"post.mscgi\" method=\"post\" enctype=\"text/plain\">\r\n"
                    "<div>\r\n<input type=\"text\" name=\"T1\" size=\"20\" value=\"POST\" />\r\n"
                    "<br /><br />\r\n<input type=\"submit\" value=\"Send\" />\r\n"
                    "<input type=\"reset\" value=\"Reset\" />\r\n</div>\r\n</form>\r\n<br />\r\n</div>\r\n</body>\r\n</html>");
        }
      else
        {
          cm.write ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\"\r\n"
                    "\"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\r\n"
                    "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\">\r\n"
                    "<head>\r\n<title>MyServer</title>\r\n"
                    "<meta http-equiv=\"content-type\" content=\"text/html;charset=UTF-8\" />\r\n"
                    "</head>\r\n<body style=\"color: #666699;\">\r\n<div style=\"text-align: center;\">\r\n"
                    "<br />\r\n<img src=\"/logo.png\" alt=\"\" style=\"border: 0px;\" />\r\n<br /><br />\r\n"
                    "Posted argument:&nbsp;");
          cm.write (post);
          cm.write ("\r\n</div>\r\n</body>\r\n</html>");
        }

      cm.clean ();
    }
  catch (exception & e)
    {
      return 1;
    }
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
