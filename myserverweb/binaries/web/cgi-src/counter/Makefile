# Makefile for counter

OBJS = main.o counter_output.o writepng.o
CPP = g++
LD = g++
CC = gcc
# uses libraies libpng math and zlib
LD-FLAGS = -lm -lz -lpng -L/usr/lib -L/usr/local/lib
CC-FLAGS = -O2 -pipe -I/usr/include -I/usr/local/include
CPP-FLAGS = -O2 -pipe -I/usr/include -I/usr/local/include

all : counter
	
counter : $(OBJS)
	$(LD) $(LD-FLAGS) $(OBJS) -shared -o ../../cgi-bin/counter.mscgi

main.o : main.cpp
	$(CPP) $(CPP-FLAGS) -c main.cpp

counter_output.o : counter_output.cpp
	$(CPP) $(CPP-FLAGS) -c counter_output.cpp

writepng.o : writepng.c
	$(CC) $(CC-FLAGS) -c writepng.c

clean :
	rm $(OBJS)

