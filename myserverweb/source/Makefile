# Makefile for myserver

OBJS = AMMimeUtils.o MIME_manager.o cXMLParser.o cgi.o clientsThread.o cserver.o filemanager.o http.o httpmsg.o isapi.o lfind.o mscgi.o myserver.o security.o sockets.o stringutils.o utility.o
CPP = g++ -W -Wall
LD = g++
# Use -lxnet on some unix platforms
CPP-LIBS = -pthread
CPP-FLAGS = 
#-O2 -pipe

all : myserver

myserver : $(OBJS)
	$(LD) $(CPP-LIBS) $(OBJS) -o ../binaries/myserver

AMMimeUtils.o : AMMimeUtils.cpp
	$(CPP) $(CPP-FLAGS) -c AMMimeUtils.cpp

MIME_manager.o : MIME_manager.cpp
	$(CPP) $(CPP-FLAGS) -c MIME_manager.cpp

cXMLParser.o : cXMLParser.cpp
	$(CPP) $(CPP-FLAGS) -c cXMLParser.cpp

cgi.o : cgi.cpp
	$(CPP) $(CPP-FLAGS) -c cgi.cpp

clientsThread.o : clientsThread.cpp
	$(CPP) $(CPP-FLAGS) -c clientsThread.cpp

cserver.o : cserver.cpp
	$(CPP) $(CPP-FLAGS) -c cserver.cpp

filemanager.o : filemanager.cpp
	$(CPP) $(CPP-FLAGS) -c filemanager.cpp

http.o : http.cpp
	$(CPP) $(CPP-FLAGS) -c http.cpp

httpmsg.o : httpmsg.cpp
	$(CPP) $(CPP-FLAGS) -c httpmsg.cpp

isapi.o : isapi.cpp
	$(CPP) $(CPP-FLAGS) -c isapi.cpp

lfind.o : lfind.cpp
	$(CPP) $(CPP-FLAGS) -c lfind.cpp

mscgi.o : mscgi.cpp
	$(CPP) $(CPP-FLAGS) -c mscgi.cpp

myserver.o : myserver.cpp
	$(CPP) $(CPP-FLAGS) -c myserver.cpp

security.o : security.cpp
	$(CPP) $(CPP-FLAGS) -c security.cpp

sockets.o : sockets.cpp
	$(CPP) $(CPP-FLAGS) -c sockets.cpp

stringutils.o : stringutils.cpp
	$(CPP) $(CPP-FLAGS) -c stringutils.cpp

utility.o : utility.cpp
	$(CPP) $(CPP-FLAGS) -c utility.cpp

clean :
	rm -f *.o ../binaries/myserver

#install : all
