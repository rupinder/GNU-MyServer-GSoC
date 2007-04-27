#include <php_embed.h>

#include <../stdafx.h>
#include <connection.h>
#include <sockets.h>
#include <server.h>
#include <mutex.h>
#include <file.h>
#include <files_utility.h>
#include <http.h>

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


