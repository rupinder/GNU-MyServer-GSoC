SUBDIRS = languages logs system web certificates plugins

EXTRA_DIST = myserver-daemon MIMEtypes.default.xml myserver.default.xml \
						 virtualhosts.default.xml

configdir = $(root_prefix)/etc/myserver

install: install-recursive
	$(install_sh_DATA) MIMEtypes.xml.unix.default $(configdir)/MIMEtypes.xml
	$(install_sh_DATA) myserver.xml.unix.default $(configdir)/myserver.xml
	$(install_sh_DATA) virtualhosts.xml.unix.default $(configdir)/virtualhosts.xml
