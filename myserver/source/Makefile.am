# Makefile for MyServer (./source)

bin_PROGRAMS = myserver
myserver_SOURCES = cached_file_buffer.cpp cached_file.cpp cached_file_factory.cpp cgi.cpp clients_thread.cpp connection.cpp control_header.cpp control_protocol.cpp Dev-C++.cpp dynamiclib.cpp dyn_filter.cpp dyn_http_command.cpp dyn_http_manager.cpp event.cpp fastcgi.cpp file.cpp files_utility.cpp filter.cpp filters_chain.cpp filters_factory.cpp gzip.cpp gzip_decompress.cpp hash_map.cpp home_dir.cpp http.cpp http_data_handler.cpp http_dir.cpp http_file.cpp http_header_checker.cpp http_headers.cpp httpmsg.cpp http_request.cpp http_response.cpp https.cpp isapi.cpp lfind.cpp log_manager.cpp md5.cpp mem_buff.cpp memory_stream.cpp mime_manager.cpp mime_utils.cpp mscgi.cpp mutex.cpp myserver.cpp myserver_regex.cpp pipe.cpp processes.cpp protocol.cpp protocols_manager.cpp securestr.cpp security_cache.cpp security.cpp semaphore.cpp server.cpp sockets.cpp stream.cpp stringutils.cpp thread.cpp utility.cpp vhosts.cpp wincgi.cpp xml_parser.cpp

INCLUDES = $(all_includes)

myserver_LDFLAGS = $(all_libraries) -rdynamic  
myserver_CXXFLAGS = $(CXXFLAGS) $(XML_CFLAGS) -DHOST_STR=\"$(build)\" -DPREFIX=\"$(prefix)\"
myserver_LDADD = ../contrib/rx/librx.a $(PTHREAD_LIB) $(XNET_LIB) $(DL_LIB) $(OPENSSL_LIB) $(ZLIB_LIB) $(XML_LIBS)

all : myserver
	cp myserver ../binaries

