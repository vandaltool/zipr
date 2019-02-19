#include <assert.h>
/* * The Towers Of Hanoi * C * Copyright (C) 1998 Amit Singh. All Rights Reserved. * * Tested with, ah well ... :) */ 
#include <stdio.h> 
#include <stdlib.h> 
#include <limits.h> 
#include <assert.h>


#define FROM 1 
#define TO 3 
#define USING 2 

void dohanoi(int N, int from, int to, int using) 
{ 
	static int count=0;

	assert(N<100); 	/* 100 would take a long time, and recurse really deeply, chnage this const if you want, but this'll avoid unintential segfaults */
	if (N > 0) 
	{ 
		dohanoi(N-1, from, using, to); 

/* this edit by JDH -  I removed this print statement to make this program more compute bound
 * also, this comment was added as a test of the branching mechanism we're proposing to use from now on
 */
//		printf ("move %d --> %d, count=%d\n", from, to, count++);  
		dohanoi(N-1, using, to, from); 
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

	strtok(0);

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
		dohanoi(N, FROM, TO, USING); 

		printf("Hanoi %d\n", i);
		fflush(stdout);
	}

	exit(0); 
}

