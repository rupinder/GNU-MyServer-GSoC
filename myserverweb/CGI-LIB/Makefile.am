# Makefile for MSCGI-lib

lib_LIBRARIES = cgi-lib.so
cgi_lib_so_SOURCES = cgi_manager.cpp ../source/utility.cpp

cgi_lib_so_CXXFLAGS = $(CXXFLAGS) $(XML_CFLAGS)

all : makemscgi cgi-lib.so
	cp cgi-lib.so ../binaries/cgi-lib/

cgi-lib.so : $(cgi_lib_so_OBJECTS) $(cgi_lib_so_DEPENDENCIES)
	-rm -f cgi-lib.so
	$(CXX) $(cgi_lib_so_OBJECTS) $(cgi_lib_so_LIBADD) -o cgi-lib.so -shared

makemscgi :
	if test x$(MAKE_MSCGI) = xno; then exit; fi

install :
	exit; ### install is disabled in myserver
