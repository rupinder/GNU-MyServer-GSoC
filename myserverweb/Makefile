# Makefile : Builds myserver, control, CGI-LIB, math_sum, counter, and post
#

MYSERVER_SUBDIRS=source control CGI-LIB binaries/web/cgi-src/math_sum binaries/web/cgi-src/post binaries/web/cgi-src/counter

all:
	@for d in $(MYSERVER_SUBDIRS); do (cd $$d && $(MAKE)); done

clean:
	@for d in $(MYSERVER_SUBDIRS); do (cd $$d && $(MAKE) clean); done
dist:
	zip myserver binaries/control binaries/myserver binaries/myserver.ico binaries/MIMEtypes.xml default binaries/myserver.xml.default binaries/virtualhosts.xml.default binaries/readme.txt binaries/logs/.keepme binaries/web/* binaries/system/* binaries/languages/* binaries/cgi-lib/cgi-lib.so binaries/cgi-lib/license.txt binaries/cgi-lib/cgi_manager.h binaries/web/cgi-bin/*.* binaries/web/cgi-src/math_sum/*.* binaries/web/cgi-src/post/*.* binaries/web/cgi-src/counter/*.* binaries/web/downloads/*.* binaries/web/documentation/*.* binaries/web/documentation/images/*
