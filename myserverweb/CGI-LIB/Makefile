all : cgi_manager.cpp ../source/utility.o
	g++ cgi_manager.cpp ../source/utility.o -o ../binaries/cgi-lib/cgi-lib.so -shared

../source/utility.o : ../source/utility.cpp
	g++ -c ../source/utility.cpp -o ../source/utility.o
clean :
	
