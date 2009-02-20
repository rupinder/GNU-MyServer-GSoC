from myserverGetLib.general import MyServerGet
import sys
import getopt

def usage(arg):
    print "usage: myserver-get update "    
        
def main(argv):
    myserverGet = MyServerGet()
    argument = "" 
    command = ""                        
    
    commands = {"":usage, "update":myserverGet.update}
        
    try:                                
        opts, args = getopt.getopt(argv, "h", ["help"]) 
    except getopt.GetoptError:           
        usage()                          
        sys.exit(2)
    for opt, arg in opts:                
        if opt in ("-h", "--help"):      
            usage("")                     
            sys.exit()                  
    
    for arg in args:
        if arg in ("update"):
            command = arg
        else:
            argument = arg
    
    commands[command](argument)

if __name__ == "__main__":
    main(sys.argv[1:])