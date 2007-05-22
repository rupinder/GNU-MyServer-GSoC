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

#ifdef WIN32
#define EXPORTABLE _declspec(dllexport);
#else
#define EXPORTABLE extern "C"
#endif

extern "C" char* name(char* name, u_long len);


EXPORTABLE int load(void* server, void* parser);

EXPORTABLE int unload(void* p);

EXPORTABLE int sendManager(HttpThreadContext* td, ConnectionPtr s, const char *filenamePath,
													 const char* cgi, int onlyHeader);


