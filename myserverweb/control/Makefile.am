bin_PROGRAMS = myserver-configure
myserver_configure_SOURCES = main.cpp fltkconfig.cpp service.cpp ../source/filemanager.cpp ../source/cXMLParser.cpp ../source/utility.cpp ../source/lfind.cpp language.cpp ../source/vector.cpp  mimetype.cpp vhost.cpp ../source/md5.cpp

INCLUDES = $(all_includes)

myserver_configure_LDFLAGS = $(all_libraries)
myserver_configure_CXXFLAGS = $(CXXFLAGS) $(XML_CFLAGS) $(FLTK_CFLAGS) -DPREFIX=\"$(prefix)\"
myserver_configure_LDADD = $(XML_LIBS) $(FLTK_LIBS)

all : all-am myserver-configure
	cp myserver-configure ../binaries/
