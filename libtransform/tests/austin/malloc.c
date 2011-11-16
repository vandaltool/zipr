/*
- Description
This small program's goal is to allocate space for an array of structures based on a usersupplied argument. 
It then fills the structures with data and performs whatever other operations are necessary on that data.

- Sample input & Expected result
	- ./a.out 
		invalid argument
	- ./a.out 100 1000
		invalid argument
        - ./a.out 100
                enough space allocated, blocks declared;
        - ./a.out 0x40
                enough space allocated, blocks declared;
        - ./a.out 0x40000001
                *** not enough space allocated, segmentation fault;

- Potential Problem
unsigned integer overflow 


*/


#include <stdlib.h>
#include <stdio.h>

struct thing {
    char buf[100];
    int i;
};

int main(int argc, char* argv[])
{
    unsigned int num; //user defined size
    unsigned int space; //total space to allocate

    char *ptr = NULL;
    struct thing *records = NULL;
    int i = 0; 

    if (argc != 2) //argument check
    {
        printf("invalid value\n");
        return -1;
    }
		
    num = strtoul(argv[1],&ptr, 0);

    if (*ptr != '\0') //argument check
    {
        printf("invalid value\n");
        return -1;
    }

    printf("Memory allocated: %u\n",(num *sizeof(struct thing)));
    fflush(stdout);
    space = num *sizeof(struct thing); //calculate total space to allocate (unsigned integer overflow) POTENTIAL OVERFLOW HERE
    printf("Total size: %u\n", space);
    fflush(stdout);
    if ((records = (struct thing *)malloc(space)) == NULL) //allocation check
    {
        printf("Error allocating storage for %u records\n",num);
        return -1;
    }
    
    for(i = num - 1; i >= 0; i --) //block declare
    {
        printf("Block %d has been declared\n", i);
        (*(records + i)).i = i;
    }

    printf("%d blocks have been declared\n", num - i - 1);
    fflush(stdout);
    free(records);
        
     return 0;
}
