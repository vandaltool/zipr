/*
- Description
This small creates a random valued array whose size is specified by user. Besides, it returns a specific element by request as long as the posision is smaller than the size.

- Sample input & Expected result
	- ./a.out
		invalid inputs
	- ./a.out 0x40
		invalid inputs
	- ./a.out 100 120
		index overflow
	- ./a.out 100 20
		return Array[20]
	- ./a.out 100 -3
		Element -3: ? (index underflow, not handled)
	- ./a.out 100 0xf0000001
		Segmentation fault

- Potential Problem
unsigned integer overflow 

*/


#include <stdlib.h>
#include <malloc.h>

int main(int argc, char* argv[])
{
	int size;
	int pos;
	int i;
	char *buf;
	char *ptr = NULL;


	if(argc != 3)
	{
		printf("invalid arguments!\n");
		return -1;
	}	
	
	size = strtoul(argv[1],&ptr, 0);
    	pos = strtoul(argv[2],&ptr, 0);
	
	fflush(stdout);

	buf=(char*)malloc(size * sizeof(char));	
	printf("Requested position: %d\n", pos);
	
	fflush(stdout);
	
	if(!buf)
	{
		printf("Allocation failure!\n");
		return -1;
	}
	
	for(i = 0; i < size; i ++ )
		buf[i] = (i * 50) %255;
	
	fflush(stdout);	
 	if(pos < size)
		printf("Element %d: %c\n",pos, buf[pos]);
	else
		printf("Index Overflow\n");	
        return 0;
}
