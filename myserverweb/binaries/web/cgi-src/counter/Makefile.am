# Makefile for counter

EXTRA_DIST = README

lib_LIBRARIES = libcounter.a
libcounter_a_SOURCES = main.cpp counter_output.cpp writepng.c

libcounter_a_CXXFLAGS = $(CXXFLAGS) $(XML_CFLAGS) $(LIBPNG_CFLAGS)
libcounter_a_LIBADD = $(LIBPNG_LIBS)

all : all-am counter.mscgi
	cp counter.mscgi ../../cgi-bin/

libcounter.a :


counter.mscgi : $(libcounter_a_OBJECTS) $(libcounter_a_DEPENDENCIES)
	-rm -f counter.mscgi
	$(CXX) $(libcounter_a_OBJECTS) $(libcounter_a_LIBADD) -o counter.mscgi -shared

install : counter.mscgi
	$(INSTALL) -D 'counter.mscgi' '$(root_prefix)/var/web/cgi-bin/counter.mscgi'

clean :
	rm -f counter.mscgi
	rm -f *.o
