# Makefile for counter
lib_LIBRARIES = counter.mscgi
counter_mscgi_SOURCES = main.cpp counter_output.cpp writepng.c

counter_mscgi_CXXFLAGS = $(CXXFLAGS) $(XML_CFLAGS) $(LIBPNG_CFLAGS)
counter_mscgi_LIBADD = $(LIBPNG_LIBS)

all : makemscgi makecounter counter.mscgi
	cp counter.mscgi ../../cgi-bin/

counter.mscgi : $(counter_mscgi_OBJECTS) $(counter_mscgi_DEPENDENCIES)
	-rm -f counter.mscgi
	$(CXX) $(counter_mscgi_OBJECTS) $(counter_mscgi_LIBADD) -o counter.mscgi -shared

makemscgi :
	if test x$(MAKE_MSCGI) = xno; then exit; fi

makecounter :
	if test x$(MAKE_COUNTER) = xno; then exit; fi

install :
	exit; ### install is disabled in myserver
