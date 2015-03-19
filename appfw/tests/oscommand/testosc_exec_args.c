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

#include <stdio.h>
#include <stdlib.h>

char *foobar="ls";
char *foobar3="%s-lt%s";
char *foobar5="%sar%s";

int main(int argc, char **argv)
{
	char *foobar2="ls";
	char *foobar3="%s;%s";
	char *data = getenv("DATA");

	if (!data)
	{
		fprintf(stderr,"DATA env. var not set\n");
		exit(1);
	}
	else
	{
		fprintf(stderr,"DATA env: %s\n", data);
	}

	char *args[2];
	args[0] = malloc(1024);
	args[1] = NULL;
	sprintf(args[0], data);
	execv("/bin/ls", args);
}
