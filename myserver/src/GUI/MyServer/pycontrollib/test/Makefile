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
# MyServer
# Copyright (C) 2009 Free Software Foundation, Inc.
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

PYTHON=/usr/bin/python
PYTHONPATH:=$(shell dirname `pwd`):${PYTHONPATH}

.PHONY: test log_test definition_test mimetypes_test vhost_test config_test \
	security_test

test: log_test definition_test mimetypes_test vhost_test config_test \
	security_test

log_test:
	-PYTHONPATH=${PYTHONPATH} ${PYTHON} log_test.py

definition_test:
	-PYTHONPATH=${PYTHONPATH} ${PYTHON} definition_test.py

mimetypes_test:
	-PYTHONPATH=${PYTHONPATH} ${PYTHON} mimetypes_test.py

vhost_test:
	-PYTHONPATH=${PYTHONPATH} ${PYTHON} vhost_test.py

config_test:
	-PYTHONPATH=${PYTHONPATH} ${PYTHON} config_test.py

security_test:
	-PYTHONPATH=${PYTHONPATH} ${PYTHON} security_test.py

.PHONY: clean
clean:
	rm *.py[co]
