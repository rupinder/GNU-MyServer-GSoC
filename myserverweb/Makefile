# Makefile : Builds myserver, control, CGI-LIB, and math_sum
#

MYSERVER_SUBDIRS=source control CGI-LIB binaries/web/cgi-src/math_sum

all:
	@for d in $(MYSERVER_SUBDIRS); do (cd $$d && $(MAKE)); done

clean:
	@for d in $(MYSERVER_SUBDIRS); do (cd $$d && $(MAKE) clean); done
