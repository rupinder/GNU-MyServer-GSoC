SUBDIRS = languages logs system web certificates external

EXTRA_DIST = myserver.ico license.txt MIMEtypes.xml.unix.default myserver.xml.unix.default readme.txt virtualhosts.xml.unix.default

configdir = $(root_prefix)/etc/myserver

install :
	$(install_sh_DATA) MIMEtypes.xml.unix.default $(configdir)/MIMEtypes.xml
	$(install_sh_DATA) myserver.xml.unix.default $(configdir)/myserver.xml
	$(install_sh_DATA) virtualhosts.xml.unix.default $(configdir)/virtualhosts.xml
