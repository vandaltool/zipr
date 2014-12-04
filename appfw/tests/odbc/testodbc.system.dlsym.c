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
#include <dlfcn.h>

int main(int argc, char **argv)
{
	char *commandInject = getenv("INJECT");
	int (*my_system)(const char *) = 0L;

 	void *libc = dlopen("libc.so.6", RTLD_LAZY);
	if (!libc)
	{
		fprintf(stderr, "couldn't find libc.so: exiting");
		exit(-1);
	}

	my_system = dlsym(libc, "system");
	if (!my_system)
	{
		fprintf(stderr, "couldn't resolve system: exiting");
		exit(-1);
	}

	if (commandInject)
		return (*my_system)(commandInject);
	else
		return (*my_system)(argv[1]);
}
