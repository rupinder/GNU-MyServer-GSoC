# Makefile : Builds myserver, control, and CGI-LIB
#

MYSERVER_SUBDIRS=source control CGI-LIB

all:
	@for d in $(MYSERVER_SUBDIRS); do (cd $$d && $(MAKE)); done

clean:
	@for d in $(MYSERVER_SUBDIRS); do (cd $$d && $(MAKE) clean); done
