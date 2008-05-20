'''
MyServer
Copyright (C) 2008 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
'''

from pycontrol.pycontrol import PyMyServerControl
from sys import argv, exit

if __name__ == "__main__":
    if len(argv) < 5:
        print "Not enough arguments\nUsage python test.py hostname port username password"
        sys.exit(1)

    control = PyMyServerControl(argv[1], int(argv[2]), argv[3], argv[4])

    control.send_header("SHOWCONNECTIONS", 0)

    control.read_header()

    while control.available_data() > 0:
        print control.read()


    control.close()
