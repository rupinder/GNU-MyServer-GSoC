# Makefile for myserver control

bin_PROGRAMS = control
control_SOURCES = configuration.cpp configureMIME.cpp control.cpp ../source/lfind.cpp configureVHOSTS.cpp

INCLUDES = $(all_includes)

control_LDFLAGS = $(all_libraries)
control_CXXFLAGS = $(CXXFLAGS) $(XML_CFLAGS) $(WX_CFLAGS)
control_LDADD = $(XML_LIBS) $(WX_LIBS)

all : makecontrol control
	cp control ../binaries

install :
	exit; ### install is disabled in myserver

makecontrol :
	if test x$(MAKE_CONTROL) = xno; then exit; fi