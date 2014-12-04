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

/**
* Very simple test to open and close a SQLite database
**/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sqlite3.h"

int main(int argc, char *argv[]) 
{
	fprintf(stderr,"entering main\n");

	sqlite3 *db;
	char *database = "testdata";
	int rc;
	char *zErrMsg = 0;
	
	/* Open Database */
	fprintf(stderr,"about to open database\n");
	rc = sqlite3_open(database, &db);
	if( rc )
	{
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return(1);
	}
	else
	{ 
		printf("Database opened successfully -- now close it\n");
		sqlite3_close(db);
	}

	return 0;
}

