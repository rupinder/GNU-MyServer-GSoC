# Makefile for myserver control

OBJS = configuration.o configureMIME.o control.o lfind.o
CPP = g++ -W -Wall
LD = g++
# Use -lxnet on some unix platforms
CPP-LIBS = `wx-config --libs` `xml2-config --libs`
CPP-FLAGS =  `wx-config --cppflags` -O2 -pipe `xml2-config --cflags`

all : wxInstalled control

wxInstalled :
	if [ -x /usr/local/bin/wx-config ]; then echo wxWindows is installed; else if [ -x /usr/bin/wx-config ]; then echo wxWindows is installed; else echo wxWindows is not installed; exit 1; fi; fi

control : $(OBJS)
	$(LD) $(CPP-LIBS) $(OBJS) -o ../binaries/control

configuration.o : configuration.cpp
	$(CPP) $(CPP-FLAGS) -c configuration.cpp

configureMIME.o : configureMIME.cpp
	$(CPP) $(CPP-FLAGS) -c configureMIME.cpp

control.o : control.cpp
	$(CPP) $(CPP-FLAGS) -c control.cpp

lfind.o : ../source/lfind.cpp
	$(CPP) -c ../source/lfind.cpp -o ./lfind.o

clean :
	rm -f *.o ../binaries/control

release :
	rm -f *.o

#install : all



