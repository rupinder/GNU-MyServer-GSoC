# makefile for MyServer tests suite driver program
#

bin_PROGRAMS = tests_suite
tests_suite_SOURCES = main.cpp test_base64.cpp test_connection.cpp test_files_utility.cpp test_filter_chain.cpp test_ftp.cpp test_gzip.cpp test_hashmap.cpp test_homedir.cpp test_http_request.cpp test_http_response.cpp test_md5.cpp test_mem_buff.cpp test_multicast.cpp test_mutex.cpp test_recursive_mutex.cpp test_regex.cpp test_pipe.cpp test_safetime.cpp test_semaphore.cpp test_thread.cpp test_utility.cpp 
tests_suite_LDADD = ../src/libmyserver.a $(CPPUNIT_LIBS) $(PTHREAD_LIB) $(IDN_LIB) $(XNET_LIB) $(EVENT_LIB) $(DL_LIB) $(OPENSSL_LIB) $(ZLIB_LIB) $(XML_LIBS)
