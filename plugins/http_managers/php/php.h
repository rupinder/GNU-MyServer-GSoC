#include <php_embed.h>

#include <../stdafx.h>
#include <include/connection.h>
#include <include/sockets.h>
#include <include/server.h>
#include <include/semaphore.h>
#include <include/mutex.h>
#include <include/file.h>
#include <include/files_utility.h>
#include <include/http.h>

extern "C" char* name(char* name, u_long len);


#ifdef WIN32
int EXPORTABLE load(void* server, void* parser);
#else
extern "C" int load(void* server,void* parser);
#endif

#ifdef WIN32
int EXPORTABLE unload(void* p);
#else
	extern "C" int unload(void* p);
#endif

extern "C" int sendManager(HttpThreadContext* td, ConnectionPtr s, const char *filenamePath,
													 const char* cgi, int onlyHeader);


