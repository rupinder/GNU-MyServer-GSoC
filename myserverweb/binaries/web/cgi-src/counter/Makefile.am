# Makefile for counter
lib_LIBRARIES = libcounter.a
libcounter_a_SOURCES = main.cpp counter_output.cpp writepng.c

libcounter_a_CXXFLAGS = $(CXXFLAGS) $(XML_CFLAGS) $(LIBPNG_CFLAGS)
libcounter_a_LIBADD = $(LIBPNG_LIBS)

all : makemscgi makecounter counter.mscgi
	cp counter.mscgi ../../cgi-bin/

libcounter.a :


counter.mscgi : $(libcounter_a_OBJECTS) $(libcounter_a_DEPENDENCIES)
	-rm -f counter.mscgi
	$(CXX) $(libcounter_a_OBJECTS) $(libcounter_a_LIBADD) -o counter.mscgi -shared

makemscgi :
	if test x$(MAKE_MSCGI) = xno; then exit; fi

makecounter :
	if test x$(MAKE_COUNTER) = xno; then exit; fi

install :
	exit; ### install is disabled in myserver

clean :
	rm -f counter.mscgi
	rm -f *.o
