/*
 * Copyright (c) 2013, 2014 - University of Virginia 
 *
 * This file may be used and modified for non-commercial purposes as long as 
 * all copyright, permission, and nonwarranty notices are preserved.  
 * Redistribution is prohibited without prior written consent from the University 
 * of Virginia.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: University of Virginia
 * e-mail: jwd@virginia.com
 * URL   : http://www.cs.virginia.edu/
 *
 */

#include <stdlib.h>
#include <stdio.h>

int main (int argc, char **argv) 
{
	char *foobar = NULL;
	int i, j;

	i = atoi(argv[1]);

	printf("i = %d\n", i);

	foobar = malloc(i); // sign/unsigned error here

	if (!foobar)
	{
		printf("malloc failed\n");
		return 1;
	}

	printf("i = %d\n", i);

	// need the strncpy to induce SIGNEDNESS annotation
	bzero(foobar, i);
	strncpy(foobar, "yeah whatever", i-1);
	printf("%d: %s\n", i, foobar);

	return 0;
}
