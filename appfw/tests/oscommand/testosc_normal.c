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

char *a = "ls";
char *b = "-lt";

/* 
   Pass in: ls -lt           test should exit normally

   Pass in: cat README       test should exit normally (first critical token is tained --> allow anyhow)
                                         we treat this as a benign injectio                                
*/
int main(int argc, char **argv)
{
	char *data = getenv("DATA");

	if (data)
	{
		fprintf(stderr,"data: %s\n", data);
		char cmd[2048];
		sprintf(cmd,"%s", data);
		system(cmd);
	}
	else
	{
		fprintf(stderr,"DATA env. var not set\n");
		exit(1);
	}
}
