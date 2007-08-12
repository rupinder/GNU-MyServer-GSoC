#!/bin/sh
# MyServer
#
# http://www.myserverproject.net
#
# Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007 The MyServer Team
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


(aclocal --version) < /dev/null > /dev/null 2>&1 ||
{
    echo;
    echo "You will need aclocal to compile MyServer";
    echo;
    exit;
}

(autoheader --version) < /dev/null > /dev/null 2>&1 ||
{
    echo;
    echo "You will need autoheader to compile MyServer";
    echo;
    exit;
}

(automake --version) < /dev/null > /dev/null 2>&1 ||
{
    echo;
    echo "You will need automake to compile MyServer";
    echo;
    exit;
}

(autoconf --version) < /dev/null > /dev/null 2>&1 ||
{
    echo;
    echo "You will need autoconf to compile MyServer";
    echo;
    exit;
}

echo "Creating configuration files for MyServer. Please wait..."
echo;

aclocal -I m4
autoheader
automake -a
autoconf
