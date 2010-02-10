# Makefile for MyServer (./src)
SUBDIRS = base conf connection connections_scheduler filter http_handler \
	log plugin protocol server GUI
AM_CPPFLAGS = $(all_includes)
bin_PROGRAMS = myserver
myserver_SOURCES = myserver.cpp
myserver_CXXFLAGS = $(CXXFLAGS) -DHOST_STR=\"$(build)\" -DPREFIX=\"$(prefix)\"
myserver_LDFLAGS =  -rdynamic
myserver_LDADD = libmyserver.a $(PTHREAD_LIB) $(IDN_LIB) \
	$(XNET_LIB) $(DL_LIB) $(SSL_LIB) $(ZLIB_LIB) \
	$(XML_LIBS)  ../lib/libgnu.a $(LDFLAGS)

clean-libmyserver.a:
	rm -f libmyserver.a

libmyserver.a: clean-libmyserver.a
	find . | grep .*\\.$(OBJEXT)$$ | xargs $(AR) cru libmyserver.a
	$(RANLIB) libmyserver.a

all-local: libmyserver.a myserver${EXEEXT}
	cp myserver${EXEEXT} ../binaries

clean-local:
	rm -f libmyserver.a
	find . | grep .*\\.$(OBJEXT)$$ | xargs rm -f
