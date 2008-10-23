bin_PROGRAMS = myserver-configure 
myserver_configure_SOURCES = control_client.cpp fltkconfig.cpp language.cpp mimetype.cpp\
		 myserver-configure.cpp progress.cpp service.cpp vector.cpp vhost.cpp

INCLUDES = $(all_includes)

myserver_configure_LDFLAGS = $(all_libraries) 
myserver_configure_CXXFLAGS = $(CXXFLAGS) $(XML_CFLAGS) $(FLTK_CFLAGS) -DPREFIX=\"$(prefix)\"
myserver_configure_LDADD = ../src/libmyserver.a $(XML_LIBS) $(FLTK_LIBS) $(XNET_LIB) $(OPENSSL_LIB) $(LDFLAGS)

EXTRA_DIST = control_client.h fltkconfig.h language.h mimetype.h progress.h service.h vector.h vhost.h

all : all-am myserver-configure$(EXEEXT)
	cp myserver-configure$(EXEEXT) ../binaries/
