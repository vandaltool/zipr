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

