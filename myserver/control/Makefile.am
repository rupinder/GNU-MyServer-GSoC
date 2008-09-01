bin_PROGRAMS = myserver-configure 
myserver_configure_SOURCES = control_client.cpp fltkconfig.cpp language.cpp mimetype.cpp\
														 myserver-configure.cpp progress.cpp service.cpp vector.cpp vhost.cpp
INCLUDES = $(all_includes)

myserver_configure_LDFLAGS = $(all_libraries) ../src/libmyserver.a 
myserver_configure_CXXFLAGS = $(CXXFLAGS) $(XML_CFLAGS) $(FLTK_CFLAGS) -DPREFIX=\"$(prefix)\"
myserver_configure_LDADD = $(XML_LIBS) $(FLTK_LIBS) $(XNET_LIB) $(OPENSSL_LIB)

EXTRA_DIST = control_client.h fltkconfig.h language.h mimetype.h progress.h service.h vector.h vhost.h

all : all-am myserver-configure
	cp myserver-configure ../binaries/
