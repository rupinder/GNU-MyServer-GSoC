# Makefile for MyServer (./source)

bin_PROGRAMS = myserver
myserver_SOURCES = threads.cpp processes.cpp protocol.cpp protocols_manager.cpp AMMimeUtils.cpp http_headers.cpp gzip.cpp MIME_manager.cpp winCGI.cpp cXMLParser.cpp cgi.cpp clientsThread.cpp cserver.cpp filemanager.cpp http.cpp httpmsg.cpp isapi.cpp lfind.cpp mscgi.cpp myserver.cpp security.cpp sockets.cpp stringutils.cpp utility.cpp vhosts.cpp fastCGI.cpp md5.cpp https.cpp securestr.cpp MemBuf.cpp myserver_regex.cpp vector.cpp control_protocol.cpp control_header.cpp connectionstruct.cpp log_manager.cpp security_cache.cpp hash_dictionary.cpp http_file.cpp http_dir.cpp http_data_handler.cpp

INCLUDES = $(all_includes)

myserver_LDFLAGS = $(all_libraries)
myserver_CXXFLAGS = $(CXXFLAGS) $(XML_CFLAGS) -DHOST_STR=\"$(build)\" -DPREFIX=\"$(prefix)\"
myserver_LDADD = ../contrib/rx/librx.a $(PTHREAD_LIB) $(XNET_LIB) $(DL_LIB) $(OPENSSL_LIB) $(ZLIB_LIB) $(XML_LIBS)

all : myserver
	cp myserver ../binaries

