/*
 - Description:
 	This program copies the data of one area to another area. The size is specified by user.
 	
 - Sample input
	- ./a.out 
		invalid argument
	- ./a.out 10 10
		invalid argument
 	- ./a.out 10
	 	expected output: Memory operation done!
	- ./a.out 2049
		size overflow
 	- ./a.out 0
 	   	expected output: Segmentation fault
	- ./a.out -1
		size overflow

 - Potential vulnerability
 	integer overflow
*/
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

int func(unsigned int cbSize) 
{
   if (cbSize < 1024) 
   {
      char *buf1 = malloc(sizeof(char)*(cbSize-1));
      char *buf2 = malloc(sizeof(char)*(cbSize-1));
      memcpy(buf1,buf2,cbSize-1);
      printf("Memory operation done!\n");	
      fflush(stdout);

      return 1;
   } 
   else 
   {
      printf("size overflow!\n ");
      fflush(stdout);
      return 0;
   }
}

int main(int argc, char* argv[])
{
	if(argc != 2)
	{
		printf("Invalid Arguments\n");
		return -1;
	}
	func(atoi(argv[1]));
	return 0;
}

