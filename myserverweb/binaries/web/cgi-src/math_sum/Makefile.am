# Makefile for math_sum
lib_LIBRARIES = math_sum.mscgi
math_sum_mscgi_SOURCES = math_sum.cpp

math_sum_mscgi_CXXFLAGS = $(CXXFLAGS) $(XML_CFLAGS)

all : makemscgi math_sum.mscgi
	cp math_sum.mscgi ../../cgi-bin/

math_sum.mscgi : $(math_sum_mscgi_OBJECTS) $(math_sum_mscgi_DEPENDENCIES)
	-rm -f math_sum.mscgi
	$(CXX) $(math_sum_mscgi_OBJECTS) $(math_sum_mscgi_LIBADD) -o math_sum.mscgi -shared

makemscgi :
	if test x$(MAKE_MSCGI) = xno; then exit; fi

install :
	exit; ### install is disabled in myserver
