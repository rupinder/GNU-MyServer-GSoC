# Makefile for myserver control

bin_PROGRAMS = control
control_SOURCES = configuration.cpp configureMIME.cpp control.cpp  configureVHOSTS.cpp

INCLUDES = $(all_includes)

control_LDFLAGS = $(all_libraries)
control_CXXFLAGS = $(CXXFLAGS) $(XML_CFLAGS) $(WX_CFLAGS) -DPREFIX=\"$(prefix)\"
control_LDADD = $(PTHREAD_LIB) $(XML_LIBS) $(WX_LIBS)

all : all-am
	-cp control ../binaries

install : control
	$(INSTALL) -D 'control' '$(bindir)/myserver-control'

uninstall :
	rm -f '$(bindir)/myserver-control'

