#SUBDIRS = languages logs system web certificates external
SUBDIRS = languages logs system web certificates

EXTRA_DIST = myserver-daemon myserver.ico license.txt MIMEtypes.xml.unix.default \
myserver.xml.unix.default readme.txt virtualhosts.xml.unix.default

configdir = $(root_prefix)/etc/myserver

install: install-recursive
	$(install_sh_DATA) MIMEtypes.xml.unix.default $(configdir)/MIMEtypes.xml
	$(install_sh_DATA) myserver.xml.unix.default $(configdir)/myserver.xml
	$(install_sh_DATA) virtualhosts.xml.unix.default $(configdir)/virtualhosts.xml
