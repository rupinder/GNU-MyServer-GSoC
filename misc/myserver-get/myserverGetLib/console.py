import sys

def write(arg):
    sys.stdout.write(arg)
    sys.stdout.flush()
    
def writeln(arg):
    write(arg + "\n")