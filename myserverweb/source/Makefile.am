# Makefile for MyServer (./source)

bin_PROGRAMS = myserver
myserver_SOURCES = AMMimeUtils.cpp cgi.cpp clientsThread.cpp connectionstruct.cpp control_header.cpp control_protocol.cpp cserver.cpp cXMLParser.cpp Dev-C++.cpp dynamiclib.cpp dyn_filter.cpp dyn_http_command.cpp fastCGI.cpp filemanager.cpp filter.cpp filters_chain.cpp filters_factory.cpp gzip.cpp hash_dictionary.cpp http.cpp http_data_handler.cpp http_dir.cpp http_file.cpp http_header_checker.cpp http_headers.cpp httpmsg.cpp https.cpp isapi.cpp lfind.cpp log_manager.cpp md5.cpp MemBuf.cpp memory_stream.cpp MIME_manager.cpp mscgi.cpp myserver.cpp myserver_regex.cpp processes.cpp protocol.cpp protocols_manager.cpp Response_RequestStructs.cpp securestr.cpp security_cache.cpp security.cpp sockets.cpp stream.cpp stringutils.cpp threads.cpp utility.cpp vhosts.cpp winCGI.cpp

INCLUDES = $(all_includes)

myserver_LDFLAGS = $(all_libraries)
myserver_CXXFLAGS = $(CXXFLAGS) $(XML_CFLAGS) -DHOST_STR=\"$(build)\" -DPREFIX=\"$(prefix)\"
myserver_LDADD = ../contrib/rx/librx.a $(PTHREAD_LIB) $(XNET_LIB) $(DL_LIB) $(OPENSSL_LIB) $(ZLIB_LIB) $(XML_LIBS)

all : myserver
	cp myserver ../binaries

