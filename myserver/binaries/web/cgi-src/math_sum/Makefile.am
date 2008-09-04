# Makefile for math_sum
lib_LIBRARIES = libmath_sum.a
libmath_sum_a_SOURCES = math_sum.cpp

libmath_sum_a_CXXFLAGS = $(CXXFLAGS) $(XML_CFLAGS) -I$(MYSERVER_HEADERS)

all : all-am math_sum.mscgi
	cp math_sum.mscgi ../../cgi-bin/

libmath_sum.a :

	
math_sum.mscgi : $(libmath_sum_a_OBJECTS) $(libmath_sum_a_DEPENDENCIES)
	-rm -f math_sum.mscgi
	$(CXX) $(libmath_sum_a_OBJECTS) ../../../../src/libmyserver.a $(libmath_sum_a_LIBADD)  $(PTHREAD_LIB) $(IDN_LIB) $(XNET_LIB) $(EVENT_LIB) $(DL_LIB) $(OPENSSL_LIB) $(ZLIB_LIB) $(XML_LIBS) $(LDFLAGS) -o math_sum.mscgi -shared

install : math_sum.mscgi
	$(INSTALL) 'math_sum.mscgi' '$(root_prefix)/var/web/cgi-bin/math_sum.mscgi'


clean :
	rm -f math_sum.mscgi
	rm -f *.o
