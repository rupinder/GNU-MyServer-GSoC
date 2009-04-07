# MyServer
# Copyright (C) 2009 The MyServer Team
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


from myserverGetLib.general import MyServerGet
import sys
import getopt
import myserverGetLib.config


def usage(arg):
    print "usage: myserver-get [update|search|install|source|source-auto|remove] [-v|--verbose|-h|--help] "    
        
def main(argv):
    myserverGet = MyServerGet() 
    command = ""                        
    myserverGetLib.config.loadConfig()
    commands = {"":usage, "update":myserverGet.update, "search":myserverGet.search,"install":myserverGet.install,"source":myserverGet.source,"source-auto":myserverGet.sourceAuto,"remove":myserverGet.remove}
    

    import os
    if os.name=="nt":
        myserverGetLib.config.arch = "win32"
    else:
        import string
        myserverGetLib.config.arch = string.lower(os.uname()[0])
        if string.find(os.uname()[4],"64")==-1:
            myserverGetLib.config.arch = myserverGetLib.config.arch + "32"
        else:
            myserverGetLib.config.arch = myserverGetLib.config.arch + "64"
        
    try:                                
        opts, args = getopt.getopt(argv, "hv", ["help","verbose"]) 
    except getopt.GetoptError:           
        usage()                          
        sys.exit(2)
    for opt, arg in opts:                
        if opt in ("-h", "--help"):      
            usage("")                     
            sys.exit()
        elif opt in ("-v", "--verbose"):      
            myserverGetLib.config.verbose = True   
    
    arguments = []
    for arg in args:
        if arg in ("update","search","install","source","source-auto","remove"):
            command = arg
        else:
            arguments.append(arg)
    
    commands[command](arguments)

if __name__ == "__main__":
    main(sys.argv[1:])