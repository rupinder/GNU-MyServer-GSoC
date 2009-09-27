# makefile for MyServer tests suite driver program
#

bin_PROGRAMS = tests_suite
tests_suite_SOURCES = main.cpp test_auth_domain.cpp test_base64.cpp test_bitvec.cpp test_cached_file_buffer.cpp test_cached_file.cpp \
test_cached_file_factory.cpp test_connection.cpp test_connections_scheduler.cpp test_file.cpp test_file_stream.cpp \
test_file_stream_creator.cpp test_files_utility.cpp test_filter_chain.cpp test_fork_server.cpp test_ftp.cpp test_gzip.cpp \
test_hashmap.cpp test_homedir.cpp test_http_req_security_domain.cpp test_http_request.cpp test_http_response.cpp test_ip.cpp \
test_log_manager.cpp test_log_stream_factory.cpp test_md5.cpp test_mem_buff.cpp test_mem_stream.cpp test_mime_manager.cpp \
test_multicast.cpp test_mutex.cpp test_pipe.cpp test_plugin_info.cpp test_recursive_mutex.cpp test_regex.cpp test_safetime.cpp \
test_security_cache.cpp test_security_domain.cpp test_security_manager.cpp test_security_token.cpp test_semaphore.cpp test_slab.cpp \
test_socket.cpp test_socket_pair.cpp test_socket_stream_creator.cpp test_ssl_socket.cpp test_thread.cpp test_unix_socket.cpp \
test_url.cpp test_utility.cpp test_validator.cpp test_validator_factory.cpp test_xml.cpp test_xml_validator.cpp test_nodetree.cpp

tests_suite_LDADD = ../src/libmyserver.a $(CPPUNIT_LDFLAGS) $(PTHREAD_LIB) $(IDN_LIB) $(XNET_LIB) $(EVENT_LIB) $(DL_LIB) $(SSL_LIB) $(ZLIB_LIB) $(XML_LIBS) $(LDFLAGS)


install:


check:
	./tests_suite$(EXEEXT)
