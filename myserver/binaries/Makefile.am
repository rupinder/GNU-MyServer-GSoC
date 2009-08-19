SUBDIRS = logs system web certificates plugins

EXTRA_DIST = myserver-daemon MIMEtypes.default.xml myserver.default.xml \
						 virtualhosts.default.xml

configdir = $(DESTDIR)/etc/myserver

install: install-recursive
	$(install_sh_DATA) MIMEtypes.default.xml $(configdir)/MIMEtypes.xml
	$(install_sh_DATA) myserver.default.xml $(configdir)/myserver.xml
	$(install_sh_DATA) virtualhosts.default.xml $(configdir)/virtualhosts.xml
