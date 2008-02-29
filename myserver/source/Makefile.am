# Makefile for MyServer (./source)

bin_PROGRAMS = myserver
myserver_SOURCES = cached_file_buffer.cpp cached_file.cpp cached_file_factory.cpp cgi.cpp cgi_manager.cpp clients_thread.cpp connection.cpp connections_scheduler.cpp control_header.cpp control_protocol.cpp Dev-C++.cpp dynamic_executor.cpp dynamiclib.cpp dynamic_protocol.cpp dyn_executor_manager.cpp dyn_filter.cpp dyn_filter_file.cpp dyn_filters_manager.cpp dyn_http_command.cpp dyn_http_command_manager.cpp dyn_http_manager.cpp http_thread_context.cpp dyn_http_manager_list.cpp event.cpp fastcgi.cpp file.cpp files_utility.cpp filter.cpp filters_chain.cpp filters_factory.cpp generic_plugins_manager.cpp gzip.cpp gzip_decompress.cpp hash_map.cpp home_dir.cpp http.cpp http_data_handler.cpp http_dir.cpp http_errors.cpp http_file.cpp http_header_checker.cpp http_headers.cpp http_request.cpp http_response.cpp https.cpp isapi.cpp lfind.cpp listen_threads.cpp log_manager.cpp md5.cpp mem_buff.cpp memory_stream.cpp mime_manager.cpp mime_utils.cpp mscgi.cpp multicast.cpp mutex.cpp myserver.cpp myserver_regex.cpp pipe.cpp plugin.cpp plugins_manager.cpp plugins_namespace.cpp plugins_namespace_manager.cpp processes.cpp process_server_manager.cpp protocol.cpp protocol_buffer.cpp protocols_manager.cpp read_write_lock.cpp safetime.cpp scgi.cpp securestr.cpp security_cache.cpp security.cpp semaphore.cpp server.cpp sockets.cpp ssl.cpp ssl_sockets.cpp stream.cpp stringutils.cpp thread.cpp utility.cpp vhosts.cpp vhost_manager.cpp wincgi.cpp xml_parser.cpp

INCLUDES = $(all_includes)

myserver_LDFLAGS = $(all_libraries) -rdynamic  
myserver_CXXFLAGS = $(CXXFLAGS) $(XML_CFLAGS) -DHOST_STR=\"$(build)\" -DPREFIX=\"$(prefix)\"
myserver_LDADD =  $(PTHREAD_LIB) $(IDN_LIB) $(XNET_LIB) $(EVENT_LIB) $(DL_LIB) $(OPENSSL_LIB) $(ZLIB_LIB) $(XML_LIBS)

all : myserver
	cp myserver ../binaries

