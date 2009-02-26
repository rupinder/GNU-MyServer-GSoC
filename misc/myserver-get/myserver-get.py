from myserverGetLib.general import MyServerGet
import sys
import getopt
import myserverGetLib.config


def usage(arg):
    print "usage: myserver-get update "    
        
def main(argv):
    myserverGet = MyServerGet() 
    command = ""                        
    
    commands = {"":usage, "update":myserverGet.update, "search":myserverGet.search,"install":myserverGet.install}
    

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
        if arg in ("update","search","install"):
            command = arg
        else:
            arguments.append(arg)
    
    commands[command](arguments)

if __name__ == "__main__":
    main(sys.argv[1:])