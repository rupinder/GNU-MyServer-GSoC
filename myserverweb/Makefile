# Makefile : Builds myserver, control, CGI-LIB, math_sum, and post
#

MYSERVER_SUBDIRS=source control CGI-LIB binaries/web/cgi-src/math_sum binaries/web/cgi-src/post

all:
	@for d in $(MYSERVER_SUBDIRS); do (cd $$d && $(MAKE)); done

clean:
	@for d in $(MYSERVER_SUBDIRS); do (cd $$d && $(MAKE) clean); done
