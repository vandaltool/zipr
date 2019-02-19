#include <assert.h>
/* * The Towers Of Hanoi * C * Copyright (C) 1998 Amit Singh. All Rights Reserved. * * Tested with, ah well ... :) */ 
#include <stdio.h> 
#include <stdlib.h> 
#include <limits.h> 
#include <assert.h>
#include <iostream>

using namespace std;


#define FROM 1 
#define TO 3 
#define USING 2 


class foo
{
	public: 

		foo() { cout<<"Constructing a foo\n"; }
		virtual ~foo() { cout<<"Destructing a foo\n"; } 

	private:

		string data;
};


void dohanoi(int N, int from, int to, int p_using) 
{ 
	static int count=0;

	assert(N<100); 	/* 100 would take a long time, and recurse really deeply, chnage this const if you want, but this'll avoid unintential segfaults */
	if (N > 0) 
	{ 
		foo *allocp=new foo [2];
		dohanoi(N-1, from, p_using, to); 
		dohanoi(N-1, p_using, to, from); 
		free(allocp);
	} 
} 


int main (int argc, char **argv) 
{ 
	long int N;
	long int i;
	int j;

	if (argc != 2) 
	{ 
		cerr<< "usage: " << argv[0] << " N"<<endl ; exit(1); 
	} 
	N = strtol(argv[1], (char **)NULL, 10); /* a bit of error checking, LONG_XXX should be there in limits.h */ 

	if (N == LONG_MIN || N == LONG_MAX || N <= 0) 
	{
	 	cerr <<  "illegal value for number of disks"<<endl;
		exit(2); 
	}

	for(i=0;i<N;i++)
	{


		cout << "Hanoi " << i << " ... " << endl;
		fflush(stdout);
		dohanoi(N, FROM, TO, USING); 

		cout << "Hanoi " << i << endl;
	}

	exit(0); 
}

