# GNU MyServer
#
# Copyright (C) 2002-2010 Free Software Foundation, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
SUBDIRS = logs system web certificates plugins

EXTRA_DIST = myserver-daemon MIMEtypes.default.xml myserver.default.xml \
						 virtualhosts.default.xml

configdir = $(DESTDIR)$(prefix)/etc/myserver

install: install-recursive
	$(install_sh_DATA) MIMEtypes.default.xml $(configdir)/MIMEtypes.default.xml
	$(install_sh_DATA) myserver.default.xml $(configdir)/myserver.default.xml
	$(install_sh_DATA) virtualhosts.default.xml $(configdir)/virtualhosts.default.xml
