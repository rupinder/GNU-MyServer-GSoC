// A quick and durty (incompleat) _finddata_t implimintation

#ifndef LFIND_H
#define LFIND_H

extern "C" 
{
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
}

#define EACCES 1
#define MAX_NAME 256
#define FILE_ATTRIBUTE_DIRECTORY 1

#include <list>

#define intptr_t class _finddata_t *

using namespace std;

struct File_Data 
{
   char name[MAX_NAME];
   int attrib;
   time_t time_write;
   off_t size;
};

class _finddata_t 
{ 
 public:
   char * name;
   int attrib;
   time_t time_write;
   off_t size;
   int findfirst(const char filename[]);
   int findnext();
   int findclose();
 private:
   list<File_Data> File_List;
   list<File_Data>::iterator Curren_File;
};

intptr_t _findfirst(const char filename[], _finddata_t * fdat );
int _findnext(intptr_t crap, _finddata_t * fdat );
int _findclose(_finddata_t * fdat);
#endif
