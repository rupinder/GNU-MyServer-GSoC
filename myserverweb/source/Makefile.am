# Makefile for MyServer (./source)

bin_PROGRAMS = myserver
myserver_SOURCES = mime_utils.cpp cgi.cpp clients_thread.cpp connection.cpp control_header.cpp control_protocol.cpp server.cpp xml_parser.cpp Dev-C++.cpp dynamiclib.cpp dyn_filter.cpp dyn_http_command.cpp dyn_http_manager.cpp fastcgi.cpp file.cpp filter.cpp filters_chain.cpp filters_factory.cpp gzip.cpp home_dir.cpp hash_map.cpp http.cpp http_data_handler.cpp http_dir.cpp http_file.cpp http_header_checker.cpp http_headers.cpp http_request.cpp http_response.cpp httpmsg.cpp https.cpp isapi.cpp lfind.cpp log_manager.cpp md5.cpp mem_buff.cpp memory_stream.cpp mime_manager.cpp mscgi.cpp myserver.cpp myserver_regex.cpp mutex.cpp pipe.cpp processes.cpp protocol.cpp protocols_manager.cpp securestr.cpp security_cache.cpp security.cpp sockets.cpp stream.cpp stringutils.cpp thread.cpp utility.cpp vhosts.cpp wincgi.cpp

INCLUDES = $(all_includes)

myserver_LDFLAGS = $(all_libraries) -rdynamic  
myserver_CXXFLAGS = $(CXXFLAGS) $(XML_CFLAGS) -DHOST_STR=\"$(build)\" -DPREFIX=\"$(prefix)\"
myserver_LDADD = ../contrib/rx/librx.a $(PTHREAD_LIB) $(XNET_LIB) $(DL_LIB) $(OPENSSL_LIB) $(ZLIB_LIB) $(XML_LIBS)

all : myserver
	cp myserver ../binaries

