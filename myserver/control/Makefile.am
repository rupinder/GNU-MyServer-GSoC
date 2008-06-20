bin_PROGRAMS = myserver-configure 
myserver_configure_SRCS = myserver-configure.cpp fltkconfig.cpp service.cpp progress.cpp vector.cpp ../src/file.cpp ../src/files_utility.cpp ../src/xml_parser.cpp ../src/utility.cpp ../src/find_data.cpp language.cpp mimetype.cpp vhost.cpp ../src/md5.cpp ../src/mem_buff.cpp control_client.cpp ../src/ssl_socket.cpp ../src/socket.cpp  ../src/mutex.cpp ../src/thread.cpp ../src/stream.cpp ../src/securestr.cpp

INCLUDES = $(all_includes)

myserver_configure_LDFLAGS = $(all_libraries)
myserver_configure_CXXFLAGS = $(CXXFLAGS) $(XML_CFLAGS) $(FLTK_CFLAGS) -DPREFIX=\"$(prefix)\"
myserver_configure_LDADD = $(XML_LIBS) $(FLTK_LIBS) $(XNET_LIB) $(OPENSSL_LIB)

all : all-am myserver-configure
	cp myserver-configure ../binaries/
