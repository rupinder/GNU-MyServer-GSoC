# Makefile for myserver

OBJS = AMMimeUtils.o http_headers.o gzip.o MIME_manager.o winCGI.o cXMLParser.o cgi.o clientsThread.o cserver.o filemanager.o http.o httpmsg.o isapi.o lfind.o mscgi.o myserver.o security.o sockets.o stringutils.o utility.o vhosts.o fastCGI.o md5.o
CPP = g++ -W -Wall
LD = g++
# Use -lxnet on some unix platforms
# -pthread is moved by -lpthread, defined by xml2-config
CPP-LIBS = -lssl -lcrypto `xml2-config --libs`

# Comment -D__linux__, if compiling not under linux
CPP-FLAGS = `xml2-config --cflags` -D__linux__
#-O2 -pipe

all : myserver
	
myserver : $(OBJS)
	$(LD) $(OBJS) $(CPP-LIBS) -o ../binaries/myserver

AMMimeUtils.o : AMMimeUtils.cpp
	$(CPP) $(CPP-FLAGS) -c AMMimeUtils.cpp

gzip.o : gzip.cpp
	$(CPP) $(CPP-FLAGS) -c gzip.cpp	

http_headers.o : http_headers.cpp
	$(CPP) $(CPP-FLAGS) -c http_headers.cpp

md5.o : md5.cpp
	$(CPP) $(CPP-FLAGS) -c md5.cpp

winCGI.o : winCGI.cpp
	$(CPP) $(CPP-FLAGS) -c winCGI.cpp

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

vhosts.o : vhosts.cpp
	$(CPP) $(CPP-FLAGS) -c vhosts.cpp

fastCGI.o : fastCGI.cpp
	$(CPP) $(CPP-FLAGS) -c fastCGI.cpp

clean :
	rm -f *.o ../binaries/myserver

release :
	rm -f *.o


#install : all
