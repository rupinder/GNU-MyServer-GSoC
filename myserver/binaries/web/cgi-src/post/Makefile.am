# Makefile for post
lib_LIBRARIES = libpost.a
libpost_a_SOURCES = post.cpp

libpost_a_CXXFLAGS = $(CXXFLAGS) $(XML_CFLAGS) -I$(MYSERVER_HEADERS)

all : all-am post.mscgi
	cp post.mscgi ../../cgi-bin/

libpost.a :


post.mscgi : $(libpost_a_OBJECTS) $(libpost_a_DEPENDENCIES)
	-rm -f post.mscgi
	$(CXX) $(libpost_a_OBJECTS) ../../../../src/libmyserver.a $(libpost_a_LIBADD)  $(PTHREAD_LIB) $(IDN_LIB) $(XNET_LIB) $(EVENT_LIB) $(DL_LIB) $(OPENSSL_LIB) $(ZLIB_LIB) $(XML_LIBS) $(LDFLAGS) -o post.mscgi -shared

install : post.mscgi
	$(INSTALL) 'post.mscgi' '$(root_prefix)/var/web/cgi-bin/post.mscgi'

clean :
	rm -f post.mscgi
	rm -f *.o
