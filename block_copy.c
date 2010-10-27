/* * The Towers Of Hanoi * C * Copyright (C) 1998 Amit Singh. All Rights Reserved. * * Tested with, ah well ... :) */ 
#include <stdio.h> 
#include <stdlib.h> 
#include <limits.h> 


#define FROM 1 
#define TO 3 
#define USING 2 


int block_copy(char* dst, char* src, int len)
{

	while(len-- > 0)
	{
		*dst++=*src++;
	}
	
}

int k=0;
int main (int argc, char **argv) 
{ 

	int **i, **j;


	i=malloc(4);
	j=malloc(4);

	*i=&k;
	
	block_copy((char*)j,(char*)i,4);


	**j=1;

	printf("k=%d\n", k);




	exit(0); 
}

