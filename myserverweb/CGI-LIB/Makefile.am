# Makefile for MSCGI-lib

lib_LIBRARIES = libcgi-lib.a
libcgi_lib_a_SOURCES = cgi_manager.cpp ../source/utility.cpp

libcgi_lib_a_CXXFLAGS = $(CXXFLAGS) $(XML_CFLAGS)

all : makemscgi cgi-lib.so
	cp cgi-lib.so ../binaries/cgi-lib/

libcgi-lib.a :

	
cgi-lib.so : $(libcgi_lib_a_OBJECTS) $(libcgi_lib_a_DEPENDENCIES)
	-rm -f cgi-lib.so
	$(CXX) $(libcgi_lib_a_OBJECTS) $(libcgi_lib_a_LIBADD) -o cgi-lib.so -shared

makemscgi :
	if test x$(MAKE_MSCGI) = xno; then exit; fi

install :
	exit; ### install is disabled in myserver

clean :
	rm -f cgi-lib.so
	rm -f *.o
