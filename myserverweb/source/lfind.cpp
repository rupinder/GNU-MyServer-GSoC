// A quick and durty (incompleat) _finddata_t implimintation
//
#include "../include/lfind.h"

extern "C" {
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
}

void * _alloca(size_t size)
{
	return malloc(size);
}

int _finddata_t::findfirst(const char filename[])
{
   int OUT[2];
   if (pipe(OUT) == -1)
     return -1;

   int pid = fork();

   if(pid < 0) // a bad thing happend
     {
	close(OUT[0]);
	close(OUT[1]);

	return -1;
     }
   else if(pid == 0) // child
     {
	close(0); // close stdin
	close(OUT[0]); // close read
	close(1); // close stdout
	dup2(OUT[1], 1);
	close(OUT[1]);

	execlp("ls", "ls", filename, NULL);

	exit(1);
     }
   // Parent
   close(OUT[1]); // close the write of OUT

   struct File_Data Temp;

   int index = 0;
   char Buffer[MAX_NAME];
   char Temp_Name[MAX_NAME+MAX_NAME];
   struct stat F_Stats;

   File_List.clear();
   
   while(read(OUT[0], &Buffer[index], 1) !=0)
     {
	if(Buffer[index] == '\n' && index != MAX_NAME-1 && index != 0)
	  {
	     Buffer[index] = '\0';

	     strcpy(Temp.name, Buffer);
	     
	     snprintf(Temp_Name, MAX_NAME+MAX_NAME, "%s%s", filename, Buffer);
	     
	     stat(Temp_Name, &F_Stats);

	     if(S_ISDIR(F_Stats.st_mode))
	       Temp.attrib = FILE_ATTRIBUTE_DIRECTORY;
	     else
	       Temp.attrib = 0;
	     
	     Temp.time_write = F_Stats.st_mtime;
	     
	     Temp.size = F_Stats.st_size;
	     
	     File_List.push_back(Temp);

	     index = 0;
	  }
	else
	  index++;
     }

   close(OUT[0]);
   
   waitpid(pid, NULL, 0);

   Curren_File = File_List.begin();  // get the fist entry
   
   name = Curren_File->name;

   attrib = Curren_File->attrib;
   
   size = Curren_File->size;
   
   time_write = Curren_File->time_write;

   return 0;
}

int _finddata_t::findnext()
{
   Curren_File++;
   
   if(Curren_File == File_List.end())
     return -1;

   name = Curren_File->name;
   attrib = Curren_File->attrib;
   size = Curren_File->size;
   time_write = Curren_File->time_write;
   return 0;

}

int _finddata_t::findclose()
{
   File_List.clear();
}

intptr_t _findfirst(const char filename[], _finddata_t * fdat )
{

   return (fdat->findfirst(filename) == 0)? fdat : (intptr_t)-1;
}

int _findnext(intptr_t crap, _finddata_t * fdat )
{

   return fdat->findnext();
}

int _findclose(_finddata_t * fdat)
{

   return fdat->findclose();
}

