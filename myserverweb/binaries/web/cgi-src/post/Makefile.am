# Makefile for post
lib_LIBRARIES = post.mscgi
post_mscgi_SOURCES = post.cpp

post_mscgi_CXXFLAGS = $(CXXFLAGS) $(XML_CFLAGS)

all : makemscgi post.mscgi
	cp post.mscgi ../../cgi-bin/

post.mscgi : $(post_mscgi_OBJECTS) $(post_mscgi_DEPENDENCIES)
	-rm -f post.mscgi
	$(CXX) $(post_mscgi_OBJECTS) $(post_mscgi_LIBADD) -o post.mscgi -shared

makemscgi :
	if test x$(MAKE_MSCGI) = xno; then exit; fi

install :
	exit; ### install is disabled in myserver
