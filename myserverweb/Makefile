# Makefile : Builds myserver and control
#

MYSERVER_SUBDIRS=source control

all:
	@for d in $(MYSERVER_SUBDIRS); do (cd $$d && $(MAKE)); done

clean:
	@for d in $(MYSERVER_SUBDIRS); do (cd $$d && $(MAKE) clean); done
