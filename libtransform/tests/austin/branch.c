/*
 - Description:
 	This program compares a user-specified value with 100, and then keeps decreasing the input value for 5 times.
 	
 - Sample input
	- ./a.out
 	   expected output: Invalid Arguments
        - ./a.out 10 100
 	   expected output: Invalid Arguments 	
	- ./a.out 10
 	   expected output: 10 is no larger than 100, -40 is larger than 100
 	- ./a.out 1000
 	   expected output: all larger than 100
	

 - Potential vulnerability
 	integer overflow
*/
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

int main(int argc, char* argv[])
{
	unsigned int cap;
	int i = 0;

	if(argc != 2)
	{
		printf("Invalid Arguments\n");
		return -1;
	}
	cap = atoi(argv[1]);   

	for(; i < 5; i ++)

	{
			
		if(cap <= 100)// compiler emits unsigned comparison code here, but cap is signed
		   printf("%d is no larger than 100\n", cap);
		else
		   printf("%d is larger than 100\n",cap);
		cap -= 50;
	}
	return 0;
}

