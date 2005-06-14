bin_PROGRAMS = myserver-configure 
myserver_configure_SOURCES = main.cpp fltkconfig.cpp service.cpp progress.cpp vector.cpp ../source/filemanager.cpp ../source/cXMLParser.cpp ../source/utility.cpp ../source/lfind.cpp language.cpp mimetype.cpp vhost.cpp ../source/md5.cpp ../source/MemBuf.cpp control_client.cpp ../source/sockets.cpp  ../source/threads.cpp ../source/stream.cpp

INCLUDES = $(all_includes)

myserver_configure_LDFLAGS = $(all_libraries)
myserver_configure_CXXFLAGS = $(CXXFLAGS) $(XML_CFLAGS) $(FLTK_CFLAGS) -DPREFIX=\"$(prefix)\"
myserver_configure_LDADD = $(XML_LIBS) $(FLTK_LIBS) $(XNET_LIB) $(OPENSSL_LIB)

all : all-am myserver-configure
	cp myserver-configure ../binaries/
