using namespace std;

#include "heading.h"
#include <include/http.h>
#include <include/http_headers.h>

string* call_function(void* context, string* func)
{
	HttpThreadContext* td = ((ThreadContext*) context)->td;
	if(!func->compare("error"))
	{
		td->http->raiseHTTPError(500);
		return allocate_new_str("");
	}
	return allocate_new_str((const char*)"");
}

string* call_function(void* context, string* func, string* arg)
{
	HttpThreadContext* td = ((ThreadContext*) context)->td;
	if(!func->compare("error"))
	{
		int code = atoi(arg->c_str());
		td->http->raiseHTTPError(code);
		return allocate_new_str("");
	}
	else if(!func->compare("getHeader"))
	{
		string* h = td->request.getValue(arg->c_str(), 0);
		if(h)
			return allocate_new_str(h->c_str());
		else
			return allocate_new_str("");
	}

	return allocate_new_str((const char*)"");
}
