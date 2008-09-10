# Makefile for MyServer (./src)
SUBDIRS = base conf connection connections_scheduler filter http_handler \
	log plugin protocol server
INCLUDES = $(all_includes)
bin_PROGRAMS = myserver
myserver_SOURCES = myserver.cpp
myserver_CXXFLAGS = $(CXXFLAGS) $(XML_CFLAGS) -DHOST_STR=\"$(build)\" -DPREFIX=\"$(prefix)\"
myserver_LDFLAGS =  -rdynamic
myserver_LDADD = libmyserver.a $(all_libraries) $(PTHREAD_LIB) $(IDN_LIB) \
	$(XNET_LIB) $(EVENT_LIB) $(DL_LIB) $(OPENSSL_LIB) $(ZLIB_LIB) \
	$(XML_LIBS) $(LDFLAGS)

libmyserver.a:
	rm -f libmyserver.a
	find -iregex .*\\.$(OBJEXT) | xargs $(AR) cru libmyserver.a
	$(RANLIB) libmyserver.a

build_parser: protocol/ftp/ftp_lexer.lpp protocol/ftp/ftp_parser.ypp
	bison --defines=../include/protocol/ftp/ftp_parser.h protocol/ftp/ftp_parser.ypp
	cp protocol/ftp/ftp_parser.tab.cpp protocol/ftp/ftp_parser.cpp
	flex --header-file=../include/protocol/ftp/ftp_lexer.h protocol/ftp/ftp_lexer.lpp
	cp ./protocol/ftp/lex.yy.c ./protocol/ftp/ftp_lexer.cpp

all-local: libmyserver.a myserver${EXEEXT}
	cp myserver${EXEEXT} ../binaries

clean-local:
	rm -f libmyserver.a
	find -iregex .*\\.$(OBJEXT) | xargs rm -f
