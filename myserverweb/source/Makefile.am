# Makefile for MyServer (./source)

bin_PROGRAMS = myserver
myserver_SOURCES = protocol.cpp AMMimeUtils.cpp http_headers.cpp gzip.cpp MIME_manager.cpp winCGI.cpp cXMLParser.cpp cgi.cpp clientsThread.cpp cserver.cpp filemanager.cpp http.cpp httpmsg.cpp isapi.cpp lfind.cpp mscgi.cpp myserver.cpp security.cpp sockets.cpp stringutils.cpp utility.cpp vhosts.cpp fastCGI.cpp md5.cpp

INCLUDES = $(all_includes)

myserver_LDFLAGS = $(all_libraries)
myserver_CXXFLAGS = $(CXXFLAGS) $(XML_CFLAGS)
myserver_LDADD = $(PTHREAD_LIB) $(DL_LIB) $(OPENSSL_LIB) $(ZLIB_LIB) $(XML_LIBS)

all : myserver
	cp myserver ../binaries

install :
	exit; ### install is disabled in myserver

