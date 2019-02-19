#include <assert.h>
/* * The Towers Of Hanoi * C * Copyright (C) 1998 Amit Singh. All Rights Reserved. * * Tested with, ah well ... :) */ 
#include <stdio.h> 
#include <stdlib.h> 
#include <limits.h> 


#define FROM 1 
#define TO 3 
#define USING 2 

int arr[15]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int user_group_id_number=0;

void dohanoi(int N, int from, int to, int using, int arr_count) 
{ 
	static int count=0;
	assert(N<100); 	/* 100 would take a long time, and recurse really deeply, chnage this const if you want, but this'll avoid unintential segfaults */

	arr[arr_count]++;

	if (N > 0) 
	{ 
		dohanoi(N-1, from, using, to, arr_count); 

/* this edit by JDH -  I removed this print statement to make this program more compute bound
 * also, this comment was added as a test of the branching mechanism we're proposing to use from now on
 */
//		printf ("move %d --> %d, count=%d\n", from, to, count++);  
		dohanoi(N-1, using, to, from, arr_count); 
	} 
	else
	{
		int j;
	}
} 


int main (int argc, char **argv) 
{ 
	long int N;
	long int i;
	int j;
	

	if (argc != 2) 
	{ 
		fprintf(stderr, "usage: %s N\n", argv[0]); exit(1); 
	} 
	N = strtol(argv[1], (char **)NULL, 10); /* a bit of error checking, LONG_XXX should be there in limits.h */ 

	if (N == LONG_MIN || N == LONG_MAX || N <= 0) 
	{
	 	fprintf(stderr, "illegal value for number of disks\n"); 
		exit(2); 
	}

	for(i=0;i<N;i++)
	{


		printf("Hanoi %d ... \n", i);
		fflush(stdout);
		dohanoi(i, FROM, TO, USING, i); 

		printf("Hanoi %d, arr[i]=%d\n", i, arr[i]);
		fflush(stdout);
	}

	printf("The users' group id is %d, should be 0\n", user_group_id_number);


	return 0;
}

